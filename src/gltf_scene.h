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
    std::vector<int> children;
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
    glm::mat4 matrix;
    bool hasMatrix;
};

struct MaterialTexture {
    int index;
    int texCoord;
    float scale;     // Only used by normal map textures
    float strength;  // Only used by occlusion textures
};

struct PBRMetallicRoughness {
    glm::vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    MaterialTexture baseColorTexture;
    MaterialTexture metallicRoughnessTexture;
    bool hasBaseColorTexture;
    bool hasMetallicRoughnessTexture;
};

struct Material {
    std::string name;
    MaterialType type;
    PBRMetallicRoughness pbrMetallicRoughness;
    MaterialTexture normalTexture;
    MaterialTexture occlusionTexture;
    bool hasNormalTexture;
    bool hasOcclusionTexture;
};

struct Texture {
    int source;
    int sampler;
    bool hasSampler;
};

struct Image {
    std::string uri;
    int width;               // Image width (in pixels)
    int height;              // Image height (in pixels)
    std::vector<char> data;  // Pixel data in RGBA8 format
};

struct Sampler {
    int magFilter;
    int minFilter;
    int wrapS;
    int wrapT;
};

struct Attribute {
    std::string name;
    int index;
};

struct Primitive {
    std::vector<Attribute> attributes;
    int indices;
    int material;
    bool hasMaterial;
};

struct Mesh {
    std::string name;
    std::vector<Primitive> primitives;
};

struct Accessor {
    int bufferView;
    int componentType;
    int count;
    int byteOffset;
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
    std::vector<Texture> textures;
    std::vector<Image> images;
    std::vector<Sampler> samplers;
    std::vector<Mesh> meshes;
    std::vector<Accessor> accessors;
    std::vector<BufferView> bufferViews;
    std::vector<Buffer> buffers;
};

}  // namespace gltf
