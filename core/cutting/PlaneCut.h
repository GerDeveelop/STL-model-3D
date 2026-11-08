#pragma once
#include "core/mesh/Mesh.h"
#include <stop_token>
namespace stlcut {
// Plane: dot(normal, position) = offset, normal is normalized internally.
struct Plane { Vec3 normal{0,0,1}; double offset{}; };
struct CutResult { Mesh positive; Mesh negative; double originalVolume{},positiveVolume{},negativeVolume{},tolerance{}; };
CutResult cutByPlane(const Mesh& mesh, Plane plane, std::stop_token stop = {});
double signedVolume(const Mesh& mesh);
void verifyClosedMesh(const Mesh& mesh, double epsilon, std::stop_token stop = {});
}
