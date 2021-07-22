//
// Created by nineball on 7/11/21.
//

#ifndef NINEENGINE_MESH_H
#define NINEENGINE_MESH_H
#include <string>
#include <vector>
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
};

struct Mesh {
    std::vector<Vertex> mVertices;
    AllocatedBuffer mVertexBuffer;

    bool load_from_obj(std::string filename);
};

struct RenderObject {
    Mesh* mesh;
    Material* material;
    Texture* texture;
    glm::mat4 transformMatrix;
};

struct MeshPushConstants {
    glm::vec4 data;
    glm::mat4 render_matrix;
};




#endif //NINEENGINE_MESH_H
