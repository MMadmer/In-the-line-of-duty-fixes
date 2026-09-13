#include "carry_weight.h"
#include "x86_detour.h"

#include <Windows.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <mutex>
#include <span>

namespace ild
{
namespace
{
X86Detour walk_hook;
bool walk_installed{};
void* walk_original{};
void* walk_resume{};

template<class T> T& field(void* object, std::size_t offset)
{
    return *reinterpret_cast<T*>(static_cast<unsigned char*>(object) + offset);
}

// The capacity the inventory window prints: the stored maximum plus what the worn suit adds. It is the owner's own
// virtual, the one the jump and the fatigue step of the same update already divide by.
bool __stdcall carry_capacity(void* condition, void* inventory, float* capacity)
{
    const auto actor = field<void*>(condition, 0xF4);
    // The walking step has just read the inventory through this actor; anything else is not the site this knows.
    if (!actor || field<void*>(actor, 0x298) != inventory) return false;
    const auto owner = static_cast<unsigned char*>(actor) + 0x274;
    using MaxCarryWeight = float(__thiscall*)(void*);
    const auto value = reinterpret_cast<MaxCarryWeight>(field<void**>(owner, 0)[38])(owner);
    if (!std::isfinite(value) || value <= 0.0f) return false;
    *capacity = value;
    return true;
}

// Entered in place of `divss xmm4, [esi+68h]`, with the rucksack weight in xmm4, the inventory in esi and the
// condition in ebx. Only xmm4 may change, so every other register is put back on either way out; without a
// capacity the original division runs from the trampoline.
__declspec(naked) void walk_ratio()
{
    __asm {
        pushad
        sub esp, 132
        movups [esp], xmm0
        movups [esp + 16], xmm1
        movups [esp + 32], xmm2
        movups [esp + 48], xmm3
        movups [esp + 64], xmm4
        movups [esp + 80], xmm5
        movups [esp + 96], xmm6
        movups [esp + 112], xmm7
        lea eax, [esp + 128]
        push eax
        push esi
        push ebx
        call carry_capacity
        test al, al
        jz original
        movss xmm4, dword ptr [esp + 64]
        divss xmm4, dword ptr [esp + 128]
        movss dword ptr [esp + 64], xmm4
        movups xmm0, [esp]
        movups xmm1, [esp + 16]
        movups xmm2, [esp + 32]
        movups xmm3, [esp + 48]
        movups xmm4, [esp + 64]
        movups xmm5, [esp + 80]
        movups xmm6, [esp + 96]
        movups xmm7, [esp + 112]
        add esp, 132
        popad
        jmp dword ptr [walk_resume]
    original:
        movups xmm0, [esp]
        movups xmm1, [esp + 16]
        movups xmm2, [esp + 32]
        movups xmm3, [esp + 48]
        movups xmm4, [esp + 64]
        movups xmm5, [esp + 80]
        movups xmm6, [esp + 96]
        movups xmm7, [esp + 112]
        add esp, 132
        popad
        jmp dword ptr [walk_original]
    }
}
}

// Shadow of Chernobyl drains stamina for the weight carried as a share of the limit, and five times faster past it.
// The jump measures that share against the capacity the inventory shows, suit included; walking measures it against
// the bare inventory maximum, so a suit that lets the player carry 85 kg still costs overweight stamina from the
// 60th. Walking is pointed at the capacity the jump already uses.
PinnedRepair install_walk_weight()
{
    static std::mutex mutex;
    const std::lock_guard lock(mutex);
    if (walk_installed) return PinnedRepair::applied;
    const auto module = GetModuleHandleW(L"xrGame.dll");
    if (!module) return PinnedRepair::pending;
    const auto base = reinterpret_cast<unsigned char*>(module);
    // CActorCondition::UpdateCondition, the moving branch from its state test to the ConditionWalk call: the weight
    // over the inventory's own maximum. Position independent.
    constexpr std::array<unsigned char, 63> walk{
        0x8B, 0x88, 0x98, 0x05, 0x00, 0x00, 0xF6, 0xC1, 0x0F, 0x74, 0x39, 0x8B, 0xB0, 0x98, 0x02, 0x00,
        0x00, 0x0F, 0xB6, 0x80, 0xC8, 0x05, 0x00, 0x00, 0x8B, 0xD1, 0xC1, 0xEA, 0x0C, 0x81, 0xE2, 0x01,
        0xFF, 0xFF, 0xFF, 0x52, 0x50, 0xE8, 0x85, 0x34, 0xFF, 0xFF, 0xF3, 0x0F, 0x10, 0x66, 0x6C, 0xF3,
        0x0F, 0x5E, 0x66, 0x68, 0x83, 0xC4, 0x04, 0x8B, 0xF3, 0x50, 0xE8, 0x00, 0x05, 0x00, 0x00};
    // CActor::g_cl_CheckControls, the jump: the same weight over the owner's virtual at 0x98. This proves the slot the
    // walking step is pointed at is the capacity this build divides by.
    constexpr std::array<unsigned char, 58> jump{
        0x8B, 0x8F, 0x98, 0x02, 0x00, 0x00, 0xD9, 0x41, 0x6C, 0x8B, 0x97, 0x74, 0x02, 0x00, 0x00, 0x8B,
        0x82, 0x98, 0x00, 0x00, 0x00, 0xD9, 0x5C, 0x24, 0x1C, 0x8B, 0xB7, 0x3C, 0x09, 0x00, 0x00, 0x8D,
        0x8F, 0x74, 0x02, 0x00, 0x00, 0xFF, 0xD0, 0xD8, 0x7C, 0x24, 0x1C, 0xD9, 0x5C, 0x24, 0x1C, 0xF3,
        0x0F, 0x10, 0x5C, 0x24, 0x1C, 0xE8, 0xC0, 0xE0, 0x00, 0x00};
    constexpr std::size_t division = 47;
    const auto start = base + 0x1DD0C1;
    if (!code_matches(start, walk) || !code_matches(base + 0x1CF456, jump)) return PinnedRepair::skipped;
    const auto site = start + division;
    if (!walk_hook.prepare(site, reinterpret_cast<void*>(&walk_ratio), std::span(walk).subspan(division, 5)))
        return PinnedRepair::failed;
    walk_original = walk_hook.original();
    walk_resume = site + 5;
    if (!walk_hook.enable()) return PinnedRepair::failed;
    walk_installed = true;
    return PinnedRepair::applied;
}

#ifdef ILD_CARRY_WEIGHT_TESTS
WalkStepForTests walk_step_for_tests()
{
    return {reinterpret_cast<void*>(&walk_ratio), &walk_resume, &walk_original};
}
#endif
}
