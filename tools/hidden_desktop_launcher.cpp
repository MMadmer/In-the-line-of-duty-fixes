#include <Windows.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

namespace
{
struct CloseContext
{
    DWORD process_id{};
};

BOOL CALLBACK close_process_window(HWND window, LPARAM parameter)
{
    const auto& context = *reinterpret_cast<const CloseContext*>(parameter);
    DWORD process_id{};
    GetWindowThreadProcessId(window, &process_id);
    if (process_id == context.process_id)
    {
        PostMessageW(window, WM_CLOSE, 0, 0);
    }

    return TRUE;
}

BOOL CALLBACK hide_process_window(HWND window, LPARAM parameter)
{
    const auto& context = *reinterpret_cast<const CloseContext*>(parameter);
    DWORD process_id{};
    GetWindowThreadProcessId(window, &process_id);
    if (process_id == context.process_id && IsWindowVisible(window))
    {
        ShowWindowAsync(window, SW_HIDE);
    }

    return TRUE;
}

BOOL CALLBACK park_process_window(HWND window, LPARAM parameter)
{
    const auto& context = *reinterpret_cast<const CloseContext*>(parameter);
    DWORD process_id{};
    GetWindowThreadProcessId(window, &process_id);
    if (process_id == context.process_id)
    {
        wchar_t title[128]{};
        GetWindowTextW(window, title, 128);
        if (std::wstring_view(title).starts_with(L"S.T.A.L.K.E.R."))
            SetWindowPos(window, HWND_BOTTOM, -5000, -5000, 0, 0,
                SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
    return TRUE;
}

struct FocusContext
{
    DWORD process_id{};
    HWND window{};
    long long area{};
};

BOOL CALLBACK find_process_window(HWND window, LPARAM parameter)
{
    auto& context = *reinterpret_cast<FocusContext*>(parameter);
    DWORD process_id{};
    GetWindowThreadProcessId(window, &process_id);
    if (process_id != context.process_id || !IsWindowVisible(window))
    {
        return TRUE;
    }

    RECT rectangle{};
    if (!GetWindowRect(window, &rectangle))
    {
        return TRUE;
    }

    const auto area = static_cast<long long>(rectangle.right - rectangle.left) *
        static_cast<long long>(rectangle.bottom - rectangle.top);
    if (area > context.area)
    {
        context.area = area;
        context.window = window;
    }

    return TRUE;
}

void focus_process_window(HDESK desktop, DWORD process_id)
{
    std::thread focus([desktop, process_id]
    {
        if (!SetThreadDesktop(desktop))
        {
            return;
        }

        for (int attempt = 0; attempt < 150; ++attempt)
        {
            FocusContext context{process_id};
            EnumDesktopWindows(desktop, find_process_window, reinterpret_cast<LPARAM>(&context));
            if (context.window)
            {
                SetForegroundWindow(context.window);
                SetFocus(context.window);
                return;
            }

            Sleep(100);
        }
    });
    focus.join();
}

[[nodiscard]] std::wstring quote(const std::wstring& value)
{
    if (value.find_first_of(L" \t\"") == std::wstring::npos)
    {
        return value;
    }

    std::wstring result = L"\"";
    std::size_t backslashes{};
    for (const auto character : value)
    {
        if (character == L'\\')
        {
            ++backslashes;
            continue;
        }

        if (character == L'\"')
        {
            result.append(backslashes * 2 + 1, L'\\');
            result.push_back(character);
            backslashes = 0;
            continue;
        }

        result.append(backslashes, L'\\');
        backslashes = 0;
        result.push_back(character);
    }

    result.append(backslashes * 2, L'\\');
    result.push_back(L'\"');
    return result;
}

[[nodiscard]] std::string narrow_ascii(const std::wstring& value)
{
    std::string result;
    result.reserve(value.size());
    for (const auto character : value)
    {
        if (character > 0x7f)
        {
            return {};
        }
        result.push_back(static_cast<char>(character));
    }
    return result;
}

[[nodiscard]] bool file_contains(const std::filesystem::path& path, const std::string& needle)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        return false;
    }

    const std::string contents(std::istreambuf_iterator<char>(stream), {});
    return contents.find(needle) != std::string::npos;
}

[[nodiscard]] bool parse_seconds(const wchar_t* value, DWORD& seconds)
{
    try
    {
        const auto parsed = std::stoul(value);
        if (!parsed || parsed > 3600)
        {
            return false;
        }
        seconds = static_cast<DWORD>(parsed);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

}

int wmain(int argc, wchar_t** argv)
{
    if (argc < 7)
    {
        std::wcerr << L"Usage: ild_hidden_desktop_launcher <hidden-desktop|hidden-window|offscreen-window> "
                      L"<load-timeout> <soak-seconds> "
                      L"<ready-log> <ready-text> <exe> [args...]\n";
        return 2;
    }

    const std::wstring mode = argv[1];
    const auto hidden_desktop = mode == L"hidden-desktop";
    const auto offscreen = mode == L"offscreen-window";
    if (!hidden_desktop && mode != L"hidden-window" && !offscreen)
    {
        return 2;
    }

    DWORD load_timeout_seconds{};
    DWORD soak_seconds{};
    if (!parse_seconds(argv[2], load_timeout_seconds) || !parse_seconds(argv[3], soak_seconds))
    {
        return 2;
    }

    const std::filesystem::path ready_log = std::filesystem::absolute(argv[4]);
    const auto ready_text = narrow_ascii(argv[5]);
    if (ready_text.empty())
    {
        return 2;
    }

    const std::filesystem::path executable = std::filesystem::absolute(argv[6]);
    std::wstring command_line = quote(executable.wstring());
    for (int index = 7; index < argc; ++index)
    {
        command_line.push_back(L' ');
        command_line += quote(argv[index]);
    }

    const auto desktop_name = hidden_desktop ? L"ILD-QA-" + std::to_wstring(GetCurrentProcessId()) : L"Default";
    const auto desktop = hidden_desktop ? CreateDesktopW(
        desktop_name.c_str(), nullptr, nullptr, 0, 0x01FF, nullptr) : nullptr;
    if (hidden_desktop && !desktop)
    {
        std::wcerr << L"CreateDesktopW failed: " << GetLastError() << L'\n';
        return 3;
    }

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    const auto startup_desktop = hidden_desktop ? L"WinSta0\\" + desktop_name : L"WinSta0\\Default";
    startup.lpDesktop = const_cast<wchar_t*>(startup_desktop.c_str());
    if (!hidden_desktop)
    {
        startup.dwFlags = STARTF_USESHOWWINDOW;
        startup.wShowWindow = SW_HIDE;
    }
    PROCESS_INFORMATION process{};
    const auto working_directory = executable.parent_path();
    std::vector<wchar_t> mutable_command(command_line.begin(), command_line.end());
    mutable_command.push_back(L'\0');

    const auto created = CreateProcessW(
        executable.c_str(),
        mutable_command.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW,
        nullptr,
        working_directory.c_str(),
        &startup,
        &process);
    if (!created)
    {
        std::wcerr << L"CreateProcessW failed: " << GetLastError() << L'\n';
        if (desktop)
        {
            CloseDesktop(desktop);
        }
        return 4;
    }

    std::wcout << L"pid=" << process.dwProcessId << L" desktop=" << desktop_name << L'\n';
    std::atomic_bool stop_hiding{};
    std::thread hider;
    if (hidden_desktop)
    {
        focus_process_window(desktop, process.dwProcessId);
    }
    else
    {
        hider = std::thread([process_id = process.dwProcessId, &stop_hiding, offscreen]
        {
            const CloseContext context{process_id};
            const auto target = OpenDesktopW(L"Default", 0, FALSE,
                DESKTOP_ENUMERATE | DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS);
            while (!stop_hiding.load(std::memory_order_relaxed))
            {
                if (target) EnumDesktopWindows(target, offscreen ? park_process_window : hide_process_window,
                    reinterpret_cast<LPARAM>(&context));
                Sleep(50);
            }
            if (target) CloseDesktop(target);
        });
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(load_timeout_seconds);
    bool ready{};
    DWORD wait = WAIT_TIMEOUT;
    while (std::chrono::steady_clock::now() < deadline)
    {
        wait = WaitForSingleObject(process.hProcess, 250);
        if (wait != WAIT_TIMEOUT)
        {
            break;
        }

        if (file_contains(ready_log, ready_text))
        {
            ready = true;
            std::wcout << L"ready=1 soak_seconds=" << soak_seconds << L'\n';
            wait = WaitForSingleObject(process.hProcess, soak_seconds * 1000);
            break;
        }
    }

    const auto load_timed_out = !ready && wait == WAIT_TIMEOUT;
    bool forced{};
    if (wait == WAIT_TIMEOUT)
    {
        const CloseContext context{process.dwProcessId};
        if (hidden_desktop)
        {
            EnumDesktopWindows(desktop, close_process_window, reinterpret_cast<LPARAM>(&context));
        }
        else
        {
            const auto target = OpenDesktopW(L"Default", 0, FALSE, DESKTOP_ENUMERATE | DESKTOP_READOBJECTS);
            if (target)
            {
                EnumDesktopWindows(target, close_process_window, reinterpret_cast<LPARAM>(&context));
                CloseDesktop(target);
            }
        }
        if (WaitForSingleObject(process.hProcess, 10'000) == WAIT_TIMEOUT)
        {
            forced = TerminateProcess(process.hProcess, 0xDEAD) != FALSE;
            WaitForSingleObject(process.hProcess, 10'000);
        }
    }

    DWORD exit_code{};
    GetExitCodeProcess(process.hProcess, &exit_code);
    std::wcout << L"exit_code=" << exit_code << L" ready=" << ready << L" load_timed_out=" << load_timed_out
               << L" forced=" << forced << L'\n';

    stop_hiding.store(true, std::memory_order_relaxed);
    if (hider.joinable())
    {
        hider.join();
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    if (desktop)
    {
        CloseDesktop(desktop);
    }
    return wait == WAIT_FAILED ? 5 : 0;
}
