#pragma once

#include "core/mesh/Mesh.h"

#include <cstdint>
#include <filesystem>
#include <istream>
#include <stop_token>

namespace stlcut {
inline constexpr std::uint64_t maxFileBytes = 256ULL * 1024 * 1024;
inline constexpr std::uint32_t maxTriangles = 2'000'000;

// Throws std::runtime_error for invalid data/resources and ImportCancelled on stop.
// Streams must be binary, seekable and positioned at the beginning of the file.
struct ImportCancelled {};
Mesh readStl(std::istream& input, std::uint64_t bytes, std::stop_token stop = {});
Mesh readStl(const std::filesystem::path& path, std::stop_token stop = {});
}
