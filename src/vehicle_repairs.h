#pragma once

namespace ild
{
enum class VehicleRepair { pending, applied, skipped, failed };

// Keeps every car in the engine's per-frame update, as an armed one always was. Pending until xrGame is loaded.
[[nodiscard]] VehicleRepair install_vehicle_updates();
}
