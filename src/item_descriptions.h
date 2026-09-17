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

// The mechanic's re-chambered guns keep the description of the gun they were made from, so the text names a
// calibre they no longer take: the "AK-47" that only fires 7.62x54R still lists 5.45x39, the German carbine
// that fires only 7.92 still promises the NATO round of its parent, and so on. Each gets a line of its own
// in the author's style, in CP1251, while the value is still the inherited one - a file already given a text of
// its own is left alone. These are literals rather than repaired_description's cache: that cache is keyed by the
// original text, which the parent and the child share.
[[nodiscard]] inline const char* corrected_description(std::string_view section, std::string_view value)
{
    if (section == "wpn_ak74_mod3" && value == "enc_weapons1_wpn-ak74") return "\xC0\xE2\xF2\xEE\xEC\xE0\xF2\x20\xCA\xE0\xEB\xE0\xF8\xED\xE8\xEA\xEE\xE2\xE0\x2C\x20\xEF\xE5\xF0\xE5\xE4\xE5\xEB\xE0\xED\xED\xFB\xE9\x20\xEC\xE5\xF5\xE0\xED\xE8\xEA\xEE\xEC\x20\xEF\xEE\xE4\x20\xE2\xE8\xED\xF2\xEE\xE2\xEE\xF7\xED\xFB\xE9\x20\xEF\xE0\xF2\xF0\xEE\xED\x20\x37\x2C\x36\x32\x78\x35\x34\x20\xEC\xEC\x2E\x20\xD2\xFF\xE6\xE5\xEB\xE5\xE5\x20\xE8\x20\xEC\xE5\xE4\xEB\xE5\xED\xED\xE5\xE5\x20\xEF\xF0\xE5\xE6\xED\xE5\xE3\xEE\x2C\x20\xE7\xE0\xF2\xEE\x20\xE1\xFC\xB8\xF2\x20\xE4\xE0\xEB\xFC\xF8\xE5\x20\xE8\x20\xEF\xF0\xEE\xE1\xE8\xE2\xE0\xE5\xF2\x20\xE1\xF0\xEE\xED\xFE\x2E\\n\x20\xC1\xEE\xE5\xEF\xF0\xE8\xEF\xE0\xF1\xFB\x3A\\n\x20\x37\x2C\x36\x32\x78\x35\x34\x20\xEC\xEC\x20\x37\xCD\x31\x2C\\n\x20\x37\x2C\x36\x32\x78\x35\x34\x20\xEC\xEC\x20\x37\xCD\x31\x34\x2C\\n\x20\x37\x2C\x36\x32\x78\x35\x34\x20\xEC\xEC\x20\xC1\xCF\x2E";
    if (section == "wpn_mp5_mod" && value == "enc_weapons1_wpn-mp5") return "\x4D\x50\x35\x2C\x20\xEF\xE5\xF0\xE5\xE4\xE5\xEB\xE0\xED\xED\xFB\xE9\x20\xEC\xE5\xF5\xE0\xED\xE8\xEA\xEE\xEC\x20\xEF\xEE\xE4\x20\xEF\xE0\xF2\xF0\xEE\xED\x20\x31\x31\x2C\x34\x33\x78\x32\x33\x20\xEC\xEC\x3A\x20\xF2\xFF\xE6\xB8\xEB\xE0\xFF\x20\xEF\xF3\xEB\xFF\x20\xE1\xFC\xB8\xF2\x20\xF1\xE8\xEB\xFC\xED\xE5\xE5\x2C\x20\xEE\xF2\xE4\xE0\xF7\xE0\x20\xE7\xE0\xEC\xE5\xF2\xED\xE5\xE5\x2E\\n\x20\xC1\xEE\xE5\xEF\xF0\xE8\xEF\xE0\xF1\xFB\x3A\\n\x20\x31\x31\x2C\x34\x33\x78\x32\x33\x20\xEC\xEC\x20\x46\x4D\x4A\x2C\\n\x20\x31\x31\x2C\x34\x33\x78\x32\x33\x20\xEC\xEC\x20\x48\x79\x64\x72\x6F\x2D\x53\x68\x6F\x63\x6B\x2E";
    if (section == "wpn_pm_mod2" && value == "enc_weapons1_wpn-pm") return "\xCF\xE8\xF1\xF2\xEE\xEB\xE5\xF2\x20\xCC\xE0\xEA\xE0\xF0\xEE\xE2\xE0\x2C\x20\xEF\xE5\xF0\xE5\xE4\xE5\xEB\xE0\xED\xED\xFB\xE9\x20\xEC\xE5\xF5\xE0\xED\xE8\xEA\xEE\xEC\x20\xEF\xEE\xE4\x20\xEF\xE0\xF2\xF0\xEE\xED\x20\x39\x78\x31\x39\x20\xEC\xEC\x3A\x20\xF2\xEE\xF7\xED\xE5\xE5\x20\xE8\x20\xEC\xEE\xF9\xED\xE5\xE5\x2C\x20\xF7\xE5\xEC\x20\xF1\x20\xF0\xEE\xE4\xED\xFB\xEC\x20\x39\x78\x31\x38\x2E\\n\x20\xC1\xEE\xE5\xEF\xF0\xE8\xEF\xE0\xF1\xFB\x3A\\n\x20\x39\x78\x31\x39\x20\xEC\xEC\x20\x46\x4D\x4A\x2C\\n\x20\x39\x78\x31\x39\x20\xEC\xEC\x20\x50\x42\x50\x2E";
    if ((section == "wpn_fort_mod2" || section == "wpn_fort_mod22") && value == "enc_weapons1_wpn-fort")
        return "\xD4\xEE\xF0\xF2\x2D\x31\x32\x2C\x20\xEF\xE5\xF0\xE5\xE4\xE5\xEB\xE0\xED\xED\xFB\xE9\x20\xEC\xE5\xF5\xE0\xED\xE8\xEA\xEE\xEC\x20\xEF\xEE\xE4\x20\xEF\xE0\xF2\xF0\xEE\xED\x20\x39\x78\x31\x39\x20\xEC\xEC\x3A\x20\xF2\xE0\x20\xE6\xE5\x20\xED\xE0\xE4\xB8\xE6\xED\xEE\xF1\xF2\xFC\x20\xE8\x20\xF2\xEE\xF7\xED\xEE\xF1\xF2\xFC\x2C\x20\xEF\xF3\xEB\xFF\x20\xF1\xE8\xEB\xFC\xED\xE5\xE5\x2E\\n\x20\xC1\xEE\xE5\xEF\xF0\xE8\xEF\xE0\xF1\xFB\x3A\\n\x20\x39\x78\x31\x39\x20\xEC\xEC\x20\x46\x4D\x4A\x2C\\n\x20\x39\x78\x31\x39\x20\xEC\xEC\x20\x50\x42\x50\x2E";
    if (section == "wpn_hpsa_mod" && value == "enc_weapons1_wpn-hpsa") return "\x42\x72\x6F\x77\x6E\x69\x6E\x67\x20\x48\x50\x2C\x20\xEF\xE5\xF0\xE5\xE4\xE5\xEB\xE0\xED\xED\xFB\xE9\x20\xEC\xE5\xF5\xE0\xED\xE8\xEA\xEE\xEC\x20\xEF\xEE\xE4\x20\xEF\xE0\xF2\xF0\xEE\xED\x20\x39\x78\x31\x38\x20\xEC\xEC\x3A\x20\xEF\xE0\xF2\xF0\xEE\xED\xFB\x20\xEA\x20\xED\xE5\xEC\xF3\x20\xF2\xE5\xEF\xE5\xF0\xFC\x20\xE2\x20\xEA\xE0\xE6\xE4\xEE\xEC\x20\xF0\xFE\xEA\xE7\xE0\xEA\xE5\x2C\x20\xF3\xF0\xEE\xED\x20\xF7\xF3\xF2\xFC\x20\xED\xE8\xE6\xE5\x2E\\n\x20\xC1\xEE\xE5\xEF\xF0\xE8\xEF\xE0\xF1\xFB\x3A\\n\x20\x39\x78\x31\x38\x20\xEC\xEC\x20\x46\x4D\x4A\x2C\\n\x20\x39\x78\x31\x38\x20\xEC\xEC\x20\x2B\x50\x2B\x2E";
    if (section == "wpn_toz34_m" && value == "enc_weapons1_wpn-toz34") return "\xCE\xE1\xF0\xE5\xE7\x20\xD2\xCE\xC7\x2D\x33\x34\x3A\x20\xEA\xEE\xF0\xEE\xF7\xE5\x20\xE8\x20\xF3\xE4\xEE\xE1\xED\xE5\xE5\x20\xE2\x20\xF2\xE5\xF1\xED\xEE\xF2\xE5\x2C\x20\xED\xEE\x20\xE4\xF0\xEE\xE1\xFC\x20\x31\x32\x78\x37\x30\x20\xE2\x20\xED\xE5\xE3\xEE\x20\xE1\xEE\xEB\xFC\xF8\xE5\x20\xED\xE5\x20\xE8\xE4\xB8\xF2\x20\x2D\x20\xF2\xEE\xEB\xFC\xEA\xEE\x20\xE6\xE5\xEA\xE0\xED\x20\xE8\x20\xE4\xF0\xEE\xF2\xE8\xEA\x2E\\n\x20\xC1\xEE\xE5\xEF\xF0\xE8\xEF\xE0\xF1\xFB\x3A\\n\x20\x31\x32\x78\x37\x36\x20\xE6\xE5\xEA\xE0\xED\x2C\\n\x20\x31\x32\x78\x37\x36\x20\xE4\xF0\xEE\xF2\xE8\xEA\x2E";
    if (section == "wpn_nemec_k98" && value.starts_with("\xD1\xE8\xFF\x20\xE2\xE8\xED\xF2\xEE\xE2\xEA\xE0\x20\xEE\xE4\xED\xEE\xE7\xED\xE0\xF7\xED\xEE\x20\xEF\xF0\xE8\xE1\xFB\xEB\xE0\x20\xF1\x20\xCD\xE5\xEC\xE5\xF6\xE8\xE8\x2E")) return "\xCD\xE5\xEC\xE5\xF6\xEA\xE8\xE9\x20\xEA\xE0\xF0\xE0\xE1\xE8\xED\x20\x4D\x61\x75\x73\x65\x72\x20\x39\x38\x6B\x2C\x20\xEA\xE0\xEA\xE8\xEC\x20\xEE\xED\x20\xE8\x20\xEF\xF0\xE8\xE1\xFB\xEB\x20\xE2\x20\xC7\xEE\xED\xF3\x3A\x20\xEF\xEE\xE4\x20\xF0\xEE\xE4\xED\xEE\xE9\x20\xEF\xE0\xF2\xF0\xEE\xED\x20\x37\x2C\x39\x32\x20\xEC\xEC\x2E\x20\xD0\xE5\xE4\xEA\xEE\xF1\xF2\xFC\x20\xF1\xE0\xEC\x20\xEF\xEE\x20\xF1\xE5\xE1\xE5\x2C\x20\xE0\x20\xEF\xE0\xF2\xF0\xEE\xED\xFB\x20\xEA\x20\xED\xE5\xEC\xF3\x20\x2D\x20\xE5\xF9\xB8\x20\xE1\xEE\xEB\xFC\xF8\xE0\xFF\x2E\\n\x20\xC1\xEE\xE5\xEF\xF0\xE8\xEF\xE0\xF1\xFB\x3A\\n\x20\x37\x2C\x39\x32\x20\xEC\xEC\x2E";
    return nullptr;
}

