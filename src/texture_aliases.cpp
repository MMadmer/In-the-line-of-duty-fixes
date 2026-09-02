#include "texture_aliases.h"
#include "iat_hook.h"
#include "sha256.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <span>
#include <string>

namespace ild
{
namespace
{
// The imported member returns const CLocatorAPI::file*, not a Boolean or an owning reader.
using Exists = const void*(__thiscall*)(void*, char*, const char*, const char*, const char*);
using OpenReader = void*(__thiscall*)(void*, const char*);
constexpr char exists_symbol[] = "?exist@CLocatorAPI@@QAEPBUfile@1@AAY0CAI@DPBD11@Z";
constexpr char open_symbol[] = "?r_open@CLocatorAPI@@QAEPAVIReader@@PBD@Z";
constexpr std::size_t exists_iat_rva = 0x695F8;
constexpr std::size_t open_iat_rva = 0x69688;
std::atomic<Exists> original_exists{};
std::atomic<OpenReader> original_open{};
std::string tank_path;
const void* virtual_reader_vtable{};
enum class InstallState { pending, checking, enabled, disabled };
std::atomic state{InstallState::pending};

struct ReaderPrefix
{
    const void* vtable;
    const std::byte* data;
    int position;
    int size;
};
static_assert(offsetof(ReaderPrefix, size) == 12);

bool matches_digest(const Sha256& digest, std::string_view expected)
{
    constexpr char digits[] = "0123456789ABCDEF";
    std::array<char, 64> text{};
    for (std::size_t index = 0; index < digest.size(); ++index)
    {
        const auto byte = std::to_integer<unsigned>(digest[index]);
        text[index * 2] = digits[byte >> 4];
        text[index * 2 + 1] = digits[byte & 15];
    }
    return std::string_view(text.data(), text.size()) == expected;
}

bool matches_hash(const std::filesystem::path& path, std::string_view expected)
{
    Sha256 digest{};
    return sha256_file(path, digest) && matches_digest(digest, expected);
}

bool matches_module(HMODULE module, const std::filesystem::path& expected_path, std::string_view expected_hash)
{
    std::array<wchar_t, 32768> path{};
    const auto length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
    return length && length < path.size() && _wcsicmp(path.data(), expected_path.c_str()) == 0 &&
        matches_hash(expected_path, expected_hash);
}

const void* __fastcall exists_with_bump_alias(
    void* filesystem, void*, char* destination, const char* path, const char* name, const char* extension)
{
    const auto original = original_exists.load(std::memory_order_acquire);
    const auto found = original(filesystem, destination, path, name, extension);
    if (found || !path || !name || !extension) return found;
    const auto fallback = texture_aliases::missing_bump_fallback(path, name, extension);
    if (fallback.empty()) return found;

    // These are the exact archive textures selected by R2's missing-bump branch, without altering material flags.
    return original(filesystem, destination, path, fallback.data(), extension);
}

void* __fastcall open_with_model_boundary(void* filesystem, void*, const char* name)
{
    const auto reader = original_open.load(std::memory_order_acquire)(filesystem, name);
    if (!reader || !name || _stricmp(name, tank_path.c_str()) != 0) return reader;
    auto& prefix = *static_cast<ReaderPrefix*>(reader);
    constexpr int damaged_length = 2096476;
    if (prefix.vtable != virtual_reader_vtable || !prefix.data || prefix.position || prefix.size != damaged_length)
        return reader;
    Sha256 digest{};
    if (!sha256_bytes(std::span(prefix.data, static_cast<std::size_t>(prefix.size)), digest) ||
        !matches_digest(digest, "095F737927C49C525662F306B59DE11D049A42F60A20F72DBFF69E342B600C40"))
        return reader;

    // The verified chunk stream ends before one stray zero; the mapped backing and destructor handles stay intact.
    prefix.size = damaged_length - 1;
    return reader;
}

bool disable(const char* reason)
{
    OutputDebugStringA(reason);
    state.store(InstallState::disabled, std::memory_order_release);
    return false;
}
}

bool install_texture_aliases(const std::filesystem::path& root)
{
    auto status = state.load(std::memory_order_acquire);
    if (status != InstallState::pending) return status == InstallState::enabled;
    const auto renderer = GetModuleHandleW(L"xrRender_R2.dll");
    const auto core = GetModuleHandleW(L"xrCore.dll");
    if (!renderer || !core) return false;
    if (!state.compare_exchange_strong(status, InstallState::checking, std::memory_order_acq_rel))
        return status == InstallState::enabled;

    if (!matches_module(renderer, root / L"bin" / L"xrRender_R2.dll",
            "2A91C9BB90A4CBF8A3E0F9265634A7F38ED19662B5B10089149FD1E7B2942F86") ||
        !matches_module(core, root / L"bin" / L"xrCore.dll",
            "E6B6E0C150C4C511B299AA3C0E4E91D6B77A4801B23C9B6E55BF7A557ABEEEEB"))
        return disable("In the Line of Duty Fixes: texture aliases disabled; unsupported renderer/core identity.\n");

    const auto original = reinterpret_cast<Exists>(GetProcAddress(core, exists_symbol));
    const auto open = reinterpret_cast<OpenReader>(GetProcAddress(core, open_symbol));
    const auto slot = reinterpret_cast<Exists*>(reinterpret_cast<std::byte*>(renderer) + exists_iat_rva);
    const auto open_slot = reinterpret_cast<OpenReader*>(reinterpret_cast<std::byte*>(renderer) + open_iat_rva);
    if (!original || !open || *slot != original || *open_slot != open)
        return disable("In the Line of Duty Fixes: texture aliases disabled; unexpected renderer FS import.\n");

    // Publish the callable original before exposing the IAT hook to renderer threads.
    original_exists.store(original, std::memory_order_release);
    original_open.store(open, std::memory_order_release);
    tank_path = (root / L"gamedata" / L"meshes" / L"physics" / L"vehicles" / L"tank_belui.ogf").string();
    virtual_reader_vtable = reinterpret_cast<std::byte*>(core) + 0x3414C;
    const auto replaced = replace_iat_import(renderer, "xrCore.dll", exists_symbol,
        reinterpret_cast<void*>(&exists_with_bump_alias));
    if (!replaced)
        return disable("In the Line of Duty Fixes: texture aliases disabled; renderer FS import is not writable.\n");
    if (!replace_iat_import(renderer, "xrCore.dll", open_symbol, reinterpret_cast<void*>(&open_with_model_boundary)))
    {
        static_cast<void>(replace_iat_import(renderer, "xrCore.dll", exists_symbol, replaced));
        return disable("In the Line of Duty Fixes: model boundary repair disabled; reader import is not writable.\n");
    }
    state.store(InstallState::enabled, std::memory_order_release);
    return true;
}

#ifdef ILD_CONSOLE_QA
bool verify_texture_repairs()
{
    if (state.load() != InstallState::enabled) return false;
    const auto core = GetModuleHandleW(L"xrCore.dll");
    const auto filesystem = reinterpret_cast<void**>(GetProcAddress(core, "?xr_FS@@3PAVCLocatorAPI@@A"));
    using Close = void(__thiscall*)(void*, void**);
    const auto close = reinterpret_cast<Close>(GetProcAddress(core, "?r_close@CLocatorAPI@@QAEXAAPAVIReader@@@Z"));
    if (!filesystem || !*filesystem || !close) return false;
    constexpr std::array names{"prop\\prop_blanket_bump", "wood\\wood_board_02_bump", "wood\\wood_board_02_bump#",
        "wood\\wood_collect_bump", "wood\\wood_collect_bump#"};
    for (const auto name : names)
    {
        std::array<char, 520> actual{}, expected{};
        const auto fallback = texture_aliases::missing_bump_fallback("$game_textures$", name, ".dds");
        const auto target = original_exists.load()(
            *filesystem, expected.data(), "$game_textures$", fallback.data(), ".dds");
        if (!target || exists_with_bump_alias(
                *filesystem, nullptr, actual.data(), "$game_textures$", name, ".dds") != target)
            return false;
    }
    auto original = original_open.load()(*filesystem, tank_path.c_str());
    auto fixed = open_with_model_boundary(*filesystem, nullptr, tank_path.c_str());
    bool valid{};
    if (original && fixed)
    {
        const auto& before = *static_cast<ReaderPrefix*>(original);
        const auto& after = *static_cast<ReaderPrefix*>(fixed);
        valid = before.size == 2096476 && after.size == 2096475 &&
            std::memcmp(before.data, after.data, static_cast<std::size_t>(before.size)) == 0;
    }
    if (original) close(*filesystem, &original);
    if (fixed) close(*filesystem, &fixed);
    return valid;
}
#endif
}
