// INITGUID instantiates GUID_SysKeyboard here; linking dinput8.lib is not an option, since this module is
// dinput8 itself.
#define INITGUID
#include <initguid.h>

#define DIRECTINPUT_VERSION 0x0800

#include "display_mode.h"
#include "iat_hook.h"

#include <d3d9.h>
#include <dinput.h>

#include <algorithm>
#include <atomic>
#include <string_view>

namespace ild
{
namespace
{
// Slots are patched rather than the interfaces wrapped, so no reference counting is taken over.
constexpr std::size_t create_device_slot = 16;   // IDirect3D9::CreateDevice
constexpr std::size_t reset_slot = 16;           // IDirect3DDevice9::Reset
constexpr std::size_t present_slot = 17;         // IDirect3DDevice9::Present
constexpr std::size_t input_create_device_slot = 3;  // IDirectInput8::CreateDevice
constexpr std::size_t cooperative_level_slot = 13;   // IDirectInputDevice8::SetCooperativeLevel

using Direct3DCreate9Fn = IDirect3D9*(WINAPI*)(UINT);
using CreateDeviceFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD,
    D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
using ResetFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
using PresentFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, const RECT*, const RECT*, HWND,
    const RGNDATA*);
using InputCreateDeviceFn = HRESULT(STDMETHODCALLTYPE*)(void*, const GUID&, void**, void*);
using CooperativeLevelFn = HRESULT(STDMETHODCALLTYPE*)(void*, HWND, DWORD);

Direct3DCreate9Fn real_create{};
CreateDeviceFn real_create_device{};
ResetFn real_reset{};
PresentFn real_present{};
InputCreateDeviceFn real_input_create_device{};
CooperativeLevelFn real_cooperative_level{};
void* keyboard_device{};
HWND keyboard_window{};
DWORD keyboard_flags{};
std::atomic<int> mode{};
HWND game_window{};
UINT back_width{};
UINT back_height{};

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

