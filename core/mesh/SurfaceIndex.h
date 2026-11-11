#pragma once
#include "Mesh.h"
#include <optional>
#include <stop_token>
namespace stlcut {
struct SurfaceFacet {
    Triangle triangle; // normalized relative to SurfaceData origin/scale
    std::int32_t material{-1};
    std::uint32_t sourceTriangle{},instance{};
};
struct SurfaceData {std::vector<SurfaceFacet> facets;Vec3 origin;double scale{1};};
// Whole mesh or one provenance instance. No source mutations or simplification.
SurfaceData surfaceData(const Mesh&,std::optional<std::uint32_t> instance={},std::stop_token = {});
struct SurfaceWork {
    std::uint64_t limit{100000000},nodes{},triangles{};
};
struct SurfaceHit {
    double distance{};
    std::int32_t material{-1};
    std::uint32_t sourceTriangle{},instance{},facet{};
};
class SurfaceIndex {
    struct Node {Bounds bounds;std::uint32_t begin{},count{},left{},right{};};
    SurfaceData data_;
    std::vector<std::uint32_t> order_;
    std::vector<Node> nodes_;
    std::uint32_t build(std::uint32_t,std::uint32_t,std::stop_token);
public:
    explicit SurfaceIndex(SurfaceData,std::stop_token = {});
    SurfaceHit nearest(Vec3 worldPoint,SurfaceWork&,std::stop_token = {},bool bruteForce=false) const;
    std::size_t facets() const{return data_.facets.size();}
    std::size_t nodes() const{return nodes_.size();}
    std::size_t bytes() const;
};
}
