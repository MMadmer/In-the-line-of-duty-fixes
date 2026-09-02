#pragma once
#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>

namespace ild
{
enum class ConfigRepair { mechanic_dialog, prince_dialog, skill_text, info_root, camp_condition,
    burer_dialog, psi_sound, car_text, skat_upgrade, mutant_sounds };
struct ConfigRepairSource
{
    std::wstring_view relative;
    std::size_t size;
    std::string_view hash;
    ConfigRepair repair;
};
[[nodiscard]] std::span<const ConfigRepairSource> config_repair_sources();
[[nodiscard]] bool is_config_repair_path(std::wstring_view path, const std::filesystem::path& root);
[[nodiscard]] bool repair_config_text(std::string& text, ConfigRepair repair);
[[nodiscard]] bool repair_config_buffer(std::span<std::byte> bytes);
}
