#define DIRECTINPUT_VERSION 0x0800

#include "iat_hook.h"
#include "command_line.h"
#include "console_hooks.h"
#include "update_bridge.h"
#include "inventory_hooks.h"
#include "startup_repairs.h"
#include "config_repairs.h"
#include "texture_aliases.h"
#include "script_patch.h"
#include "display_mode.h"
#include "reader_repairs.h"
#include "vehicle_repairs.h"
#include "sha256.h"

#include <Windows.h>
#include <dinput.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace
{
using DirectInput8CreateFn = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
using CreateFileMappingAFn = decltype(&CreateFileMappingA);
using ReadFn = int(__cdecl*)(int, void*, unsigned int);
using OsHandleFn = std::intptr_t(__cdecl*)(int);

// The builds the native tweaks were validated against. They are recorded for support only: nothing below is
// enabled or disabled by these digests, every repair checks what it needs by itself.
struct Identity
{
    const wchar_t* relative;
    std::string_view hash;
};
constexpr std::array identities{
    Identity{L"bin\\XR_3DA.exe", "B22BC15B94A2A58C4E7046E46D46A3750D80C399BA8F37A2EF40CCF78EE3126D"},
    Identity{L"bin\\xrCore.dll", "E6B6E0C150C4C511B299AA3C0E4E91D6B77A4801B23C9B6E55BF7A557ABEEEEB"},
    Identity{L"bin\\xrGame.dll", "277B67FD6D21839A2F6C246EF57C8AD0C31079C0EAAAB179A8072D1B74A0284F"},
    Identity{L"bin\\xrSound.dll", "741FE39CDB2081CADB7CAEE33C111C60BE7EE1248F01FFB6B8F550AF50BCEFEA"},
    Identity{L"bin\\xrRender_R2.dll", "2A91C9BB90A4CBF8A3E0F9265634A7F38ED19662B5B10089149FD1E7B2942F86"},
    Identity{L"gamedata\\scripts\\_g.script", "2C5C2CCD95AE5B91F58C988D777C21444B832B746AFE3B565DF9A0E42F7AF2EE"},
    Identity{L"gamedata\\scripts\\bind_stalker.script",
        "34732168AF8F7941A8BC87B7481A8A8686B447C27C25A914A11986D423B5C5B9"},
    Identity{L"gamedata\\scripts\\ui_main_menu.script",
        "F18503040ED2FBBB84161857C0B55C84E8101CC911868978217A8E2C11577E08"},
};
enum IdentityIndex : std::size_t { engine_identity, core_identity, game_identity, sound_identity };

CreateFileMappingAFn real_create_file_mapping{};
ReadFn real_read{};
OsHandleFn crt_handle{};
std::filesystem::path game_root;
std::wstring target_script;
std::wstring target_actor;
std::wstring target_menu;
std::filesystem::path resolved_root;
std::atomic<bool> bridge_installed{};
INIT_ONCE install_once = INIT_ONCE_STATIC_INIT;

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

void start_update_service(const std::filesystem::path& engine)
{
    const auto root = engine.parent_path().parent_path();
    const auto updater = root / L"InTheLineOfDutyFixesUpdater.exe";
    // The restart must reproduce the original command line, including X-Ray's space-terminated switches.
    const auto tail = ild::command_line_tail(GetCommandLineW());
    const auto qa = ild::has_switch(tail, L"-qa_update");
    std::wstring command = quote_argument(updater.wstring()) + L" --service --game-dir " +
        quote_argument(root.wstring()) + L" --game-pid " + std::to_wstring(GetCurrentProcessId()) +
        L" --restart-exe " + quote_argument(engine.wstring()) + L" --restart-args " + quote_argument(std::wstring(tail));
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

// std::filesystem::path::string() throws for a name the ANSI code page cannot hold, which would take the game
// down inside DirectInput8Create; the report is UTF-8 instead.
[[nodiscard]] std::string utf8(std::wstring_view text)
{
    if (text.empty()) return {};
    const auto size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0,
        nullptr, nullptr);
    if (size <= 0) return {};
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), size, nullptr, nullptr);
    return result;
}

