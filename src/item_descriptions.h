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
