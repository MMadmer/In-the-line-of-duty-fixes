#define DIRECTINPUT_VERSION 0x0800

#include "iat_hook.h"
#include "console_hooks.h"
#include "update_bridge.h"
#include "inventory_hooks.h"
#include "script_patch.h"
#include "sha256.h"

#include <Windows.h>
#include <dinput.h>
#include <shellapi.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>

namespace
{
using DirectInput8CreateFn = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
using CreateFileMappingAFn = decltype(&CreateFileMappingA);
using ReadFn = int(__cdecl*)(int, void*, unsigned int);

constexpr ild::Sha256 expected_engine_hash{
    std::byte{0xB2}, std::byte{0x2B}, std::byte{0xC1}, std::byte{0x5B}, std::byte{0x94}, std::byte{0xA2},
    std::byte{0xA5}, std::byte{0x8C}, std::byte{0x4E}, std::byte{0x70}, std::byte{0x46}, std::byte{0xE4},
    std::byte{0x6D}, std::byte{0x46}, std::byte{0xA3}, std::byte{0x75}, std::byte{0x0D}, std::byte{0x80},
    std::byte{0xC3}, std::byte{0x99}, std::byte{0xBA}, std::byte{0x8F}, std::byte{0x37}, std::byte{0xA2},
    std::byte{0xEF}, std::byte{0x40}, std::byte{0xCC}, std::byte{0xF7}, std::byte{0x8E}, std::byte{0xE3},
    std::byte{0x12}, std::byte{0x6D},
};

constexpr ild::Sha256 expected_core_hash{
    std::byte{0xE6}, std::byte{0xB6}, std::byte{0xE0}, std::byte{0xC1}, std::byte{0x50}, std::byte{0xC4},
    std::byte{0xC5}, std::byte{0x11}, std::byte{0xB2}, std::byte{0x99}, std::byte{0xAA}, std::byte{0x3C},
    std::byte{0x0E}, std::byte{0x4E}, std::byte{0x91}, std::byte{0xD6}, std::byte{0xB7}, std::byte{0x7A},
    std::byte{0x48}, std::byte{0x01}, std::byte{0xB2}, std::byte{0x3C}, std::byte{0x9B}, std::byte{0x6E},
    std::byte{0x55}, std::byte{0xBF}, std::byte{0x7A}, std::byte{0x55}, std::byte{0x7A}, std::byte{0xBE},
    std::byte{0xEE}, std::byte{0xEB},
};

constexpr ild::Sha256 expected_script_hash{
    std::byte{0x2C}, std::byte{0x5C}, std::byte{0x2C}, std::byte{0xCD}, std::byte{0x95}, std::byte{0xAE},
    std::byte{0x5B}, std::byte{0x91}, std::byte{0xF5}, std::byte{0x8C}, std::byte{0x98}, std::byte{0x8D},
    std::byte{0x77}, std::byte{0x7C}, std::byte{0x21}, std::byte{0x44}, std::byte{0x4B}, std::byte{0x83},
    std::byte{0x2B}, std::byte{0x74}, std::byte{0x6A}, std::byte{0xFE}, std::byte{0x3B}, std::byte{0x56},
    std::byte{0x5D}, std::byte{0xF9}, std::byte{0xA0}, std::byte{0xE4}, std::byte{0x2F}, std::byte{0x7A},
    std::byte{0xF2}, std::byte{0xEE},
};

CreateFileMappingAFn real_create_file_mapping{};
ReadFn real_read{};
constexpr ild::Sha256 expected_actor_hash{
    std::byte{0x34}, std::byte{0x73}, std::byte{0x21}, std::byte{0x68}, std::byte{0xAF}, std::byte{0x8F},
    std::byte{0x79}, std::byte{0x41}, std::byte{0xA8}, std::byte{0xBC}, std::byte{0x87}, std::byte{0xB7},
    std::byte{0x48}, std::byte{0x1A}, std::byte{0x8A}, std::byte{0x86}, std::byte{0x86}, std::byte{0xB4},
    std::byte{0x47}, std::byte{0xC2}, std::byte{0x7C}, std::byte{0x25}, std::byte{0xA9}, std::byte{0x14},
    std::byte{0xA1}, std::byte{0x19}, std::byte{0x86}, std::byte{0xD4}, std::byte{0x23}, std::byte{0xB5},
    std::byte{0xC5}, std::byte{0xB9}
};
constexpr ild::Sha256 expected_menu_hash{
    std::byte{0xF1}, std::byte{0x85}, std::byte{0x03}, std::byte{0x04}, std::byte{0x0E}, std::byte{0xD2},
    std::byte{0xFB}, std::byte{0xBB}, std::byte{0x84}, std::byte{0x16}, std::byte{0x18}, std::byte{0x57},
    std::byte{0xC0}, std::byte{0xB5}, std::byte{0x5C}, std::byte{0x84}, std::byte{0xE8}, std::byte{0x10},
    std::byte{0x1C}, std::byte{0xC9}, std::byte{0x11}, std::byte{0x86}, std::byte{0x89}, std::byte{0x78},
    std::byte{0x21}, std::byte{0x7A}, std::byte{0x8E}, std::byte{0x2C}, std::byte{0x11}, std::byte{0x57},
    std::byte{0x7E}, std::byte{0x08},
};
std::filesystem::path target_script;
std::filesystem::path target_actor;
std::filesystem::path game_root;
INIT_ONCE install_once = INIT_ONCE_STATIC_INIT;
INIT_ONCE message_once = INIT_ONCE_STATIC_INIT;

[[nodiscard]] std::wstring quote_argument(const std::wstring& value)
{
    std::wstring result = L"\"";
    std::size_t slashes{};
    for (const auto character : value)
    {
        if (character == L'\\') { ++slashes; continue; }
        result.append(character == L'\"' ? slashes * 2 + 1 : slashes, L'\\');
        slashes = 0;
        result.push_back(character);
    }
    result.append(slashes * 2, L'\\');
    result.push_back(L'\"');
    return result;
}

int __cdecl read_hook(int file, void* buffer, unsigned int size)
{
    static_cast<void>(ild::install_inventory_hooks(game_root));
    const auto read = real_read(file, buffer, size);
    if ((read == 8844 || read == 18708) && buffer)
    {
        const auto bytes = std::span(static_cast<std::byte*>(buffer), static_cast<std::size_t>(read));
        ild::Sha256 actual{};
        if (ild::sha256_bytes(bytes, actual) && actual == expected_menu_hash)
            static_cast<void>(ild::script_patch::bind_update_menu(bytes));
        else if (actual == expected_actor_hash)
            static_cast<void>(ild::script_patch::bind_gameplay(bytes));
    }
    return read;
}

void start_update_service(const std::filesystem::path& engine)
{
    const auto root = engine.parent_path().parent_path();
    const auto updater = root / L"InTheLineOfDutyFixesUpdater.exe";
    std::wstring restart_arguments;
    bool qa{};
    int count{};
    const auto arguments = CommandLineToArgvW(GetCommandLineW(), &count);
    if (arguments)
    {
        for (int index = 1; index < count; ++index)
        {
            if (std::wstring_view(arguments[index]) == L"-qa_update") qa = true;
            if (!restart_arguments.empty()) restart_arguments.push_back(L' ');
            restart_arguments += quote_argument(arguments[index]);
        }
        LocalFree(arguments);
    }
    std::wstring command = quote_argument(updater.wstring()) + L" --service --game-dir " +
        quote_argument(root.wstring()) + L" --game-pid " + std::to_wstring(GetCurrentProcessId()) +
        L" --restart-exe " + quote_argument(engine.wstring()) + L" --restart-args " + quote_argument(restart_arguments);
    if (qa)
    {
        std::array<wchar_t, 2048> api{};
        const auto length = GetEnvironmentVariableW(L"ILD_QA_UPDATE_API", api.data(), static_cast<DWORD>(api.size()));
        if (length && length < api.size()) command += L" --qa --api " + quote_argument(api.data());
    }
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process{};
    if (CreateProcessW(updater.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
            nullptr, root.c_str(), &startup, &process))
    {
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
    }
}

[[nodiscard]] std::filesystem::path executable_path()
{
    std::wstring buffer(32768, L'\0');
    const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (!length || length >= buffer.size())
    {
        return {};
    }

    buffer.resize(length);
    return std::filesystem::path(buffer);
}

[[nodiscard]] bool equals_hash(const std::filesystem::path& path, const ild::Sha256& expected)
{
    ild::Sha256 actual{};
    return ild::sha256_file(path, actual) && actual == expected;
}

BOOL CALLBACK show_unsupported_message(PINIT_ONCE, PVOID parameter, PVOID*)
{
    MessageBoxW(
        nullptr,
        static_cast<const wchar_t*>(parameter),
        L"In the Line of Duty Fixes",
        MB_OK | MB_ICONWARNING | MB_SYSTEMMODAL);
    return TRUE;
}

void report_unsupported(const wchar_t* reason)
{
    InitOnceExecuteOnce(&message_once, show_unsupported_message, const_cast<wchar_t*>(reason), nullptr);
}

[[nodiscard]] bool handle_matches_target(HANDLE file)
{
    std::wstring buffer(32768, L'\0');
    const auto length = GetFinalPathNameByHandleW(file, buffer.data(), static_cast<DWORD>(buffer.size()),
        FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if (!length || length >= buffer.size())
    {
        return false;
    }

    buffer.resize(length);
    constexpr std::wstring_view extended_prefix = L"\\\\?\\";
    if (buffer.starts_with(extended_prefix))
    {
        buffer.erase(0, extended_prefix.size());
    }

    return _wcsicmp(buffer.c_str(), target_script.c_str()) == 0 ||
        _wcsicmp(buffer.c_str(), target_actor.c_str()) == 0;
}

HANDLE WINAPI create_file_mapping_hook(
    HANDLE file,
    LPSECURITY_ATTRIBUTES attributes,
    DWORD protect,
    DWORD maximum_size_high,
    DWORD maximum_size_low,
    LPCSTR name)
{
    if (!real_create_file_mapping)
    {
        return CreateFileMappingA(file, attributes, protect, maximum_size_high, maximum_size_low, name);
    }
    if (file == INVALID_HANDLE_VALUE || !handle_matches_target(file))
    {
        return real_create_file_mapping(file, attributes, protect, maximum_size_high, maximum_size_low, name);
    }

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > 1024 * 1024)
    {
        return real_create_file_mapping(file, attributes, protect, maximum_size_high, maximum_size_low, name);
    }

    const auto source_mapping = real_create_file_mapping(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!source_mapping)
    {
        return real_create_file_mapping(file, attributes, protect, maximum_size_high, maximum_size_low, name);
    }

    const auto private_mapping = real_create_file_mapping(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        size.HighPart,
        size.LowPart,
        nullptr);
    if (!private_mapping)
    {
        CloseHandle(source_mapping);
        return real_create_file_mapping(file, attributes, protect, maximum_size_high, maximum_size_low, name);
    }

    const auto source = MapViewOfFile(source_mapping, FILE_MAP_READ, 0, 0, static_cast<SIZE_T>(size.QuadPart));
    const auto destination = MapViewOfFile(private_mapping, FILE_MAP_WRITE, 0, 0, static_cast<SIZE_T>(size.QuadPart));
    bool patched{};
    if (source && destination)
    {
        std::memcpy(destination, source, static_cast<SIZE_T>(size.QuadPart));
        const auto bytes = std::span(static_cast<std::byte*>(destination), static_cast<SIZE_T>(size.QuadPart));
        ild::Sha256 source_hash{};
        if (ild::sha256_bytes(bytes, source_hash))
        {
            patched = source_hash == expected_script_hash ?
                ild::script_patch::remove_console_execution(bytes) == ild::script_patch::Result::applied :
                source_hash == expected_actor_hash && ild::script_patch::bind_gameplay(bytes);
        }
    }

    if (source)
    {
        UnmapViewOfFile(source);
    }
    if (destination)
    {
        UnmapViewOfFile(destination);
    }
    CloseHandle(source_mapping);

    if (patched)
    {
        return private_mapping;
    }

    CloseHandle(private_mapping);
    report_unsupported(L"The installed _g.script did not match the validated console fix. The fix was disabled.");
    return real_create_file_mapping(file, attributes, protect, maximum_size_high, maximum_size_low, name);
}

BOOL CALLBACK install_fixes(PINIT_ONCE, PVOID, PVOID*)
{
    const auto engine = executable_path();
    if (engine.empty())
    {
        report_unsupported(L"The game executable path could not be resolved. The fix was disabled.");
        return TRUE;
    }

    const auto bin = engine.parent_path();
    const auto root = bin.parent_path();
    game_root = root;
    const auto core = bin / L"xrCore.dll";
    target_script = root / L"gamedata" / L"scripts" / L"_g.script";
    target_actor = root / L"gamedata" / L"scripts" / L"bind_stalker.script";

    if (!equals_hash(engine, expected_engine_hash) || !equals_hash(core, expected_core_hash) ||
        !equals_hash(target_script, expected_script_hash))
    {
        report_unsupported(L"This game or mod build is not supported. No in-memory fixes were applied.");
        return TRUE;
    }

    const auto core_module = GetModuleHandleW(L"xrCore.dll");
    if (!ild::install_console_hooks(GetModuleHandleW(nullptr)))
        report_unsupported(L"The validated console entry points could not be hooked. Console editing was not changed.");
#ifdef ILD_CONSOLE_QA
    if (std::wcsstr(GetCommandLineW(), L"-ild_console_qa"))
        ild::run_console_selftest(GetModuleHandleW(nullptr), root / L"console-qa.txt");
#endif
    real_create_file_mapping = reinterpret_cast<CreateFileMappingAFn>(ild::replace_iat_import(
        core_module,
        "KERNEL32.dll",
        "CreateFileMappingA",
        reinterpret_cast<void*>(&create_file_mapping_hook)));

    if (!real_create_file_mapping)
    {
        report_unsupported(L"The validated xrCore.dll import could not be hooked. The fix was disabled.");
    }

    const auto menu = root / L"gamedata" / L"scripts" / L"ui_main_menu.script";
    if (std::filesystem::exists(root / L"InTheLineOfDutyFixesUpdater.exe") &&
        std::filesystem::exists(root / L"gamedata" / L"scripts" / L"ild_fix_ui.script") &&
        std::filesystem::exists(root / L"gamedata" / L"config" / L"ui" / L"ild_fixes_update.xml") &&
        equals_hash(menu, expected_menu_hash) && ild::install_update_bridge(GetModuleHandleW(nullptr), root))
    {
        const auto crt = GetModuleHandleW(L"MSVCR80.dll");
        real_read = crt ? reinterpret_cast<ReadFn>(GetProcAddress(crt, "_read")) : nullptr;
        if (real_read && ild::replace_iat_import(core_module, "MSVCR80.dll", "_read",
                reinterpret_cast<void*>(&read_hook)))
            start_update_service(engine);
    }

    return TRUE;
}

[[nodiscard]] DirectInput8CreateFn load_real_direct_input()
{
    static const auto function = []
    {
        std::wstring path(MAX_PATH, L'\0');
        const auto length = GetSystemDirectoryW(path.data(), static_cast<UINT>(path.size()));
        if (!length || length >= path.size())
        {
            return DirectInput8CreateFn{};
        }

        path.resize(length);
        path += L"\\dinput8.dll";
        const auto module = LoadLibraryW(path.c_str());
        return module ? reinterpret_cast<DirectInput8CreateFn>(GetProcAddress(module, "DirectInput8Create")) : nullptr;
    }();

    return function;
}
}

extern "C" HRESULT WINAPI DirectInput8Create(
    HINSTANCE instance,
    DWORD version,
    REFIID interface_id,
    LPVOID* output,
    LPUNKNOWN outer)
{
    const auto real = load_real_direct_input();
    if (!real)
    {
        return DIERR_GENERIC;
    }

    InitOnceExecuteOnce(&install_once, install_fixes, nullptr, nullptr);
    return real(instance, version, interface_id, output, outer);
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(instance);
    }

    return TRUE;
}
