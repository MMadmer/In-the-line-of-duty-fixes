#pragma once

#include <Windows.h>
#ifdef ILD_CONSOLE_QA
#include <filesystem>
#endif

namespace ild
{
[[nodiscard]] bool install_console_hooks(HMODULE engine);
#ifdef ILD_CONSOLE_QA
void run_console_selftest(HMODULE engine, const std::filesystem::path& report);
#endif
}
