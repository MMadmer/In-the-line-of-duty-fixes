#pragma once

#include <cstddef>
#include <span>

namespace ild::script_patch
{
enum class Binding
{
    bound,
    skipped,
    unsupported_source,
};

// The save-format repairs of the actor script are made either way; the call into the Lua payload only when the
// payload is on disk, or the actor's spawn would die on a module the engine cannot load.
[[nodiscard]] Binding bind_gameplay(std::span<std::byte> source, bool payload_present = true);
enum class Result
{
    applied,
    already_applied,
    unsupported_source,
};

[[nodiscard]] Result remove_console_execution(std::span<std::byte> source);
[[nodiscard]] bool reveal_abort_reason(std::span<std::byte> source);
[[nodiscard]] bool bind_update_menu(std::span<std::byte> source);
}