// Lines the mod did write, but wrong, in files the system config includes. Those are parsed before any hook of the
// pack exists, so each value is corrected where the engine reads it, and only while it still holds the mod's own.
// - The PP-4a sensor lost its name to a single letter; the neighbouring short name carries the right string id.
// - The two German submachine guns hold each other's id, and neither is in any string table. The MP-41 and the
//   MP-40 variant inherit from the MP-40, so the section decides which name a value stands for.
// - The rat king artefact is declared as a medkit, so the quick-use key eats it ahead of an ordinary medkit and
//   the player pays health for a tushkan. It is food to the engine now, which runs the very same eat lines.
// - The OC-33 came from Arsenal Mod with a shell effect that particles.xr does not have, and every shot near the
//   camera asks for it: a fatal. It drops the shells every other gun of the mod drops.
// - The German carbine is named like its NATO parent; its name says which one it is.
[[nodiscard]] inline const char* corrected_item_text(std::string_view section, std::string_view key,
    std::string_view value)
{
    if (section == "syper_art_dryg_tyshkan") return key == "class" && value == "II_MEDKI" ? "II_FOOD" : nullptr;
    if (section == "kruglov_flash") return key == "inv_name" && value == "i" ? "item_detector_yantar_name" : nullptr;
    if (section == "wpn_oc33")
        return key == "shell_particles" && value == "weapons\\arsenal_shells1" ? "weapons\\generic_shells" : nullptr;
    if (key == "description") return corrected_description(section, value);
    if (key != "inv_name" && key != "inv_name_short") return nullptr;
    if (section == "wpn_nemec_k98") return value == "Karabin-98" || value == "wpn-k98" ? "Karabin-98 (7.92)" : nullptr;
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
    // The courier suit's immunity section holds 1.00 for every hit type, where every other suit of the mod holds
    // a few hundredths and nothing at all for radiation and psi. The engine multiplies each hit by that number
    // and takes the result off the suit's condition, so this one lost a third of itself to a single hit and
    // wore through to nothing in minutes - and even a walk through radiation wore it. It takes the numbers of
    // the stalker suit it is cut from, whose bone protection it already shares, while the file still says 1.00.
    if (section == "sect_kyrier_outfit_immunities" && value == 1.0f)
    {
        if (key == "burn_immunity") return 0.03f;
        if (key == "strike_immunity") return 0.02f;
        if (key == "shock_immunity") return 0.01f;
        if (key == "wound_immunity") return 0.015f;
        if (key == "radiation_immunity" || key == "telepatic_immunity") return 0.0f;
        if (key == "chemical_burn_immunity") return 0.035f;
        if (key == "explosion_immunity") return 0.02f;
        if (key == "fire_wound_immunity") return 0.022f;
    }
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
