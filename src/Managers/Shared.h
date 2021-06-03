//
// Created by nineball on 6/2/21.
//

#ifndef NINEENGINE_SHARED_H
#define NINEENGINE_SHARED_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>

#include <set>
#include <cstdint>
#include <vector>


using Entity = size_t;

class System
{
public:
    std::set<Entity> mEntities;
};

struct Transform
{
    glm::vec3 position;
    glm::vec3 rotation;
    int scale;
};

struct Forces {
    glm::vec3 velocity;
    glm::vec3 acceleration;
};

struct VkRenderable
{
    ///Generic buffers
    VkDeviceMemory indexBufferMemory;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkBuffer vertexBuffer;
    ///Uniform buffers
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    ///Texture
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
};




#endif //NINEENGINE_SHARED_H
