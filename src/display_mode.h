#pragma once

#include <Windows.h>

#include <filesystem>

namespace ild
{
// Screen mode: 0 exclusive fullscreen, 1 borderless window, 2 window.
// rs_fullscreen alone only chooses between exclusive fullscreen and a window whose device is still created
// with the engine's own parameters, so Alt+Tab and the Windows key stay captured. The presentation
// parameters are set at device creation and at every reset instead, which is what actually decides it.
[[nodiscard]] bool install_display_mode(const std::filesystem::path& root);
void set_display_mode(int mode);
[[nodiscard]] int display_mode();
}
