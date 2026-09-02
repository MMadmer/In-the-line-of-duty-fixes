#pragma once

#include <cstddef>
#include <span>

namespace ild::script_patch
{
enum class Result
{
    applied,
    already_applied,
    unsupported_source,
};

[[nodiscard]] Result remove_console_execution(std::span<std::byte> source);
[[nodiscard]] bool bind_update_menu(std::span<std::byte> source);
}
