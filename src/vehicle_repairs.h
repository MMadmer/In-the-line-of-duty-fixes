#pragma once

#include "pinned_repair.h"

namespace ild
{
// Keeps every car in the engine's per-frame update, as an armed one always was. Pending until xrGame is loaded.
[[nodiscard]] PinnedRepair install_vehicle_updates();
}
