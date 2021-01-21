// Basic reader and data types for the glTF scene format.
//
// Author: Fredrik Nysjo (2021)
//

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <vector>

namespace gltf {

enum MaterialType { DEFAULT_MATERIAL = 0, PBR_METALLIC_ROUGHNESS = 1 };

struct Scene {
    std::string name;
    std::vector<int> nodes;
};

struct Node {
    int mesh;
    std::string name;
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
    glm::mat4 matrix;
    bool hasMatrix;
};

struct PBRMetallicRoughness {
    glm::vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
};

struct Material {
    std::string name;
    MaterialType type;
    PBRMetallicRoughness pbrMetallicRoughness;
};

struct Attribute {
    std::string name;
    int index;
};

struct Primitive {
    std::vector<Attribute> attributes;
    int indices;
    int material;
};

struct Mesh {
    std::string name;
    std::vector<Primitive> primitives;
};

struct Accessor {
    int bufferView;
    int componentType;
    int count;
    std::string type;
};

struct BufferView {
    int buffer;
    int byteLength;
    int byteOffset;
    int byteStride;
};

struct Buffer {
    int byteLength;
    std::string uri;
    std::vector<char> data;
};

struct GLTFAsset {
    std::vector<Scene> scenes;
    std::vector<Node> nodes;
    std::vector<Material> materials;
    std::vector<Mesh> meshes;
    std::vector<Accessor> accessors;
    std::vector<BufferView> bufferViews;
    std::vector<Buffer> buffers;
};

}  // namespace gltf
