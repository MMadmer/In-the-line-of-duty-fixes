#pragma once

#include <filesystem>

namespace ild
{
// The engine only maps a file it is asked to read once the file is large; anything smaller is read straight
// into a private buffer. The config repairs ride on CreateFileMapping, so none of them ever reached a small
// file. This catches every reader the file system builds instead, and rewrites the private buffer in place.
[[nodiscard]] bool install_reader_repairs(const std::filesystem::path& root);

#ifdef ILD_CONSOLE_QA
[[nodiscard]] int reader_repairs_applied();
#endif
}
