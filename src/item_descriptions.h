#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace ild
{
// The grave stashes no longer take the shovel, so its description stops calling it short-lived: "though not
// durable, but" becomes "sturdy and reliable, and", in the mod's own CP1251. The item config is parsed before any
// hook exists, so the text is replaced where the engine reads it rather than in the file.
inline constexpr std::string_view shovel_lifetime =
    "\xF5\xEE\xF2\xFC \xE8 \xED\xE5 \xE4\xEE\xEB\xE3\xEE\xE2\xE5\xF7\xED\xE0, \xED\xEE";
inline constexpr std::string_view shovel_sturdy =
    "\xEA\xF0\xE5\xEF\xEA\xE0\xFF \xE8 \xED\xE0\xE4\xB8\xE6\xED\xE0\xFF, \xE4\xE0 \xE8";

// det_artefact_super is the one detector the mod never wrote text for: no string table anywhere declares
// det_art_super or det_art_super_info, so the inventory prints those identifiers where the name and the
// description belong. Its lines live in misc/items.ltx, which the system config includes and the engine parses
// before any hook of the pack exists, so a file repair cannot reach them - the text is supplied where the engine
// reads it, exactly as the shovel's description is. CP1251, the encoding the mod's own item file uses; the name
// follows its two siblings, and the description says what the detector actually does.
inline constexpr char super_detector_name[] =
    "\xC4\xE5\xF2\xE5\xEA\xF2\xEE\xF0\"\xC2\xF1\xE5\xE2\xE8\xE4\xFF\xF9\xE8\xE9\"";
inline constexpr char super_detector_short[] = "\xC2\xF1\xE5\xE2\xE8\xE4\xFF\xF9\xE8\xE9";
inline constexpr char super_detector_info[] =
    "\xCC\xE5\xF2\xE8\xF2 \xED\xE0 \xEA\xE0\xF0\xF2\xE5 \xEA\xE0\xE6\xE4\xFB\xE9 "
    "\xE0\xF0\xF2\xE5\xF4\xE0\xEA\xF2 \xF3\xF0\xEE\xE2\xED\xFF \xE8 "
    "\xEF\xEE\xE4\xEF\xE8\xF1\xFB\xE2\xE0\xE5\xF2 \xE5\xE3\xEE \xED\xE0\xE7\xE2\xE0\xED\xE8\xE5\xEC - "
    "\xF0\xE0\xF1\xF1\xF2\xEE\xFF\xED\xE8\xE5 \xE5\xEC\xF3 \xED\xE5 \xEF\xEE\xEC\xE5\xF5\xE0.";

// Text for a line the mod left as its own identifier, by the key the engine asks for. Any other section is left
// alone. The value is a literal, so it already outlives the process and needs no storage of ours.
[[nodiscard]] inline const char* supplied_item_text(std::string_view section, std::string_view key)
{
    if (section != "det_artefact_super") return nullptr;
    if (key == "inv_name") return super_detector_name;
    if (key == "inv_name_short") return super_detector_short;
    if (key == "description") return super_detector_info;
    return nullptr;
}

[[nodiscard]] inline std::optional<std::string> repaired_description(std::string_view section, std::string_view text)
{
    if (section != "item_lopata") return std::nullopt;
    const auto at = text.find(shovel_lifetime);
    if (at == text.npos || text.find(shovel_lifetime, at + shovel_lifetime.size()) != text.npos) return std::nullopt;
    std::string result(text);
    result.replace(at, shovel_lifetime.size(), shovel_sturdy);
    return result;
}
}
