#pragma once
#include "core/mesh/Mesh.h"
#include <ostream>
#include <stop_token>
#include <string>
namespace stlcut {
// Little-endian binary STL. Caller owns atomic destination replacement.
void writeStl(std::ostream& out, const Mesh& mesh, std::stop_token stop = {});
// Returns bytes only after round-trip precision/volume checks. Cut pieces must
// also remain closed and oriented. Does not repair geometry or write a file.
std::string validatedStl(const Mesh& mesh, bool requireClosed, std::stop_token stop = {});
}
