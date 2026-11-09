#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace stlcut {
struct Vec3 { double x{}, y{}, z{}; };
struct Triangle { std::array<Vec3, 3> vertices; };
struct Material {
    std::uint32_t resource{},property{};
    std::array<std::uint8_t,4> rgba{};
    std::string name;
};
struct TriangleStyle {
    std::uint32_t instance{};
    std::array<std::int32_t,3> material{-1,-1,-1};
};
struct SourceInstance {
    std::uint32_t id{},object{},firstTriangle{},triangleCount{};
    std::string name;
    std::string partPath;
    std::uint32_t modelObject{},buildObject{};
    bool reflected{};
    std::vector<std::uint32_t> objectHierarchy;
};
struct PaintFacet {Triangle barycentric;std::uint32_t sourceTriangle{};std::int32_t material{-1};std::uint16_t filament{};};
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
    std::vector<Material> materials;
    std::vector<TriangleStyle> styles;
    std::vector<SourceInstance> instances;
    bool unsupportedAppearance{};
    std::vector<PaintFacet> paintFacets;
    std::size_t paintedTriangles{},unresolvedPaintFacets{};
};
}
