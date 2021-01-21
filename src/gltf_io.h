// Basic reader and data types for the glTF scene format.
//
// Author: Fredrik Nysjo (2021)
//

#pragma once

#include "gltf_scene.h"

#include <string>

namespace gltf {

bool load_gltf_asset(const std::string &filename, const std::string &filedir, GLTFAsset &asset);

}  // namespace gltf
