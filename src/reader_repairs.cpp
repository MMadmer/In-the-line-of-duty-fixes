#include "reader_repairs.h"

#include "config_repairs.h"
#include "sha256.h"
#include "x86_detour.h"

#include <Windows.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <span>
#include <string_view>

namespace ild
{
namespace
{
// Both overloads end in the same worker, but the worker takes a calling convention of its own; the two
// exported entry points are ordinary member functions, and between them they carry every caller outside
// xrCore, which is where the configuration files are opened.
using OpenReader = void*(__thiscall*)(void*, const char*);
using OpenInFolder = void*(__thiscall*)(void*, const char*, const char*);
constexpr char open_symbol[] = "?r_open@CLocatorAPI@@QAEPAVIReader@@PBD@Z";
constexpr char open_in_folder_symbol[] = "?r_open@CLocatorAPI@@QAEPAVIReader@@PBD0@Z";

// Every reader the file system hands out starts with these four fields.
struct ReaderPrefix
{
    const void* vtable;
    std::byte* data;
    int position;
    int size;
};
static_assert(offsetof(ReaderPrefix, size) == 12);

constexpr std::string_view core_hash =
    "E6B6E0C150C4C511B299AA3C0E4E91D6B77A4801B23C9B6E55BF7A557ABEEEEB";

// A repaired copy of one archived file, kept for the life of the process: a reader over an archive points
// straight into the read-only mapping, so the copy is what the engine is handed instead.
struct Replacement
{
    std::atomic<std::byte*> patched{};
    std::atomic<int> size{};
    Sha256 original{};
};
constexpr std::size_t replacement_slots = 4;
std::array<Replacement, replacement_slots> replacements{};

X86Detour detour;
X86Detour folder_detour;
std::atomic<OpenReader> original_open{};
std::atomic<OpenInFolder> original_open_in_folder{};
std::atomic<int> applied{};

// A private buffer belongs to the engine and can be rewritten where it stands; a mapped view is the file
// itself and must never be written to.
[[nodiscard]] bool privately_writable(const void* address, std::size_t size)
{
    MEMORY_BASIC_INFORMATION information{};
    if (!VirtualQuery(address, &information, sizeof information)) return false;
    if (information.Type != MEM_PRIVATE || information.State != MEM_COMMIT) return false;
    constexpr DWORD writable = PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
    const auto end = static_cast<const std::byte*>(address) + size;
    return (information.Protect & writable) != 0 &&
        end <= static_cast<const std::byte*>(information.BaseAddress) + information.RegionSize;
}

// Hand the reader a repaired copy without touching the archive it came from. Only a reader that owns no
// memory of its own is repointed: for an entry stored uncompressed the engine returns a plain reader over
// the archive mapping, which never frees or unmaps what it was given.
[[nodiscard]] bool substitute(ReaderPrefix& prefix)
{
    const auto size = static_cast<std::size_t>(prefix.size);
    Sha256 digest{};
    if (!sha256_bytes(std::span(prefix.data, size), digest)) return false;

    for (auto& slot : replacements)
    {
        if (slot.size.load(std::memory_order_acquire) != prefix.size) continue;
        const auto ready = slot.patched.load(std::memory_order_acquire);
        if (ready && slot.original == digest)
        {
            prefix.data = ready;
            return true;
        }
    }

    auto copy = static_cast<std::byte*>(::operator new(size, std::nothrow));
    if (!copy) return false;
    std::memcpy(copy, prefix.data, size);
    if (!repair_config_buffer(std::span(copy, size)))
    {
        ::operator delete(copy);
        return false;
    }
    for (auto& slot : replacements)
    {
        int free_slot = 0;
        if (!slot.size.compare_exchange_strong(free_slot, prefix.size, std::memory_order_acq_rel)) continue;
        slot.original = digest;
        slot.patched.store(copy, std::memory_order_release);
        prefix.data = copy;
        return true;
    }
    // Every slot is taken; the copy is still correct for this one reader, so it is used and then dropped.
    ::operator delete(copy);
    return false;
}

void repair_reader(void* reader)
{
    if (!reader) return;
    auto& prefix = *static_cast<ReaderPrefix*>(reader);
    if (!prefix.data || prefix.size <= 0 || !is_config_repair_size(static_cast<std::size_t>(prefix.size)))
        return;
    const auto size = static_cast<std::size_t>(prefix.size);
    // The size is only a filter; what decides is the digest of the contents, inside the repair itself.
    const auto repaired = privately_writable(prefix.data, size)
        ? repair_config_buffer(std::span(prefix.data, size))
        : substitute(prefix);
    if (repaired) applied.fetch_add(1, std::memory_order_relaxed);
}

void* __fastcall open_with_config_repairs(void* filesystem, void*, const char* name)
{
    const auto reader = original_open.load(std::memory_order_acquire)(filesystem, name);
    repair_reader(reader);
    return reader;
}

void* __fastcall open_in_folder_with_config_repairs(void* filesystem, void*, const char* path, const char* name)
{
    const auto reader = original_open_in_folder.load(std::memory_order_acquire)(filesystem, path, name);
    repair_reader(reader);
    return reader;
}
}

bool install_reader_repairs(const std::filesystem::path& root)
{
    if (original_open.load(std::memory_order_acquire)) return true;
    const auto core = GetModuleHandleW(L"xrCore.dll");
    if (!core) return false;
    Sha256 digest{};
    if (!sha256_file(root / L"bin" / L"xrCore.dll", digest)) return false;
    constexpr char digits[] = "0123456789ABCDEF";
    std::array<char, 64> identity{};
    for (std::size_t index = 0; index < digest.size(); ++index)
    {
        const auto value = std::to_integer<unsigned>(digest[index]);
        identity[index * 2] = digits[value >> 4];
        identity[index * 2 + 1] = digits[value & 15];
    }
    if (std::string_view(identity.data(), identity.size()) != core_hash) return false;
    const auto open = reinterpret_cast<void*>(GetProcAddress(core, open_symbol));
    const auto open_in_folder = reinterpret_cast<void*>(GetProcAddress(core, open_in_folder_symbol));
    if (!open || !open_in_folder) return false;
    // Whole-instruction prologues, pinned to the validated xrCore.
    constexpr std::array<unsigned char, 6> prologue{0x8B, 0x54, 0x24, 0x04, 0x8B, 0xC1};
    constexpr std::array<unsigned char, 7> folder_prologue{0x8B, 0xC1, 0x51, 0x8B, 0x4C, 0x24, 0x08};
    if (!detour.prepare(open, reinterpret_cast<void*>(&open_with_config_repairs), prologue) ||
        !folder_detour.prepare(open_in_folder, reinterpret_cast<void*>(&open_in_folder_with_config_repairs),
            folder_prologue))
        return false;
    original_open.store(reinterpret_cast<OpenReader>(detour.original()), std::memory_order_release);
    original_open_in_folder.store(reinterpret_cast<OpenInFolder>(folder_detour.original()),
        std::memory_order_release);
    if (detour.enable() && folder_detour.enable()) return true;
    detour.disable();
    folder_detour.disable();
    original_open.store(nullptr, std::memory_order_release);
    original_open_in_folder.store(nullptr, std::memory_order_release);
    return false;
}

#ifdef ILD_CONSOLE_QA
int reader_repairs_applied()
{
    return applied.load(std::memory_order_relaxed);
}
#endif
}
