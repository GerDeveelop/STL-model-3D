#pragma once
#include "StlReader.h"
#include <span>

namespace stlcut {
Mesh readMesh(std::span<const unsigned char> data, std::stop_token stop = {});
Mesh readMesh(const std::filesystem::path& path, std::stop_token stop = {});
void updateBounds(Mesh& mesh);
}
