//
// Created by nineball on 7/11/21.
//

#ifndef NINEENGINE_MESH_H
#define NINEENGINE_MESH_H
#include <string>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "Types.h"


struct VertexInputDescription {

    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 uv;

    static VertexInputDescription get_vertex_description();

    bool operator==(const Vertex& other) const {
        return position == other.position && normal == other.normal && color == other.color && uv == other.uv;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^
                    (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
                    ((hash<glm::vec3>()(vertex.color) ^
                    (hash<glm::vec2>()(vertex.uv) << 1)) >> 1);
                    //Shitty hash combiner... should be close enough whatever.
        }
    };
}



struct Mesh {
    std::vector<Vertex> mVertices {};
    AllocatedBuffer mVertexBuffer;

    std::vector<uint32_t> mIndices {};
    AllocatedBuffer mIndexBuffer;

    std::string texture;
};

struct MeshGroup {
    std::vector<Mesh> mMeshes;
    std::vector<bool> enabled;
    bool load_objects_from_file(std::string filename);
};

struct RenderObject {
    MeshGroup* meshGroup;
    Material* material;

    glm::mat4 transformMatrix;
};

struct MeshPushConstants {
    glm::vec4 data;
    glm::mat4 render_matrix;
};




#endif //NINEENGINE_MESH_H
