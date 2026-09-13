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

// Nomad sells the courier suit as one that "lets you carry close to 90 kilograms". Its walk limit already is 90, but
// the capacity the inventory shows was left at 85, and the suit's own description, copied from the same line while
// the mod still stood on vanilla's 50, says "close to 80". Both follow the promise: "под 80." becomes "под 90.".
inline constexpr std::string_view courier_eighty = "\xEF\xEE\xE4 80.";
inline constexpr std::string_view courier_ninety = "\xEF\xEE\xE4 90.";

// A repair that must find its phrase exactly once in the line it rewrites.
[[nodiscard]] inline std::optional<std::string> replaced_once(std::string_view text, std::string_view old,
    std::string_view next)
{
    const auto at = text.find(old);
    if (at == text.npos || text.find(old, at + old.size()) != text.npos) return std::nullopt;
    std::string result(text);
    result.replace(at, old.size(), next);
    return result;
}

[[nodiscard]] inline std::optional<std::string> repaired_description(std::string_view section, std::string_view text)
{
    if (section == "item_lopata") return replaced_once(text, shovel_lifetime, shovel_sturdy);
    if (section == "kyrier_outfit") return replaced_once(text, courier_eighty, courier_ninety);
    return std::nullopt;
}

// Lines the mod did write, but wrong, in files the system config includes. Those are parsed before any hook of the
// pack exists, so each value is corrected where the engine reads it, and only while it still holds the mod's own.
// - The PP-4a sensor lost its name to a single letter; the neighbouring short name carries the right string id.
// - The two German submachine guns hold each other's id, and neither is in any string table. The MP-41 and the
//   MP-40 variant inherit from the MP-40, so the section decides which name a value stands for.
[[nodiscard]] inline const char* corrected_item_text(std::string_view section, std::string_view key,
    std::string_view value)
{
    if (section == "kruglov_flash") return key == "inv_name" && value == "i" ? "item_detector_yantar_name" : nullptr;
    if (key != "inv_name" && key != "inv_name_short") return nullptr;
    if (section == "wpn_mp41") return value == "mp40" || value == "mp41" ? "\xCC\xCF-41" : nullptr;
    if (section == "wpn_mp40" || section == "wpn_mp40n") return value == "mp41" ? "\xCC\xCF-40" : nullptr;
    return nullptr;
}

// Numbers read the same way. The sensor's grid keys hold a pixel position the engine multiplies by fifty again, so
// vanilla's cell comes back; the courier suit's capacity is raised to the 90 kg its seller promises.
[[nodiscard]] inline std::optional<float> corrected_item_number(std::string_view section, std::string_view key,
    float value)
{
    if (section == "kruglov_flash")
    {
        if (key == "inv_grid_x" && value == 4000.0f) return 5.0f;
        if (key == "inv_grid_y" && value == 1950.0f) return 14.0f;
    }
    if (section == "kyrier_outfit" && key == "additional_inventory_weight2" && value == 25.0f) return 30.0f;
    // The one weightless artefact among fifty-eight, and the crowbar that weighs 0.3 kg in the rucksack and 3.5 kg
    // in the hand: the stand-in the hidden-slot script swaps in for it copied a knife's weight.
    if (section == "af_gravi" && key == "inv_weight" && value == 0.0f) return 0.5f;
    if (section == "fake_lom" && key == "inv_weight" && value == 0.3f) return 3.5f;
    return std::nullopt;
}

// The B-94 rifle asks to be bound to a script module that exists nowhere, so the engine looks it up and reports the
// miss on every spawn. The line reads as absent while it still names that module.
[[nodiscard]] inline bool hidden_item_line(std::string_view section, std::string_view key, std::string_view value)
{
    return section == "wpn_b94" && key == "script_binding" && value == "bind_wpn.init";
}
}
