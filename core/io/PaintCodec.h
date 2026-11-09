#pragma once
#include "core/mesh/Mesh.h"
#include <stop_token>
#include <string_view>
namespace stlcut {
struct PaintLeaf {Triangle barycentric;std::uint16_t filament{};};
// Independent decoder of the paint_color/mmu_segmentation wire format.
// Geometry is barycentric over the unchanged source triangle; zero inherits.
std::vector<PaintLeaf> decodePaint(std::string_view code,std::stop_token stop = {});
}
