#pragma once

#include <Windows.h>

#include <array>
#include <filesystem>
#include <span>

namespace ild
{
using Sha256 = std::array<std::byte, 32>;

[[nodiscard]] bool sha256_file(const std::filesystem::path& path, Sha256& digest);
[[nodiscard]] bool sha256_bytes(std::span<const std::byte> bytes, Sha256& digest);
}
