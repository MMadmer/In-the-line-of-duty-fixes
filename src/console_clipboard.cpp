#include "console_clipboard.h"
#include <algorithm>

namespace ild
{
bool copy_console_clipboard(const std::string& text, HWND owner)
{
    const auto count = MultiByteToWideChar(CP_ACP, 0, text.c_str(), -1, nullptr, 0);
    if (!count) return false;
    const auto memory = GlobalAlloc(GMEM_MOVEABLE, static_cast<SIZE_T>(count) * sizeof(wchar_t));
    if (!memory) return false;
    const auto buffer = static_cast<wchar_t*>(GlobalLock(memory));
    if (!buffer) { GlobalFree(memory); return false; }
    MultiByteToWideChar(CP_ACP, 0, text.c_str(), -1, buffer, count);
    GlobalUnlock(memory);
    if (!OpenClipboard(owner ? owner : GetActiveWindow())) { GlobalFree(memory); return false; }
    const auto copied = EmptyClipboard() && SetClipboardData(CF_UNICODETEXT, memory);
    CloseClipboard();
    if (!copied) GlobalFree(memory);
    return copied;
}

std::string paste_console_clipboard()
{
    if (!OpenClipboard(GetActiveWindow())) return {};
    std::string result;
    const auto memory = GetClipboardData(CF_UNICODETEXT);
    if (memory)
    {
        const auto source = static_cast<const wchar_t*>(GlobalLock(memory));
        if (source)
        {
            const auto maximum = std::min<std::size_t>(GlobalSize(memory) / sizeof(wchar_t), 4096);
            const auto length = wcsnlen_s(source, maximum);
            const auto count = WideCharToMultiByte(CP_ACP, 0, source, static_cast<int>(length), nullptr, 0, nullptr, nullptr);
            if (count > 0)
            {
                result.resize(count);
                WideCharToMultiByte(CP_ACP, 0, source, static_cast<int>(length), result.data(), count, nullptr, nullptr);
            }
            GlobalUnlock(memory);
        }
    }
    CloseClipboard();
    return result;
}
}
