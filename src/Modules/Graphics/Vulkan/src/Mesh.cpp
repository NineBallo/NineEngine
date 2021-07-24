//
// Created by nineball on 7/11/21.
//

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream>
#include <unordered_map>
#include "Mesh.h"


VertexInputDescription Vertex::get_vertex_description()
{
    VertexInputDescription description;

    //we will have just 1 vertex buffer binding, with a per-vertex rate
    VkVertexInputBindingDescription mainBinding = {};
    mainBinding.binding = 0;
    mainBinding.stride = sizeof(Vertex);
    mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    description.bindings.push_back(mainBinding);

    //Position will be stored at Location 0
    VkVertexInputAttributeDescription positionAttribute = {};
    positionAttribute.binding = 0;
    positionAttribute.location = 0;
    positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    positionAttribute.offset = offsetof(Vertex, position);

    //Normal will be stored at Location 1
    VkVertexInputAttributeDescription normalAttribute = {};
    normalAttribute.binding = 0;
    normalAttribute.location = 1;
    normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    normalAttribute.offset = offsetof(Vertex, normal);

    //Color will be stored at Location 2
    VkVertexInputAttributeDescription colorAttribute = {};
    colorAttribute.binding = 0;
    colorAttribute.location = 2;
    colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    colorAttribute.offset = offsetof(Vertex, color);

    //UV will be stored at Location 3
    VkVertexInputAttributeDescription uvAttribute = {};
    uvAttribute.binding = 0;
    uvAttribute.location = 3;
    uvAttribute.format = VK_FORMAT_R32G32_SFLOAT;
    uvAttribute.offset = offsetof(Vertex, uv);

    description.attributes.push_back(positionAttribute);
    description.attributes.push_back(normalAttribute);
    description.attributes.push_back(colorAttribute);
    description.attributes.push_back(uvAttribute);

    return description;
}






bool Mesh::load_from_obj(std::string filename)
{
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = "./models/";

    tinyobj::ObjReader reader;

    reader.ParseFromFile(filename, reader_config);

    if (!reader.Warning().empty()) {
        std::cerr << reader.Warning() << std::endl;
    }

    //vertex arrays of file
    auto& attrib = reader.GetAttrib();
    //each object in file
    auto& shapes = reader.GetShapes();
    //Material of shape
    auto& materials = reader.GetMaterials();


    // tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, filename.c_str(), nullptr, true);

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (auto & shape : shapes) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            auto fv = size_t(shape.mesh.num_face_vertices[f]);


            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                Vertex new_vert {};

                // access to vertex
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                //vertex position
                tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];

                new_vert.position.x = vx;
                new_vert.position.y = vy;
                new_vert.position.z = vz;


                //vertex normal
                if (idx.normal_index >= 0) {
                    tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                    tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                    tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

                    new_vert.normal.x = nx;
                    new_vert.normal.y = ny;
                    new_vert.normal.z = nz;
                }
                //vertex uv
                if (idx.texcoord_index >= 0) {
                    tinyobj::real_t ux = attrib.texcoords[2 * idx.texcoord_index + 0];
                    tinyobj::real_t uy = attrib.texcoords[2 * idx.texcoord_index + 1];

                    new_vert.uv.x = ux;
                    new_vert.uv.y = 1-uy;
                }

                //Optional: vertex colors
                //tinyobj::real_t red   = attrib.colors[3 * idx.vertex_index + 0];
                //tinyobj::real_t green = attrib.colors[3 * idx.vertex_index + 1];
                //tinyobj::real_t blue  = attrib.colors[3 * idx.vertex_index + 2];

                new_vert.color.x = 1.f;
                new_vert.color.y = 0.5f;
                new_vert.color.z = 1.f;

                if(uniqueVertices.count(new_vert) == 0) {
                    //Add it to logging(?) array, then add it to official array
                    uniqueVertices[new_vert] = mVertices.size();
                    mVertices.push_back(new_vert);
                }
                mIndices.push_back(uniqueVertices[new_vert]);
            }
            index_offset += fv;
        }
    }

    return true;
}