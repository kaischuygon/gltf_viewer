// Basic reader and data types for the glTF scene format.
//
// Author: Fredrik Nysjo (2021)
//

#include "gltf_io.h"

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

namespace json = rapidjson;  // Use shorter alias for namespace

namespace gltf {

static bool load_file_to_bytebuffer(const std::string &filename, std::vector<char> &buffer)
{
    FILE *stream = std::fopen(filename.c_str(), "rb+");
    if (!stream) {
        std::cerr << "Error: Could not open " << filename << std::endl;
        return false;
    }

    const int BUFFER_SIZE_MAX = 32000000;  // FIXME
    buffer.resize(BUFFER_SIZE_MAX);
    buffer.resize(std::fread(&buffer[0], sizeof(char), BUFFER_SIZE_MAX, stream));
    assert(buffer.size() <= BUFFER_SIZE_MAX);
    std::fclose(stream);
    return true;
}

static std::vector<Node> create_nodes_from_json(const json::Value &value)
{
    std::vector<Node> nodes(value.Size());
    for (unsigned i = 0; i < value.Size(); ++i) {
        nodes[i].mesh = value[i]["mesh"].GetInt();
        nodes[i].name = value[i]["name"].GetString();
        if (value[i].HasMember("translation")) {
            const json::Value &tmp = value[i]["translation"];
            for (unsigned j = 0; j < tmp.Size(); ++j) {
                nodes[i].translation[j] = float(tmp[j].GetDouble());
            }
        } else {
            nodes[i].translation = glm::vec3(0.0f);
        }
        if (value[i].HasMember("rotation")) {
            const json::Value &tmp = value[i]["rotation"];
            for (unsigned j = 0; j < tmp.Size(); ++j) {
                nodes[i].rotation[j] = float(tmp[j].GetDouble());
            }
        } else {
            nodes[i].rotation = glm::quat();
        }
        if (value[i].HasMember("scale")) {
            const json::Value &tmp = value[i]["scale"];
            for (unsigned j = 0; j < tmp.Size(); ++j) {
                nodes[i].scale[j] = float(tmp[j].GetDouble());
            }
        } else {
            nodes[i].scale = glm::vec3(1.0f);
        }
        nodes[i].hasMatrix = false;
        if (value[i].HasMember("matrix")) {
            const json::Value &tmp = value[i]["matrix"];
            for (unsigned j = 0; j < tmp.Size(); ++j) {
                // Note: matrix is stored in column major array order
                nodes[i].matrix[j / 4][j % 4] = float(tmp[j].GetDouble());
            }
            nodes[i].hasMatrix = true;
        } else {
            nodes[i].matrix = glm::mat4(1.0f);
        }
    }
    return nodes;
}

static PBRMetallicRoughness create_pbr_metallic_roughness_from_json(const json::Value &value)
{
    PBRMetallicRoughness pbrMetallicRoughness;
    if (value.HasMember("baseColorFactor")) {
        const json::Value &tmp = value["baseColorFactor"];
        for (unsigned i = 0; i < tmp.Size(); ++i) {
            pbrMetallicRoughness.baseColorFactor[i] = float(tmp[i].GetDouble());
        }
    } else {
        pbrMetallicRoughness.baseColorFactor = glm::vec4(1.0f);
    }
    if (value.HasMember("roughnessFactor")) {
        pbrMetallicRoughness.roughnessFactor = float(value["roughnessFactor"].GetDouble());
    } else {
        pbrMetallicRoughness.roughnessFactor = 1.0f;
    }
    if (value.HasMember("metallicFactor")) {
        pbrMetallicRoughness.metallicFactor = float(value["metallicFactor"].GetDouble());
    } else {
        pbrMetallicRoughness.metallicFactor = 0.0f;
    }
    return pbrMetallicRoughness;
}

static std::vector<Material> create_materials_from_json(const json::Value &value)
{
    std::vector<Material> materials(value.Size());
    for (unsigned i = 0; i < value.Size(); ++i) {
        materials[i].name = value[i]["name"].GetString();
        materials[i].type = DEFAULT_MATERIAL;
        if (value[i].HasMember("pbrMetallicRoughness")) {
            auto pbrMetallicRoughness =
                create_pbr_metallic_roughness_from_json(value[i]["pbrMetallicRoughness"]);
            materials[i].pbrMetallicRoughness = pbrMetallicRoughness;
            materials[i].type = PBR_METALLIC_ROUGHNESS;
        }
    }
    return materials;
}

static std::vector<Primitive> create_primitives_from_json(const json::Value &value)
{
    std::vector<Primitive> primitives(value.Size());
    for (unsigned i = 0; i < value.Size(); ++i) {
        for (const auto &it : value[i]["attributes"].GetObject()) {
            Attribute attribute = {it.name.GetString(), it.value.GetInt()};
            primitives[i].attributes.push_back(attribute);
        }
        primitives[i].indices = value[i]["indices"].GetInt();
        if (value[i].HasMember("material")) {
            primitives[i].material = value[i]["material"].GetInt();
        }
    }
    return primitives;
}

static std::vector<Mesh> create_meshes_from_json(const json::Value &value)
{
    std::vector<Mesh> meshes(value.Size());
    for (unsigned i = 0; i < value.Size(); ++i) {
        meshes[i].name = value[i]["name"].GetString();
        if (value[i].HasMember("primitives")) {
            auto primitives = create_primitives_from_json(value[i]["primitives"]);
            meshes[i].primitives = primitives;
        }
    }
    return meshes;
}

static std::vector<Accessor> create_accessors_from_json(const json::Value &value)
{
    std::vector<Accessor> accessors(value.Size());
    for (unsigned i = 0; i < value.Size(); ++i) {
        accessors[i].bufferView = value[i]["bufferView"].GetInt();
        accessors[i].componentType = value[i]["componentType"].GetInt();
        accessors[i].count = value[i]["count"].GetInt();
        accessors[i].type = value[i]["type"].GetString();
    }
    return accessors;
}

static std::vector<BufferView> create_buffer_views_from_json(const json::Value &value)
{
    std::vector<BufferView> bufferViews(value.Size());
    for (unsigned i = 0; i < value.Size(); ++i) {
        bufferViews[i].buffer = value[i]["buffer"].GetInt();
        bufferViews[i].byteLength = value[i]["byteLength"].GetInt();
        bufferViews[i].byteOffset = value[i]["byteOffset"].GetInt();
    }
    return bufferViews;
}

static std::vector<Buffer> create_buffers_from_json(const json::Value &value)
{
    std::vector<Buffer> buffers(value.Size());
    for (unsigned i = 0; i < value.Size(); ++i) {
        buffers[i].byteLength = value[i]["byteLength"].GetInt();
        buffers[i].uri = value[i]["uri"].GetString();
    }
    return buffers;
}

bool load_gltf_asset(const std::string &filename, const std::string &filedir, GLTFAsset &asset)
{
    json::Document root;
    std::vector<char> buffer;
    if (!load_file_to_bytebuffer(filedir + filename, buffer)) {
        std::cerr << "Error: Could not open " << filename << std::endl;
        return false;
    }
    root.Parse(&buffer[0]);

    asset = GLTFAsset();

    if (root.HasMember("nodes")) {
        auto nodes = create_nodes_from_json(root["nodes"]);
        asset.nodes = nodes;
    }

    if (root.HasMember("materials")) {
        auto materials = create_materials_from_json(root["materials"]);
        asset.materials = materials;
    }

    if (root.HasMember("meshes")) {
        auto meshes = create_meshes_from_json(root["meshes"]);
        asset.meshes = meshes;
    }

    if (root.HasMember("accessors")) {
        auto accessors = create_accessors_from_json(root["accessors"]);
        asset.accessors = accessors;
    }

    if (root.HasMember("bufferViews")) {
        auto bufferViews = create_buffer_views_from_json(root["bufferViews"]);
        asset.bufferViews = bufferViews;
    }

    if (root.HasMember("buffers")) {
        auto buffers = create_buffers_from_json(root["buffers"]);
        // Now also load the actual buffer data (from .bin files)
        for (unsigned i = 0; i < buffers.size(); ++i) {
            load_file_to_bytebuffer(filedir + buffers[i].uri, buffers[i].data);
        }
        asset.buffers = buffers;
    }

    return true;
}

}  // namespace gltf
