//
// Created by nineball on 7/11/21.
//

#include <iostream>
#include <unordered_map>
#include "Mesh.h"

///TODO move this to like IO or som and convert everything to a fast static format
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

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

bool MeshGroup::load_objects_from_file(std::string filename) {
    Assimp::Importer importer;

    //With assimp it will automatically strip the duplicate vertices for us
    const aiScene *scene = importer.ReadFile(filename,
                                             aiProcess_JoinIdenticalVertices         |
                                             aiProcess_ImproveCacheLocality          |
                                             aiProcess_Triangulate);

    if (!scene) {
        throw std::runtime_error(importer.GetErrorString());
        return false;
    }

    mTextures.reserve(scene->mNumMaterials);
    mTextures.resize(scene->mNumMaterials);

    mMatToIdx.reserve(scene->mNumMaterials);
    mMeshes.reserve(scene->mNumMeshes);



    for (uint32_t i = 0, currentTextureCount = 0; i < scene->mNumMeshes; i++) {

        aiMesh *mesh = scene->mMeshes[i];
        Mesh renderMesh{};


        aiString name = scene->mMaterials[mesh->mMaterialIndex]->GetName();
        if(!mMatToIdx.contains(name.data)) {
            mMatToIdx[name.data] = currentTextureCount;
            currentTextureCount++;
        }
        renderMesh.mMaterial = name.data;


        renderMesh.mVertices.reserve(mesh->mNumVertices);
        for (uint32_t vertexIdx = 0; vertexIdx < mesh->mNumVertices; vertexIdx++) {
            Vertex vertex{};


            aiVector3D vert = mesh->mVertices[vertexIdx];

            vertex.position.x = vert.x;
            vertex.position.y = vert.y;
            vertex.position.z = vert.z;

            if (mesh->HasNormals()) {
                aiVector3D norm = mesh->mNormals[vertexIdx];

                vertex.normal.x = norm.x;
                vertex.normal.y = norm.y;
                vertex.normal.z = norm.z;
            }

            if (mesh->mTextureCoords[0]) {
                glm::vec2 coords;
                coords.x = mesh->mTextureCoords[0][vertexIdx].x;
                coords.y = 1-mesh->mTextureCoords[0][vertexIdx].y;
                vertex.uv = coords;
            }

            renderMesh.mVertices.push_back(vertex);
        }

        //Get all indices
        renderMesh.mIndices.reserve(mesh->mNumFaces * 3);
        for (uint32_t face = 0; face < mesh->mNumFaces; face++) {
            renderMesh.mIndices.push_back(mesh->mFaces[face].mIndices[0]);
            renderMesh.mIndices.push_back(mesh->mFaces[face].mIndices[1]);
            renderMesh.mIndices.push_back(mesh->mFaces[face].mIndices[2]);
        }

        mMeshes.push_back(renderMesh);
        //enabled[i] = true;
    }
    return true;
}