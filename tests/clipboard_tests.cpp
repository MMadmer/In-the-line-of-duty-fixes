#include "console_clipboard.h"
#include <iostream>

int main()
{
    const auto original_station = GetProcessWindowStation();
    const auto original_desktop = GetThreadDesktop(GetCurrentThreadId());
    const auto name = L"ILD-Clipboard-QA-" + std::to_wstring(GetCurrentProcessId());
    const auto station = CreateWindowStationW(name.c_str(), 0, WINSTA_ALL_ACCESS, nullptr);
    if (!station || !SetProcessWindowStation(station))
    {
        std::cerr << "Private clipboard station unavailable; no clipboard access performed. Win32="
                  << GetLastError() << '\n';
        return 1;
    }
    const auto desktop = CreateDesktopW(L"Test", nullptr, nullptr, 0, GENERIC_ALL, nullptr);
    if (!desktop || !SetThreadDesktop(desktop)) return 2;

    // A separate window station owns a separate clipboard; the user's data is never opened.
    const auto window = CreateWindowExW(0, L"STATIC", L"ILD clipboard test", WS_POPUP,
        0, 0, 1, 1, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    const auto copied = window && ild::copy_console_clipboard("ild clipboard roundtrip", window);
    const auto matched = copied && ild::paste_console_clipboard() == "ild clipboard roundtrip";
    if (window) DestroyWindow(window);
    SetProcessWindowStation(original_station);
    SetThreadDesktop(original_desktop);
    CloseDesktop(desktop);
    CloseWindowStation(station);
    std::cout << "roundtrip=" << matched << " clipboard_scope=private_window_station\n";
    return matched ? 0 : 3;
}
