// Basic reader and data types for the glTF scene format.
//
// Author: Fredrik Nysjo (2021)
//

#pragma once

#include "gltf_scene.h"

#include <GL/gl3w.h>

namespace gltf {

// Attribute locations we will use in vertex shaders
enum AttributeLocation { POSITION = 0, COLOR_0 = 1, NORMAL = 2, TEXCOORD_0 = 3 };

struct Drawable {
    GLuint vao;
    GLuint buffer;
    GLenum indexType;
    int indexCount;
    int indexByteOffset;
};

typedef std::vector<Drawable> DrawableList;

void create_drawables_from_gltf_asset(DrawableList &drawables, const GLTFAsset &asset);

void destroy_drawables(DrawableList &drawables);

}  // namespace gltf
