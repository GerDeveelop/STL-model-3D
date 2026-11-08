#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace stlcut {
struct Vec3 { double x{}, y{}, z{}; };
struct Triangle { std::array<Vec3, 3> vertices; };
struct Bounds {
    Vec3 min;
    Vec3 max;
    [[nodiscard]] Vec3 dimensions() const
    { return {max.x - min.x, max.y - min.y, max.z - min.z}; }
};
struct Mesh {
    std::vector<Triangle> triangles;
    Bounds bounds;
    std::size_t degenerateTriangles{};
    std::string format;
    std::string units{"unspecified"};
    std::vector<std::string> warnings;
};
}
