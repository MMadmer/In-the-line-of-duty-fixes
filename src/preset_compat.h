#pragma once
#include <string_view>

namespace ild
{
[[nodiscard]] constexpr bool obsolete_preset_option(std::string_view line)
{
    const auto first = line.find_first_not_of(" \t");
    if (first == line.npos) return false;
    line.remove_prefix(first);
    const auto end = line.find_first_of(" \t\r\n");
    const auto name = line.substr(0, end);
    return name == "r__dtex_range" || name == "r__ssa_glod_end" || name == "r__ssa_glod_start" ||
        name == "r__wallmark_ttl" || name == "rs_detail" || name == "rs_skeleton_update" || name == "vid_bpp";
}
}
