#pragma once

#include <filesystem>
#include <string_view>

namespace ild
{
[[nodiscard]] bool install_texture_aliases(const std::filesystem::path& root);
#ifdef ILD_CONSOLE_QA
[[nodiscard]] bool verify_texture_repairs();
#endif

namespace texture_aliases
{
[[nodiscard]] constexpr std::string_view missing_bump_fallback(
    std::string_view path, std::string_view name, std::string_view extension) noexcept
{
    if (path != "$game_textures$" || extension != ".dds") return {};
    if (name == "prop\\prop_blanket_bump" || name == "wood\\wood_board_02_bump" ||
        name == "wood\\wood_collect_bump") return "ed\\ed_dummy_bump";
    if (name == "wood\\wood_board_02_bump#" || name == "wood\\wood_collect_bump#")
        return "ed\\ed_dummy_bump#";
    return {};
}
}
}
