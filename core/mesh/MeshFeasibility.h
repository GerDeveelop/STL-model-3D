#pragma once
#include "Mesh.h"
#include <stop_token>
namespace stlcut {
struct InstanceFeasibility {
    std::uint32_t instance{};
    std::size_t triangles{},surfaceFacets{},unresolvedFacets{},materials{},components{},degenerate{},boundaryEdges{},nonmanifoldEdges{},inconsistentEdges{};
    bool topologyAnalyzed{},closedOriented{},positiveVolume{},currentReconstructionBudget{};
    double volume{};
    std::string reason;
};
// Screening using exact coordinate welding; does not replace tolerant Manifold
// validation, certify no self-intersections, repair, or split a physical body.
std::vector<InstanceFeasibility> meshFeasibility(const Mesh&,std::stop_token = {});
}
