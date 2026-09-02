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
// R2 substitutes ed_dummy_bump / ed_dummy_bump# for any missing bump texture after logging a warning; the alias
// hands the renderer the same resource up front, so the picture is unchanged and the console stays readable.
[[nodiscard]] constexpr std::string_view missing_bump_fallback(
    std::string_view path, std::string_view name, std::string_view extension) noexcept
{
    if (path != "$game_textures$" || extension != ".dds" || name.starts_with("ed\\ed_dummy_bump")) return {};
    if (name.ends_with("_bump#")) return "ed\\ed_dummy_bump#";
    if (name.ends_with("_bump")) return "ed\\ed_dummy_bump";
    return {};
}
}
}