    // A window has to fit the work area, or a desktop-sized backbuffer puts the title bar off screen and
    // covers the taskbar, which is what made "windowed" look borderless.
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

[[nodiscard]] bool window_is_above(HWND window, HWND other)
{
    for (auto current = GetWindow(window, GW_HWNDPREV); current; current = GetWindow(current, GW_HWNDPREV))
        if (current == other) return false;
    return window != other;
}

// The engine restyles its own window after the device is up, so the choice is held every frame. A borderless
// window must also not sit above whatever the player switched to, or they see a frozen frame instead of it.
void maintain_window_state()
{
    const auto current = mode.load(std::memory_order_acquire);
    if (!game_window || current == 0) return;
    const auto style = static_cast<LONG_PTR>(current == 2
        ? WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS
        : WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS);
    const auto extended = static_cast<LONG_PTR>(current == 2
        ? WS_EX_APPWINDOW | WS_EX_WINDOWEDGE
        : WS_EX_APPWINDOW);
    if (GetWindowLongPtrW(game_window, GWL_STYLE) != style ||
        GetWindowLongPtrW(game_window, GWL_EXSTYLE) != extended)
        apply_window_style(game_window, back_width, back_height);

    const auto foreground = GetForegroundWindow();
    if (current == 1 && foreground && foreground != game_window && window_is_above(game_window, foreground))
        SetWindowPos(game_window, foreground, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void configure(D3DPRESENT_PARAMETERS* parameters)
{
    if (!parameters) return;
    if (mode.load(std::memory_order_acquire) == 0)
    {
        parameters->Windowed = FALSE;
        return;
    }
    // This is what rs_fullscreen cannot do: a device created windowed never takes the display exclusively,
    // so Alt+Tab shows the other program instead of a frozen frame.
    parameters->Windowed = TRUE;
    parameters->FullScreen_RefreshRateInHz = 0;
    parameters->BackBufferFormat = D3DFMT_UNKNOWN;
}

HRESULT STDMETHODCALLTYPE hooked_present(IDirect3DDevice9* device, const RECT* source,
    const RECT* destination, HWND window, const RGNDATA* dirty)
{
    maintain_window_state();
    return real_present(device, source, destination, window, dirty);
}

HRESULT STDMETHODCALLTYPE hooked_reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* parameters)
{
    configure(parameters);
    const auto result = real_reset(device, parameters);
    if (SUCCEEDED(result) && parameters)
    {
        back_width = parameters->BackBufferWidth;
        back_height = parameters->BackBufferHeight;
        apply_window_style(game_window, back_width, back_height);
    }
    return result;
}

HRESULT STDMETHODCALLTYPE hooked_create_device(IDirect3D9* self, UINT adapter, D3DDEVTYPE type,
    HWND focus, DWORD flags, D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** device)
{
    configure(parameters);
    const auto result = real_create_device(self, adapter, type, focus, flags, parameters, device);
    if (FAILED(result) || !device || !*device) return result;
    game_window = parameters && parameters->hDeviceWindow ? parameters->hDeviceWindow : focus;
    const auto table = *reinterpret_cast<void***>(*device);
    if (!real_reset)
        static_cast<void>(patch_slot(table, reset_slot, reinterpret_cast<void*>(&hooked_reset),
            reinterpret_cast<void**>(&real_reset)));
    if (!real_present)
        static_cast<void>(patch_slot(table, present_slot, reinterpret_cast<void*>(&hooked_present),
            reinterpret_cast<void**>(&real_present)));
    if (parameters)
    {
        back_width = parameters->BackBufferWidth;
        back_height = parameters->BackBufferHeight;
        apply_window_style(game_window, back_width, back_height);
    }
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

[[nodiscard]] DWORD adjust_keyboard(DWORD flags)
{
    if (mode.load(std::memory_order_acquire) == 0) return flags;
    // DISCL_NOWINKEY is the direct block on the Windows key, and an exclusive grab swallows Alt+Tab. Neither
    // belongs in a window. The mouse is never touched, so aiming and cursor capture are unchanged.
    return (flags & ~DWORD{DISCL_EXCLUSIVE | DISCL_NOWINKEY}) | DISCL_NONEXCLUSIVE;
}

HRESULT STDMETHODCALLTYPE hooked_cooperative_level(void* device, HWND window, DWORD flags)
{
    keyboard_device = device;
    keyboard_window = window;
    keyboard_flags = flags;
    return real_cooperative_level(device, window, adjust_keyboard(flags));
}

HRESULT STDMETHODCALLTYPE hooked_input_create_device(void* self, const GUID& id, void** device, void* outer)
{
    const auto result = real_input_create_device(self, id, device, outer);
    if (FAILED(result) || !device || !*device || id != GUID_SysKeyboard || real_cooperative_level)
        return result;
    const auto table = *reinterpret_cast<void***>(*device);
    static_cast<void>(patch_slot(table, cooperative_level_slot,
        reinterpret_cast<void*>(&hooked_cooperative_level),
        reinterpret_cast<void**>(&real_cooperative_level)));
    return result;
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
    if (at == text.npos || at + 11 >= text.size()) return 0;
    const auto digit = text[at + 11];
    return digit >= '0' && digit <= '2' ? digit - '0' : 0;
}
}

void load_display_mode(const std::filesystem::path& root)
{
    // The keyboard is acquired long before the renderer exists, so the mode has to be known by then.
    static bool loaded{};
    if (loaded) return;
    loaded = true;
    mode.store(stored_mode(root), std::memory_order_release);
}

void hook_direct_input(void* instance)
{
    if (!instance || real_input_create_device) return;
    const auto table = *reinterpret_cast<void***>(instance);
    static_cast<void>(patch_slot(table, input_create_device_slot,
        reinterpret_cast<void*>(&hooked_input_create_device),
        reinterpret_cast<void**>(&real_input_create_device)));
}

bool install_display_mode(const std::filesystem::path& root)
{
    static bool installed{};
    if (installed) return true;
    const auto renderer = GetModuleHandleW(L"xrRender_R2.dll");
    if (!renderer) return false;
    load_display_mode(root);
    const auto previous = replace_iat_import(renderer, "d3d9.dll", "Direct3DCreate9",
        reinterpret_cast<void*>(&hooked_create));
    if (!previous) return false;
    real_create = reinterpret_cast<Direct3DCreate9Fn>(previous);
    installed = true;
    return true;
}

void set_display_mode(int value)
{
    if (mode.exchange(value, std::memory_order_acq_rel) == value) return;
    // The keyboard is already acquired by now, so it has to be re-acquired at the level the new mode wants.
    if (keyboard_device && keyboard_window && real_cooperative_level)
    {
        using SimpleFn = HRESULT(STDMETHODCALLTYPE*)(void*);
        const auto table = *reinterpret_cast<void***>(keyboard_device);
        const auto unacquire = reinterpret_cast<SimpleFn>(table[8]);
        const auto acquire = reinterpret_cast<SimpleFn>(table[7]);
        unacquire(keyboard_device);
        real_cooperative_level(keyboard_device, keyboard_window, adjust_keyboard(keyboard_flags));
        acquire(keyboard_device);
    }
}

int display_mode()
{
    return mode.load(std::memory_order_acquire);
}
}
