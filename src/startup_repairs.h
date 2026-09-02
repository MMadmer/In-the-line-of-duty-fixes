#pragma once
#include <Windows.h>
#include <filesystem>

namespace ild
{
[[nodiscard]] bool install_input_name_fix(HMODULE engine);
[[nodiscard]] bool install_preset_compatibility(HMODULE engine);
[[nodiscard]] bool install_audio_metadata_fix(const std::filesystem::path& root);
}
