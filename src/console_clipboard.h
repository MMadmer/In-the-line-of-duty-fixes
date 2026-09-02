#pragma once
#include <Windows.h>
#include <string>

namespace ild
{
[[nodiscard]] bool copy_console_clipboard(const std::string& text, HWND owner = nullptr);
[[nodiscard]] std::string paste_console_clipboard();
}
