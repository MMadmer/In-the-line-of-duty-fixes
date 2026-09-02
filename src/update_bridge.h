#pragma once
#include <Windows.h>
#include <filesystem>

namespace ild
{
[[nodiscard]] bool install_update_bridge(HMODULE engine, const std::filesystem::path& root);
}
