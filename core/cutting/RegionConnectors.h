#pragma once
#include "PlaneCut.h"
#include <string>
namespace stlcut {
struct RegionConnection {
    std::size_t first{},second{},donor{},receiver{}; // zero-based result indices
    bool connected{};
    Vec3 position,normal; // pin starts on interface, normal points into receiver
    double addedVolume{},removedVolume{};
    std::string reason;
};
struct RegionConnectorResult {
    std::vector<Mesh> pieces;
    std::vector<RegionConnection> connections;
};
// Bounded local contact search; independent input, no partial mutations.
// A skipped candidate pair is reported, not silently resized or forced.
RegionConnectorResult connectRegions(const std::vector<Mesh>&,const ConnectorSpec&,std::stop_token = {});
}
