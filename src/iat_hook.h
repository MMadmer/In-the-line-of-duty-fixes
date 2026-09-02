#pragma once

#include <Windows.h>

namespace ild
{
[[nodiscard]] void* replace_iat_import(
    HMODULE module,
    const char* imported_module,
    const char* imported_function,
    void* replacement);
}
