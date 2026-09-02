#include "x86_detour.h"
#include <d3d9.h>
#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include <string>

namespace ild::qa
{
namespace
{
X86Detour end_hook;
using End = void(__thiscall*)(void*);
End original_end{};
std::filesystem::path output_root;
std::string requested;

void capture()
{
    const auto base = reinterpret_cast<const unsigned char*>(GetModuleHandleW(nullptr));
    const auto device = *reinterpret_cast<IDirect3DDevice9* const*>(base + 0x10BFF0);
    if (!device) return;
    Microsoft::WRL::ComPtr<IDirect3DSurface9> back, staging;
    if (FAILED(device->GetRenderTarget(0, back.GetAddressOf()))) return;
    D3DSURFACE_DESC desc{};
    if (FAILED(back->GetDesc(&desc)) || desc.Width > 8192 || desc.Height > 8192 ||
        (desc.Format != D3DFMT_A8R8G8B8 && desc.Format != D3DFMT_X8R8G8B8)) return;
    if (FAILED(device->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format,
            D3DPOOL_SYSTEMMEM, staging.GetAddressOf(), nullptr)) ||
        FAILED(device->GetRenderTargetData(back.Get(), staging.Get()))) return;
    D3DLOCKED_RECT lock{};
    if (FAILED(staging->LockRect(&lock, nullptr, D3DLOCK_READONLY))) return;
    BITMAPFILEHEADER header{};
    BITMAPINFOHEADER info{};
    header.bfType = 0x4D42;
    header.bfOffBits = sizeof(header) + sizeof(info);
    header.bfSize = header.bfOffBits + desc.Width * desc.Height * 4;
    info.biSize = sizeof(info);
    info.biWidth = static_cast<LONG>(desc.Width);
    info.biHeight = -static_cast<LONG>(desc.Height);
    info.biPlanes = 1;
    info.biBitCount = 32;
    std::ofstream output(output_root / ("ui-" + requested + ".bmp"), std::ios::binary);
    output.write(reinterpret_cast<const char*>(&header), sizeof(header));
    output.write(reinterpret_cast<const char*>(&info), sizeof(info));
    for (UINT row = 0; row < desc.Height; ++row)
        output.write(static_cast<const char*>(lock.pBits) + row * lock.Pitch, desc.Width * 4);
    staging->UnlockRect();
    requested.clear();
}

void __fastcall before_end(void* device, void*)
{
    if (!requested.empty()) capture();
    original_end(device);
}
}

void install_capture(const std::filesystem::path& root)
{
    output_root = root / L".ild-fixes" / L"runtime";
    const auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    constexpr unsigned char expected[]{0x51, 0x83, 0x3D, 0x08, 0xC0, 0x50, 0x00, 0x00};
    if (!end_hook.prepare(base + 0x832F0, reinterpret_cast<void*>(&before_end), expected)) return;
    original_end = reinterpret_cast<End>(end_hook.original());
    if (!end_hook.enable()) return;

    // QA renders inactive windows without activating input or taking desktop focus.
    auto gate = base + 0x83D6D;
    if (gate[0] != 0x74 || gate[1] != 0x5F) return;
    DWORD protection{};
    if (!VirtualProtect(gate, 2, PAGE_EXECUTE_READWRITE, &protection)) return;
    gate[0] = gate[1] = 0x90;
    FlushInstructionCache(GetCurrentProcess(), gate, 2);
    DWORD ignored{};
    VirtualProtect(gate, 2, protection, &ignored);
}

void request_capture(const std::string& state)
{
    if (state == "available" || state == "ready" || state == "last-page") requested = state;
}
}
