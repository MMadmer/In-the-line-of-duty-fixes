#pragma once

#include <Windows.h>
#include <array>
#include <cstddef>
#include <span>

namespace ild
{
// True when the bytes at the address are committed, readable and equal to the expected ones. A fixed address in a
// build that is smaller or laid out differently is never read past what is mapped there.
[[nodiscard]] bool code_matches(const void* address, std::span<const unsigned char> expected);

class X86Detour
{
public:
    [[nodiscard]] bool prepare(void* target, void* replacement, std::span<const unsigned char> expected);
    [[nodiscard]] bool enable();
    void disable();
    [[nodiscard]] void* original() const { return trampoline_; }

private:
    void* target_{};
    void* replacement_{};
    void* trampoline_{};
    std::array<unsigned char, 16> saved_{};
    std::size_t length_{};
    bool enabled_{};
};
}
