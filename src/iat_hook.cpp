#include "iat_hook.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ild
{
void* replace_iat_import(
    HMODULE module,
    const char* imported_module,
    const char* imported_function,
    void* replacement)
{
    if (!module || !imported_module || !imported_function || !replacement)
    {
        return nullptr;
    }

    const auto base = reinterpret_cast<std::byte*>(module);
    const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
    {
        return nullptr;
    }

    const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
    {
        return nullptr;
    }

    const auto& directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!directory.VirtualAddress)
    {
        return nullptr;
    }

    auto descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + directory.VirtualAddress);
    for (; descriptor->Name; ++descriptor)
    {
        const auto module_name = reinterpret_cast<const char*>(base + descriptor->Name);
        if (_stricmp(module_name, imported_module) != 0 || !descriptor->OriginalFirstThunk)
        {
            continue;
        }

        auto names = reinterpret_cast<IMAGE_THUNK_DATA*>(base + descriptor->OriginalFirstThunk);
        auto addresses = reinterpret_cast<IMAGE_THUNK_DATA*>(base + descriptor->FirstThunk);
        for (; names->u1.AddressOfData; ++names, ++addresses)
        {
            if (IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal))
            {
                continue;
            }

            const auto import = reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(base + names->u1.AddressOfData);
            if (std::strcmp(reinterpret_cast<const char*>(import->Name), imported_function) != 0)
            {
                continue;
            }

            DWORD old_protect{};
            if (!VirtualProtect(&addresses->u1.Function, sizeof(addresses->u1.Function), PAGE_READWRITE, &old_protect))
            {
                return nullptr;
            }

            const auto original = reinterpret_cast<void*>(addresses->u1.Function);
            addresses->u1.Function = reinterpret_cast<ULONG_PTR>(replacement);
            FlushInstructionCache(GetCurrentProcess(), &addresses->u1.Function, sizeof(addresses->u1.Function));

            DWORD ignored{};
            VirtualProtect(&addresses->u1.Function, sizeof(addresses->u1.Function), old_protect, &ignored);
            return original;
        }
    }

    return nullptr;
}
}
