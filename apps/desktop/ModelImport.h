#pragma once
#include "core/io/MeshReader.h"
// Qt XML/JSON adaptation lives outside the geometry core.
stlcut::Mesh readModel(std::span<const unsigned char> data,std::stop_token stop = {});
stlcut::Mesh readModel(const std::filesystem::path& path,std::stop_token stop = {});
