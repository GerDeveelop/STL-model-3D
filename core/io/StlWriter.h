#pragma once
#include "core/mesh/Mesh.h"
#include <ostream>
#include <stop_token>
namespace stlcut {
// Little-endian binary STL. Caller owns atomic destination replacement.
void writeStl(std::ostream& out, const Mesh& mesh, std::stop_token stop = {});
}
