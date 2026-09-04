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

// The device is created before the loader's own entry point runs on some start-ups, so the import is
// replaced from DllMain, where only the executable's import table is touched.
[[nodiscard]] bool install_display_mode_early();

// Called with the IDirectInput8 the engine receives, so a windowed mode can stop the keyboard being held
// exclusively and stop DISCL_NOWINKEY blocking the Windows key.
void load_display_mode(const std::filesystem::path& root);
void hook_direct_input(void* instance);
void set_display_mode(int mode);
[[nodiscard]] int display_mode();
}
