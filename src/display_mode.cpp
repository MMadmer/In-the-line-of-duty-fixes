#include "display_mode.h"
#include "iat_hook.h"

#include <d3d9.h>

#include <algorithm>
#include <atomic>

namespace ild
{
namespace
{
// IDirect3D9::CreateDevice and IDirect3DDevice9::Reset are both the seventeenth entry of their interface.
// Patching the two slots avoids wrapping the interfaces, so no reference counting is taken over.
constexpr std::size_t create_device_slot = 16;
constexpr std::size_t reset_slot = 16;

using Direct3DCreate9Fn = IDirect3D9*(WINAPI*)(UINT);
using CreateDeviceFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD,
    D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
using ResetFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);

Direct3DCreate9Fn real_create{};
CreateDeviceFn real_create_device{};
ResetFn real_reset{};
std::atomic<int> mode{};
HWND game_window{};

bool patch_slot(void** table, std::size_t slot, void* replacement, void** previous)
{
    DWORD protection{};
    if (!VirtualProtect(table + slot, sizeof(void*), PAGE_READWRITE, &protection)) return false;
    if (previous) *previous = table[slot];
    table[slot] = replacement;
    DWORD restored{};
    VirtualProtect(table + slot, sizeof(void*), protection, &restored);
    return true;
}

void apply_window_style(HWND window, UINT width, UINT height)
{
    const auto current = mode.load(std::memory_order_acquire);
    if (!window || current == 0) return;
    MONITORINFO monitor{sizeof(monitor)};
    if (!GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor)) return;

    if (current == 1)
    {
        SetWindowLongPtrW(window, GWL_STYLE, WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS);
        SetWindowLongPtrW(window, GWL_EXSTYLE, WS_EX_APPWINDOW);
        SetWindowPos(window, HWND_NOTOPMOST, monitor.rcMonitor.left, monitor.rcMonitor.top,
            monitor.rcMonitor.right - monitor.rcMonitor.left,
            monitor.rcMonitor.bottom - monitor.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOACTIVATE);
        return;
    }

    // A window has to fit the work area, or a desktop-sized backbuffer hides the title bar and the taskbar.
    constexpr DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS;
    constexpr DWORD extended = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    SetWindowLongPtrW(window, GWL_STYLE, style);
    SetWindowLongPtrW(window, GWL_EXSTYLE, extended);
    RECT frame{};
    AdjustWindowRectEx(&frame, style, FALSE, extended);
    const auto work_width = monitor.rcWork.right - monitor.rcWork.left - (frame.right - frame.left);
    const auto work_height = monitor.rcWork.bottom - monitor.rcWork.top - (frame.bottom - frame.top);
    const auto scale = (std::min)({1.0,
        static_cast<double>((std::max)(1L, work_width)) / (std::max)(1u, width),
        static_cast<double>((std::max)(1L, work_height)) / (std::max)(1u, height)});
    RECT rectangle{0, 0, static_cast<LONG>(width * scale), static_cast<LONG>(height * scale)};
    AdjustWindowRectEx(&rectangle, style, FALSE, extended);
    const auto window_width = rectangle.right - rectangle.left;
    const auto window_height = rectangle.bottom - rectangle.top;
    SetWindowPos(window, HWND_NOTOPMOST,
        monitor.rcWork.left + (monitor.rcWork.right - monitor.rcWork.left - window_width) / 2,
        monitor.rcWork.top + (monitor.rcWork.bottom - monitor.rcWork.top - window_height) / 2,
        window_width, window_height, SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOACTIVATE);
}

void configure(D3DPRESENT_PARAMETERS* parameters)
{
    if (!parameters) return;
    if (mode.load(std::memory_order_acquire) == 0)
    {
        parameters->Windowed = FALSE;
        return;
    }
    // This is the part rs_fullscreen cannot do: a device created windowed never takes the display
    // exclusively, so Alt+Tab and the Windows key keep working.
    parameters->Windowed = TRUE;
    parameters->FullScreen_RefreshRateInHz = 0;
    parameters->BackBufferFormat = D3DFMT_UNKNOWN;
}

HRESULT STDMETHODCALLTYPE hooked_reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* parameters)
{
    configure(parameters);
    const auto result = real_reset(device, parameters);
    if (SUCCEEDED(result) && parameters)
        apply_window_style(game_window, parameters->BackBufferWidth, parameters->BackBufferHeight);
    return result;
}

HRESULT STDMETHODCALLTYPE hooked_create_device(IDirect3D9* self, UINT adapter, D3DDEVTYPE type,
    HWND focus, DWORD flags, D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** device)
{
    configure(parameters);
    const auto result = real_create_device(self, adapter, type, focus, flags, parameters, device);
    if (FAILED(result) || !device || !*device) return result;
    game_window = parameters && parameters->hDeviceWindow ? parameters->hDeviceWindow : focus;
    if (!real_reset)
    {
        const auto table = *reinterpret_cast<void***>(*device);
        static_cast<void>(patch_slot(table, reset_slot, reinterpret_cast<void*>(&hooked_reset),
            reinterpret_cast<void**>(&real_reset)));
    }
    if (parameters) apply_window_style(game_window, parameters->BackBufferWidth, parameters->BackBufferHeight);
    return result;
}

IDirect3D9* WINAPI hooked_create(UINT version)
{
    const auto instance = real_create(version);
    if (instance && !real_create_device)
    {
        const auto table = *reinterpret_cast<void***>(instance);
        static_cast<void>(patch_slot(table, create_device_slot, reinterpret_cast<void*>(&hooked_create_device),
            reinterpret_cast<void**>(&real_create_device)));
    }
    return instance;
}

[[nodiscard]] int stored_mode(const std::filesystem::path& root)
{
    const auto file = CreateFileW((root / L".ild-fixes" / L"settings.txt").c_str(), GENERIC_READ,
        FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return 0;
    char data[4096]{};
    DWORD read{};
    const auto ok = ReadFile(file, data, sizeof(data) - 1, &read, nullptr);
    CloseHandle(file);
    if (!ok) return 0;
    const std::string_view text(data, read);
    const auto at = text.find("video_mode=");
    if (at == text.npos) return 0;
    const auto digit = text[at + 11];
    return digit >= '0' && digit <= '2' ? digit - '0' : 0;
}
}

bool install_display_mode(const std::filesystem::path& root)
{
    static bool installed{};
    if (installed) return true;
    const auto renderer = GetModuleHandleW(L"xrRender_R2.dll");
    if (!renderer) return false;
    mode.store(stored_mode(root), std::memory_order_release);
    const auto previous = replace_iat_import(renderer, "d3d9.dll", "Direct3DCreate9",
        reinterpret_cast<void*>(&hooked_create));
    if (!previous) return false;
    real_create = reinterpret_cast<Direct3DCreate9Fn>(previous);
    installed = true;
    return true;
}

void set_display_mode(int value)
{
    mode.store(value, std::memory_order_release);
}

int display_mode()
{
    return mode.load(std::memory_order_acquire);
}
}
