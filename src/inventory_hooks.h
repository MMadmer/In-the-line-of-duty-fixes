#pragma once
#include <filesystem>

namespace ild
{
bool install_inventory_hooks(const std::filesystem::path& root);
#ifdef ILD_CONSOLE_QA
const char* qa_inventory_swap(unsigned id);
#endif
}
