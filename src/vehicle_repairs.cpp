#include "vehicle_repairs.h"
#include "x86_detour.h"

#include <Windows.h>

#include <array>
#include <cstring>

namespace ild
{
// A client object runs its per-frame update only while it is a "crow": drawn on screen, within 30 m of the camera,
// or asking to be one through AlwaysTheCrow, which a car does only while it carries an active weapon. That update
// is the one place a car's physics is copied into the position scripts read. Stock ph_car was written for armed
// vehicles and parks a car for good once that position has not moved for a second, so an unarmed rally truck the
// player is not looking at is declared stuck and stopped while it is physically driving. Every car now asks to
// stay a crow; a parked car's update does nearly nothing.
VehicleRepair install_vehicle_updates()
{
    const auto module = GetModuleHandleW(L"xrGame.dll");
    if (!module) return VehicleRepair::pending;
    // CCar::AlwaysTheCrow: return m_car_weapon && m_car_weapon->IsActive(). Position independent, whole function.
    constexpr std::array<unsigned char, 28> original{
        0x8B, 0x81, 0x10, 0x05, 0x00, 0x00, 0x85, 0xC0, 0x74, 0x0F, 0x80, 0xB8, 0xCC, 0x00,
        0x00, 0x00, 0x00, 0x74, 0x06, 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3, 0x33, 0xC0, 0xC3};
    // mov eax, 1; ret - it replaces exactly the function's first instruction.
    constexpr std::array<unsigned char, 6> always{0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3};
    const auto target = reinterpret_cast<unsigned char*>(module) + 0x269020;
    if (code_matches(target, always) && code_matches(target + always.size(),
            std::span(original).subspan(always.size()))) return VehicleRepair::applied;
    if (!code_matches(target, original)) return VehicleRepair::skipped;
    DWORD protection{};
    if (!VirtualProtect(target, always.size(), PAGE_EXECUTE_READWRITE, &protection)) return VehicleRepair::failed;
    std::memcpy(target, always.data(), always.size());
    FlushInstructionCache(GetCurrentProcess(), target, always.size());
    DWORD ignored{};
    VirtualProtect(target, always.size(), protection, &ignored);
    return VehicleRepair::applied;
}
}
