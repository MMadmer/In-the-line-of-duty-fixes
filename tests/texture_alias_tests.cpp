#include "texture_aliases.h"

#include <array>
#include <string_view>

int main()
{
    using ild::texture_aliases::missing_bump_fallback;
    constexpr std::array normal{
        "prop\\prop_blanket_bump", "wood\\wood_board_02_bump", "wood\\wood_collect_bump"};
    constexpr std::array correction{"wood\\wood_board_02_bump#", "wood\\wood_collect_bump#"};
    for (const auto name : normal)
        if (missing_bump_fallback("$game_textures$", name, ".dds") != "ed\\ed_dummy_bump") return 1;
    for (const auto name : correction)
        if (missing_bump_fallback("$game_textures$", name, ".dds") != "ed\\ed_dummy_bump#") return 2;
    constexpr std::array excluded{
        "prop\\prop_blanket_bump#", "wood\\wood_board_01_bump", "wood\\wood_collect",
        "wood\\wood_collect_bump.dds", "..\\wood\\wood_collect_bump", "Wood\\wood_collect_bump", ""};
    for (const auto name : excluded)
        if (!missing_bump_fallback("$game_textures$", name, ".dds").empty()) return 3;
    if (!missing_bump_fallback("$level$", normal[0], ".dds").empty()) return 4;
    if (!missing_bump_fallback("$game_textures$", normal[0], ".thm").empty()) return 5;
    if (!missing_bump_fallback({}, normal[0], ".dds").empty()) return 6;
    if (!missing_bump_fallback("$game_textures$", normal[0], {}).empty()) return 7;
    return 0;
}