[[nodiscard]] std::string hex(const ild::Sha256& digest)
{
    constexpr char digits[] = "0123456789ABCDEF";
    std::string text;
    for (const auto byte : digest)
    {
        const auto value = std::to_integer<unsigned>(byte);
        text += digits[value >> 4];
        text += digits[value & 15];
    }
    return text;
}

// A support report the player can send back. It is written on every launch, before and independently of any
// repair, and the scripts the pack binds later add what actually happened to them, so an installation where
// something did not apply still says exactly why.
std::mutex report_mutex;
std::filesystem::path report_root;
std::string report_head;
std::vector<std::string> report_runtime;

void write_report_locked()
{
    // A script loaded before the loader has finished is kept and written together with the rest.
    if (report_root.empty()) return;
    std::error_code error;
    const auto directory = report_root / L".ild-fixes" / L"runtime";
    std::filesystem::create_directories(directory, error);
    if (error) return;
    std::string text = report_head + "\n[runtime]\n";
    for (const auto& line : report_runtime) text += line;
    if (report_runtime.empty()) text += "  no script has been loaded yet\n";
    text += "\nSend this file if a repair did not take effect.\n";
    const auto file = CreateFileW((directory / L"loader-report.txt").c_str(), GENERIC_WRITE, FILE_SHARE_READ,
        nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written{};
    static_cast<void>(WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &written, nullptr));
    CloseHandle(file);
}

void note_once(std::atomic<bool>& noted, std::string_view step, std::string_view outcome)
{
    if (noted.exchange(true, std::memory_order_acq_rel)) return;
    const std::lock_guard lock(report_mutex);
    report_runtime.push_back("  " + std::string(step) + " = " + std::string(outcome) + "\n");
    write_report_locked();
}

std::atomic<bool> console_noted{}, abort_noted{}, actor_noted{}, menu_noted{}, vehicle_noted{};
std::atomic<bool> vehicle_pending{};

[[nodiscard]] std::string_view vehicle_outcome(ild::VehicleRepair result)
{
    return result == ild::VehicleRepair::applied ? "ok" : result == ild::VehicleRepair::skipped ?
        "skipped, this build differs from the validated one" : "FAILED";
}

[[nodiscard]] bool record_identity(std::string& report, const Identity& identity)
{
    ild::Sha256 actual{};
    const auto readable = ild::sha256_file(game_root / identity.relative, actual);
    const auto matches = readable && hex(actual) == identity.hash;
    report += "  " + utf8(identity.relative) + "\n    expected " + std::string(identity.hash) + "\n    actual   ";
    report += readable ? hex(actual) : std::string("<file not readable>");
    report += matches ? "  MATCH\n" : "  MISMATCH\n";
    return matches;
}

// Whether a stored option survives a restart depends on the file being there and on the game seeing the real
// folder. The executable asks for no execution level, so in a protected folder UAC virtualizes it: whatever it
// writes lands in the player's VirtualStore, where an elevated session and the updater never look.
void record_settings(std::string& report, const std::filesystem::path& root)
{
    report += "\n[settings]\n";
    DWORD virtualized{};
    HANDLE token{};
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        DWORD length{};
        if (!GetTokenInformation(token, TokenVirtualizationEnabled, &virtualized, sizeof virtualized, &length))
            virtualized = 0;
        CloseHandle(token);
    }
    if (!virtualized)
        report += "  UAC file virtualization = off\n";
    else
    {
        // A token can be eligible without anything being redirected; only an existing copy of the pack's own folder
        // under VirtualStore shows that its files there differ from what an elevated session or the updater sees.
        std::wstring local(32768, L'\0');
        const auto length = GetEnvironmentVariableW(L"LOCALAPPDATA", local.data(), static_cast<DWORD>(local.size()));
        local.resize(length < local.size() ? length : 0);
        const auto store = std::filesystem::path(local) / L"VirtualStore" / root.relative_path() / L".ild-fixes";
        std::error_code error;
        report += !local.empty() && std::filesystem::exists(store, error) ?
            "  UAC file virtualization = on, this folder's writes are redirected to " + utf8(store.wstring()) + "\n" :
            std::string("  UAC file virtualization = on, nothing of this folder is redirected\n");
    }
    const auto file = CreateFileW((root / L".ild-fixes" / L"settings.txt").c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        report += "  no settings file, every option is at its default\n";
        return;
    }
    std::array<char, 4096> data{};
    DWORD read{};
    const auto complete = ReadFile(file, data.data(), static_cast<DWORD>(data.size() - 1), &read, nullptr);
    CloseHandle(file);
    std::string_view text(data.data(), complete ? read : 0);
    while (!text.empty())
    {
        const auto end = text.find('\n');
        auto line = text.substr(0, end);
        text.remove_prefix(end == text.npos ? text.size() : end + 1);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        if (line.find('=') != line.npos && line.size() <= 128) report += "  " + std::string(line) + "\n";
    }
}

