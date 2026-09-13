#include "carry_weight.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace
{
float capacity_value = 85.0f;

// Stands in for CInventoryOwner::MaxCarryWeight: a thiscall virtual without stack arguments, returning on the FPU.
float __fastcall max_carry_weight(void*, void*)
{
    return capacity_value;
}

std::array<void*, 40> vtable{};
std::array<unsigned char, 0x400> actor{};
std::array<unsigned char, 0x200> condition{};
std::array<unsigned char, 0x80> inventory{};
std::array<unsigned char, 0x80> stranger{};

// What the step is entered with and what it leaves behind, kept where the naked driver can address them directly.
void* condition_address{};
void* inventory_address{};
float weight{};
float sentinels[8]{};
float results[8]{};
unsigned registers_after[7]{};
void* saved_esp{};
void* esp_after{};
unsigned char took_original{};
void* entry{};
void** resume_slot{};
void** original_slot{};

// Enters the step the way the engine does - a jump with every register holding something - and records every
// register when the step leaves through either exit.
__declspec(naked) void run_step()
{
    __asm {
        push ebp
        push ebx
        push esi
        push edi
        mov saved_esp, esp
        mov byte ptr [took_original], 0
        mov eax, offset resumed
        mov ecx, resume_slot
        mov [ecx], eax
        mov eax, offset fell_back
        mov ecx, original_slot
        mov [ecx], eax
        movss xmm0, dword ptr [sentinels]
        movss xmm1, dword ptr [sentinels + 4]
        movss xmm2, dword ptr [sentinels + 8]
        movss xmm3, dword ptr [sentinels + 12]
        movss xmm5, dword ptr [sentinels + 20]
        movss xmm6, dword ptr [sentinels + 24]
        movss xmm7, dword ptr [sentinels + 28]
        movss xmm4, dword ptr [weight]
        mov ebx, condition_address
        mov esi, inventory_address
        mov eax, 0x11111111
        mov ecx, 0x22222222
        mov edx, 0x33333333
        mov edi, 0x44444444
        mov ebp, 0x55555555
        jmp dword ptr [entry]
    fell_back:
        mov byte ptr [took_original], 1
    resumed:
        mov esp_after, esp
        mov dword ptr [registers_after], eax
        mov dword ptr [registers_after + 4], ecx
        mov dword ptr [registers_after + 8], edx
        mov dword ptr [registers_after + 12], ebx
        mov dword ptr [registers_after + 16], esi
        mov dword ptr [registers_after + 20], edi
        mov dword ptr [registers_after + 24], ebp
        movss dword ptr [results], xmm0
        movss dword ptr [results + 4], xmm1
        movss dword ptr [results + 8], xmm2
        movss dword ptr [results + 12], xmm3
        movss dword ptr [results + 16], xmm4
        movss dword ptr [results + 20], xmm5
        movss dword ptr [results + 24], xmm6
        movss dword ptr [results + 28], xmm7
        mov esp, saved_esp
        pop edi
        pop esi
        pop ebx
        pop ebp
        ret
    }
}

// Everything but xmm4 must come back exactly as it went in.
[[nodiscard]] bool registers_kept()
{
    const std::array<unsigned, 7> expected{0x11111111, 0x22222222, 0x33333333,
        static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(condition_address)),
        static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(inventory_address)), 0x44444444, 0x55555555};
    for (std::size_t index = 0; index < expected.size(); ++index)
        if (registers_after[index] != expected[index]) return false;
    for (std::size_t index = 0; index < 8; ++index)
        if (index != 4 && results[index] != sentinels[index]) return false;
    return esp_after == saved_esp;
}
}

int main()
{
    vtable[38] = reinterpret_cast<void*>(&max_carry_weight);
    const auto table = vtable.data();
    const auto held = inventory.data();
    const auto owner = actor.data();
    std::memcpy(actor.data() + 0x274, &table, sizeof table);
    std::memcpy(actor.data() + 0x298, &held, sizeof held);
    std::memcpy(condition.data() + 0xF4, &owner, sizeof owner);
    const auto step = ild::walk_step_for_tests();
    entry = step.entry;
    resume_slot = step.resume;
    original_slot = step.original;
    for (std::size_t index = 0; index < 8; ++index) sentinels[index] = 100.0f + static_cast<float>(index);
    condition_address = condition.data();
    inventory_address = inventory.data();

    // 51 kg in a suit that carries 85: the share is measured against 85, not against the bare 60.
    weight = 51.0f;
    run_step();
    if (took_original) return 1;
    if (std::fabs(results[4] - 51.0f / 85.0f) > 1e-6f) return 2;
    if (!registers_kept()) return 3;

    // No usable capacity: the original division runs instead, with nothing else touched.
    capacity_value = 0.0f;
    run_step();
    if (!took_original || results[4] != 51.0f || !registers_kept()) return 4;
    capacity_value = std::nanf("");
    run_step();
    if (!took_original || !registers_kept()) return 5;

    // An inventory the actor does not hold is not the call site this was written for.
    capacity_value = 85.0f;
    inventory_address = stranger.data();
    run_step();
    if (!took_original || results[4] != 51.0f || !registers_kept()) return 6;
    return 0;
}
