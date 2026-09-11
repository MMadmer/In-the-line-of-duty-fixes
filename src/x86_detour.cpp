#include "x86_detour.h"

#include <cstdint>
#include <cstring>

namespace ild
{
namespace
{
void write_jump(unsigned char* source, const void* target)
{
    source[0] = 0xE9;
    const auto displacement = reinterpret_cast<std::uintptr_t>(target) -
        reinterpret_cast<std::uintptr_t>(source) - 5;
    std::memcpy(source + 1, &displacement, 4);
}
}

bool code_matches(const void* address, std::span<const unsigned char> expected)
{
    constexpr DWORD readable = PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY | PAGE_READONLY |
        PAGE_READWRITE | PAGE_WRITECOPY;
    auto cursor = static_cast<const unsigned char*>(address);
    const auto end = cursor + expected.size();
    if (!address) return false;
    while (cursor < end)
    {
        MEMORY_BASIC_INFORMATION region{};
        if (!VirtualQuery(cursor, &region, sizeof region) || region.State != MEM_COMMIT ||
            !(region.Protect & readable) || (region.Protect & PAGE_GUARD)) return false;
        cursor = static_cast<const unsigned char*>(region.BaseAddress) + region.RegionSize;
    }
    return std::memcmp(address, expected.data(), expected.size()) == 0;
}

bool X86Detour::prepare(void* target, void* replacement, std::span<const unsigned char> expected)
{
    if (!target || !replacement || expected.size() < 5 || expected.size() > saved_.size() ||
        !code_matches(target, expected)) return false;
    target_ = target;
    replacement_ = replacement;
    length_ = expected.size();
    std::memcpy(saved_.data(), target, length_);
    trampoline_ = VirtualAlloc(nullptr, length_ + 5, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!trampoline_) return false;
    std::memcpy(trampoline_, target, length_);
    write_jump(static_cast<unsigned char*>(trampoline_) + length_, static_cast<unsigned char*>(target) + length_);
    DWORD previous{};
    if (!VirtualProtect(trampoline_, length_ + 5, PAGE_EXECUTE_READ, &previous)) return false;
    FlushInstructionCache(GetCurrentProcess(), trampoline_, length_ + 5);
    return true;
}

bool X86Detour::enable()
{
    if (!trampoline_ || enabled_) return false;
    DWORD previous{};
    if (!VirtualProtect(target_, length_, PAGE_EXECUTE_READWRITE, &previous)) return false;
    std::memset(target_, 0x90, length_);
    write_jump(static_cast<unsigned char*>(target_), replacement_);
    FlushInstructionCache(GetCurrentProcess(), target_, length_);
    DWORD ignored{};
    VirtualProtect(target_, length_, previous, &ignored);
    enabled_ = true;
    return true;
}

void X86Detour::disable()
{
    if (!enabled_) return;
    DWORD previous{};
    if (!VirtualProtect(target_, length_, PAGE_EXECUTE_READWRITE, &previous)) return;
    std::memcpy(target_, saved_.data(), length_);
    FlushInstructionCache(GetCurrentProcess(), target_, length_);
    DWORD ignored{};
    VirtualProtect(target_, length_, previous, &ignored);
    enabled_ = false;
}
}
