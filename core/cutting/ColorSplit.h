#pragma once
#include "PlaneCut.h"
#include <stop_token>
namespace stlcut {
// Exact separation of already closed, uniformly labelled material regions.
// Surface painting requires a separate volumetric reconstruction policy.
std::vector<Mesh> splitByMaterial(const Mesh& mesh,std::stop_token stop = {},const ConnectorSpec* connector = nullptr);
}
