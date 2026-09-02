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

bool X86Detour::prepare(void* target, void* replacement, std::span<const unsigned char> expected)
{
    if (!target || !replacement || expected.size() < 5 || expected.size() > saved_.size() ||
        std::memcmp(target, expected.data(), expected.size()) != 0) return false;
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
