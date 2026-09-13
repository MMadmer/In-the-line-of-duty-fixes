#pragma once

#include "pinned_repair.h"

namespace ild
{
// Walking drains stamina against the carrying capacity the inventory shows, suit included, as jumping already does.
[[nodiscard]] PinnedRepair install_walk_weight();

#ifdef ILD_CARRY_WEIGHT_TESTS
// The walking-step replacement and the two addresses it leaves through, so a unit test can run it on fake objects.
struct WalkStepForTests
{
    void* entry;
    void** resume;
    void** original;
};
[[nodiscard]] WalkStepForTests walk_step_for_tests();
#endif
}
