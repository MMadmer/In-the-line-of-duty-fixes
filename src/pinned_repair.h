#pragma once

namespace ild
{
// A patch pinned to the validated game DLL: pending until that DLL is loaded, skipped on any other build.
enum class PinnedRepair { pending, applied, skipped, failed };
}
