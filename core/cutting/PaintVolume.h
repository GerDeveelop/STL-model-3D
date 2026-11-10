#pragma once
#include "core/mesh/Mesh.h"
#include "RegionConnectors.h"
#include <stop_token>
namespace stlcut {
enum class PaintVolumeMode { Inserts, Partition };
struct PaintVolumeSpec {
    PaintVolumeMode mode{PaintVolumeMode::Partition};
    double resolution{1}; // Maximum cell edge, in model units.
    double depth{2}; // Inserts only: nearest exterior distance at cell center.
};
// Approximate internal interfaces; clip cells to the unchanged source exterior.
// Throws before publishing if labels, budgets, topology or export are invalid.
std::vector<Mesh> splitPaintVolume(const Mesh&,const PaintVolumeSpec&,std::stop_token = {},
                                 const ConnectorSpec* = nullptr,std::vector<RegionConnection>* = nullptr);
}
