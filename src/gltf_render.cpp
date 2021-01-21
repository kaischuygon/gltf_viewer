// Basic reader and data types for the glTF scene format.
//
// Author: Fredrik Nysjo (2021)
//

#include "gltf_render.h"

namespace gltf {

void create_drawable_from_gltf_asset(Drawable &drawable, const GLTFAsset &asset)
{
    glGenBuffers(1, &drawable.buffer);
    glBindBuffer(GL_COPY_WRITE_BUFFER, drawable.buffer);
    glBufferData(GL_COPY_WRITE_BUFFER, asset.buffers[0].byteLength, &asset.buffers[0].data[0],
                 GL_STATIC_DRAW);

    glGenVertexArrays(1, &drawable.vao);
    glBindVertexArray(drawable.vao);
    {
        glBindBuffer(GL_ARRAY_BUFFER, drawable.buffer);

        const Primitive &primitive = asset.meshes[0].primitives[0];
        for (const auto &it : primitive.attributes) {
            const Accessor &accessor = asset.accessors[it.index];
            const BufferView &bufferView = asset.bufferViews[accessor.bufferView];
            if (it.name.compare("POSITION") == 0) {
                glEnableVertexAttribArray(POSITION);
                // Note: we often declare the position attribute as vec4 in the
                // vertex shader, even if the actual type in the buffer is
                // vec3. This is valid and will give us a homogenous coordinate
                // with the last component assigned the value 1.
                glVertexAttribPointer(POSITION, 3 /*VEC3*/, accessor.componentType, GL_FALSE, 0,
                                      (GLvoid *)(intptr_t)bufferView.byteOffset);
            } else if (it.name.compare("COLOR_0") == 0) {
                glEnableVertexAttribArray(COLOR_0);
                glVertexAttribPointer(COLOR_0, 4 /*VEC4*/, accessor.componentType, GL_FALSE, 0,
                                      (GLvoid *)(intptr_t)bufferView.byteOffset);
            } else if (it.name.compare("NORMAL") == 0) {
                glEnableVertexAttribArray(NORMAL);
                glVertexAttribPointer(NORMAL, 3 /*VEC3*/, accessor.componentType, GL_FALSE, 0,
                                      (GLvoid *)(intptr_t)bufferView.byteOffset);
            } else if (it.name.compare("TEXCOORD_0") == 0) {
                glEnableVertexAttribArray(TEXCOORD_0);
                glVertexAttribPointer(TEXCOORD_0, 2 /*VEC2*/, accessor.componentType, GL_FALSE, 0,
                                      (GLvoid *)(intptr_t)bufferView.byteOffset);
            }
            // You can add more attributes (TEXCOORD_1, TEXCOORD_2, etc.) here...
        }
    }
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.buffer);

        const Primitive &primitive = asset.meshes[0].primitives[0];
        const Accessor &accessor = asset.accessors[primitive.indices];
        const BufferView &bufferView = asset.bufferViews[accessor.bufferView];
        drawable.indexCount = accessor.count;
        drawable.indexType = accessor.componentType;
        drawable.indexByteOffset = bufferView.byteOffset;
    }
    glBindVertexArray(0);
}

void destroy_drawable(Drawable &drawable)
{
    glDeleteBuffers(1, &drawable.buffer);
    glDeleteVertexArrays(1, &drawable.vao);
}

void create_drawables_from_gltf_asset(DrawableList &drawables, const GLTFAsset &asset)
{
    // First clean up existing OpenGL resources
    destroy_drawables(drawables);

    // Create vertex buffer
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_COPY_WRITE_BUFFER, buffer);
    assert(asset.buffers.size() == 1);
    glBufferData(GL_COPY_WRITE_BUFFER, asset.buffers[0].byteLength, &asset.buffers[0].data[0],
                 GL_STATIC_DRAW);

    // Create one vertex array object per mesh/drawable
    drawables.resize(asset.meshes.size());
    for (unsigned i = 0; i < asset.meshes.size(); ++i) {
        assert(asset.meshes[i].primitives.size() == 1);
        drawables[i].buffer = buffer;

        glGenVertexArrays(1, &drawables[i].vao);
        glBindVertexArray(drawables[i].vao);

        // Specify vertex format
        glBindBuffer(GL_ARRAY_BUFFER, drawables[i].buffer);
        const Primitive &primitive = asset.meshes[i].primitives[0];
        for (const auto &it : primitive.attributes) {
            const Accessor &accessor = asset.accessors[it.index];
            const BufferView &bufferView = asset.bufferViews[accessor.bufferView];

            if (it.name.compare("POSITION") == 0) {
                glEnableVertexAttribArray(POSITION);
                // Note: we often declare the position attribute as vec4 in the
                // vertex shader, even if the actual type in the buffer is
                // vec3. This is valid and will give us a homogenous coordinate
                // with the last component assigned the value 1.
                glVertexAttribPointer(POSITION, 3 /*VEC3*/, accessor.componentType, GL_FALSE, 0,
                                      (GLvoid *)(intptr_t)bufferView.byteOffset);
            } else if (it.name.compare("COLOR_0") == 0) {
                glEnableVertexAttribArray(COLOR_0);
                glVertexAttribPointer(COLOR_0, 4 /*VEC4*/, accessor.componentType, GL_FALSE, 0,
                                      (GLvoid *)(intptr_t)bufferView.byteOffset);
            } else if (it.name.compare("NORMAL") == 0) {
                glEnableVertexAttribArray(NORMAL);
                glVertexAttribPointer(NORMAL, 3 /*VEC3*/, accessor.componentType, GL_FALSE, 0,
                                      (GLvoid *)(intptr_t)bufferView.byteOffset);
            } else if (it.name.compare("TEXCOORD_0") == 0) {
                glEnableVertexAttribArray(TEXCOORD_0);
                glVertexAttribPointer(TEXCOORD_0, 2 /*VEC2*/, accessor.componentType, GL_FALSE, 0,
                                      (GLvoid *)(intptr_t)bufferView.byteOffset);
            }
            // You can add support for more named attributes here...
        }

        // Specify index format
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawables[i].buffer);
        const Accessor &accessor = asset.accessors[primitive.indices];
        const BufferView &bufferView = asset.bufferViews[accessor.bufferView];
        drawables[i].indexCount = accessor.count;
        drawables[i].indexType = accessor.componentType;
        drawables[i].indexByteOffset = bufferView.byteOffset;
    }
    glBindVertexArray(0);
}

void destroy_drawables(DrawableList &drawables)
{
    for (unsigned i = 0; i < drawables.size(); ++i) {
        glDeleteBuffers(1, &drawables[i].buffer);
        glDeleteVertexArrays(1, &drawables[i].vao);
    }
    drawables.clear();
}

}  // namespace gltf