// A tweak pinned to one build that finds another one has done its job by leaving the game alone; that is not a
// failure the player needs to hear about, only something support needs to see.
void record(std::string& report, std::string_view step, bool ok, bool validated = true)
{
    report += "  " + std::string(step) + (ok ? " = ok\n" : validated ? " = FAILED\n" :
        " = skipped, this build differs from the validated one\n");
}

[[nodiscard]] std::wstring handle_path(HANDLE handle)
{
    std::wstring buffer(1024, L'\0');
    for (;;)
    {
        const auto length = GetFinalPathNameByHandleW(handle, buffer.data(), static_cast<DWORD>(buffer.size()),
            FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
        if (!length || length > 32768) return {};
        if (length < buffer.size())
        {
            buffer.resize(length);
            break;
        }
        buffer.assign(length + 1, L'\0');
    }
    constexpr std::wstring_view extended_prefix = L"\\\\?\\";
    if (buffer.starts_with(extended_prefix)) buffer.erase(0, extended_prefix.size());
    return buffer;
}

// A game started through a junction, a SUBST drive or a short name reports its executable under that name, while
// every file it opens resolves to the real location, so both sides are compared in resolved form.
[[nodiscard]] std::filesystem::path resolve_directory(const std::filesystem::path& directory)
{
    const auto handle = CreateFileW(directory.c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);
    if (handle == INVALID_HANDLE_VALUE) return directory;
    auto resolved = handle_path(handle);
    CloseHandle(handle);
    return resolved.empty() ? directory : std::filesystem::path(std::move(resolved));
}

// Which repair a file needs. Chosen by path, so a copy of the mod whose bytes differ slightly still gets the
// repair; each transform verifies its own anchor and refuses an unexpected file.
enum class ScriptFile { none, console_script, actor_script, menu_script, config };

[[nodiscard]] ScriptFile classify_handle(HANDLE file)
{
    const auto path = handle_path(file);
    if (path.empty()) return ScriptFile::none;
    if (_wcsicmp(path.c_str(), target_script.c_str()) == 0) return ScriptFile::console_script;
    if (_wcsicmp(path.c_str(), target_actor.c_str()) == 0) return ScriptFile::actor_script;
    if (_wcsicmp(path.c_str(), target_menu.c_str()) == 0) return ScriptFile::menu_script;
    return ild::is_config_repair_path(path, resolved_root) ? ScriptFile::config : ScriptFile::none;
}

// Returns whether the bytes were changed.
[[nodiscard]] bool patch_script(ScriptFile kind, std::span<std::byte> bytes)
{
    using ild::script_patch::Result;
    if (kind == ScriptFile::console_script)
    {
        // Independent of each other: an abort() the pack does not recognise still leaves the spam fix.
        const auto reason = ild::script_patch::reveal_abort_reason(bytes);
        const auto console = ild::script_patch::remove_console_execution(bytes);
        note_once(console_noted, "console spam fix in _g.script", console == Result::applied ? "ok" :
            console == Result::already_applied ? "already present" : "FAILED, the statement was not found");
        note_once(abort_noted, "crash reason in _g.script", reason ? "ok" : "skipped, abort() differs");
        return reason || console == Result::applied;
    }
    if (kind == ScriptFile::actor_script)
    {
        const auto bound = ild::script_patch::bind_gameplay(bytes);
        note_once(actor_noted, "gameplay repairs bound to bind_stalker.script", bound ? "ok" :
            "FAILED, the anchor was not found");
        return bound;
    }
    if (kind == ScriptFile::menu_script)
    {
        // The menu options name console commands of the bridge; binding them without it would hand the engine's
        // option manager a command that does not exist.
        if (!bridge_installed.load(std::memory_order_acquire))
        {
            note_once(menu_noted, "updater and options bound to ui_main_menu.script",
                "skipped, the console bridge is not installed");
            return false;
        }
        const auto bound = ild::script_patch::bind_update_menu(bytes);
        note_once(menu_noted, "updater and options bound to ui_main_menu.script", bound ? "ok" :
            "FAILED, the anchor was not found");
        return bound;
    }
    return kind == ScriptFile::config && ild::repair_config_buffer(bytes);
}

int __cdecl read_hook(int file, void* buffer, unsigned int size)
{
    static_cast<void>(ild::install_inventory_hooks(game_root));
    static_cast<void>(ild::install_texture_aliases(game_root));
    if (vehicle_pending.load(std::memory_order_acquire))
    {
        const auto vehicles = ild::install_vehicle_updates();
        if (vehicles != ild::VehicleRepair::pending && vehicle_pending.exchange(false, std::memory_order_acq_rel))
            note_once(vehicle_noted, "unseen vehicles keep updating", vehicle_outcome(vehicles));
    }
    const auto read = real_read(file, buffer, size);
    if (read <= 0 || !buffer) return read;
    const auto bytes = std::span(static_cast<std::byte*>(buffer), static_cast<std::size_t>(read));
    // Chosen by size and digest, so it needs no path.
    static_cast<void>(ild::repair_config_buffer(bytes));
    const auto handle = crt_handle ? reinterpret_cast<HANDLE>(crt_handle(file)) : INVALID_HANDLE_VALUE;
    LARGE_INTEGER length{};
    // A script anchor can only be trusted in a read that returned the whole file.
    if (handle == INVALID_HANDLE_VALUE || !GetFileSizeEx(handle, &length) || length.QuadPart != read) return read;
    const auto kind = classify_handle(handle);
    if (kind != ScriptFile::config) static_cast<void>(patch_script(kind, bytes));
    return read;
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
    const auto kind = file == INVALID_HANDLE_VALUE ? ScriptFile::none : classify_handle(file);
    if (kind == ScriptFile::none)
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
        patched = patch_script(kind,
            std::span(static_cast<std::byte*>(destination), static_cast<SIZE_T>(size.QuadPart)));
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
    return real_create_file_mapping(file, attributes, protect, maximum_size_high, maximum_size_low, name);
}

BOOL CALLBACK install_fixes(PINIT_ONCE, PVOID, PVOID*)
{
    const auto engine = executable_path();
    if (engine.empty()) return TRUE;

    const auto root = engine.parent_path().parent_path();
    game_root = root;
    ild::load_display_mode(root);
    resolved_root = resolve_directory(root);
    const auto scripts = resolved_root / L"gamedata" / L"scripts";
    target_script = (scripts / L"_g.script").wstring();
    target_actor = (scripts / L"bind_stalker.script").wstring();
    target_menu = (scripts / L"ui_main_menu.script").wstring();

    std::string report = "In the Line of Duty Fixes - loader report\nversion " ILD_VERSION "\ngame root " +
        utf8(root.wstring()) + "\n\n[identity]\n";
    std::array<bool, identities.size()> validated{};
    for (std::size_t index = 0; index < identities.size(); ++index)
        validated[index] = record_identity(report, identities[index]);

    // Only the tweaks that write to fixed addresses need the builds above, and each checks its own patch site
    // before touching anything. Script, config and Lua repairs, the updater and the options it carries work on
    // any build of this engine, including one without the mod's own binaries.
    report += "\n[install]\n";
    const auto executable = GetModuleHandleW(nullptr);
    const auto core_module = GetModuleHandleW(L"xrCore.dll");
    record(report, "keyboard names", ild::install_input_name_fix(executable), validated[engine_identity]);
    record(report, "graphics presets", ild::install_preset_compatibility(executable), validated[engine_identity]);
    record(report, "screen mode", ild::install_display_mode(root));
    record(report, "sound metadata", ild::install_audio_metadata_fix(root), validated[sound_identity]);
    record(report, "console editing", ild::install_console_hooks(executable), validated[engine_identity]);
    // The game DLL is loaded before input, so no car exists yet and nothing can be running the patched code.
    const auto vehicles = ild::install_vehicle_updates();
    if (vehicles == ild::VehicleRepair::pending)
        vehicle_pending.store(true, std::memory_order_release);
    else
        report += "  unseen vehicles keep updating = " + std::string(vehicle_outcome(vehicles)) + "\n";
#ifdef ILD_CONSOLE_QA
    if (ild::has_switch(ild::command_line_tail(GetCommandLineW()), L"-ild_console_qa"))
        ild::run_console_selftest(executable, root / L"console-qa.txt");
#endif
    real_create_file_mapping = reinterpret_cast<CreateFileMappingAFn>(ild::replace_iat_import(
        core_module,
        "KERNEL32.dll",
        "CreateFileMappingA",
        reinterpret_cast<void*>(&create_file_mapping_hook)));
    // This import carries the console-spam fix, the larger script and config repairs and the Lua payload with the
    // quest and NPC repairs. It is resolved by name, so it does not care which build of the engine runs.
    record(report, "script, config and Lua repairs", real_create_file_mapping != nullptr);
    // Files below the engine's mapping threshold never reach the hook above, so the reader is caught as well.
    record(report, "small config repairs", ild::install_reader_repairs(root), validated[core_identity]);

    // The bridge checks the console command layout against the executable's own exports, not its digest.
    const auto payload_present = std::filesystem::exists(root / L"InTheLineOfDutyFixesUpdater.exe") &&
        std::filesystem::exists(root / L"gamedata" / L"scripts" / L"ild_fix_ui.script") &&
        std::filesystem::exists(root / L"gamedata" / L"config" / L"ui" / L"ild_fixes_update.xml");
    const auto bridge = payload_present && ild::install_update_bridge(executable, root);
    bridge_installed.store(bridge, std::memory_order_release);
    record(report, "in-game updater, options and support verbs", bridge);

    // Small files are read straight into a buffer, which is how the menu and the actor script arrive; the knife
    // and texture repairs are also installed from here because their modules load later than this.
    const auto crt = GetModuleHandleW(L"MSVCR80.dll");
    real_read = crt ? reinterpret_cast<ReadFn>(GetProcAddress(crt, "_read")) : nullptr;
    crt_handle = crt ? reinterpret_cast<OsHandleFn>(GetProcAddress(crt, "_get_osfhandle")) : nullptr;
    const auto reader = real_read && ild::replace_iat_import(core_module, "MSVCR80.dll", "_read",
        reinterpret_cast<void*>(&read_hook));
    record(report, "script bindings and late repairs", reader);
    if (bridge && reader) start_update_service(engine);
    record_settings(report, root);

    const std::lock_guard lock(report_mutex);
    report_root = root;
    report_head = std::move(report);
    write_report_locked();
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
    const auto result = real(instance, version, interface_id, output, outer);
    // Both the ANSI and Unicode interfaces share this layout, so the slot patch works for either.
    if (SUCCEEDED(result) && output) ild::hook_direct_input(*output);
    return result;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(instance);
        // Only the executable's import table is rewritten here, so this is safe under the loader lock, and
        // it is the one place guaranteed to run before the render device exists.
        static_cast<void>(ild::install_display_mode_early());
    }

    return TRUE;
}
