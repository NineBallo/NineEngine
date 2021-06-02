//
// Created by nineball on 5/30/21.
//

#ifndef NINEENGINE_ENTITYMANAGER_H
#define NINEENGINE_ENTITYMANAGER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "array"
#include "queue"
#include "../Graphics/Vulkan/Vulkan.h"

class Entity {
public:
    Entity();
    ~Entity();
    virtual void draw();
    virtual void update();
};

class VkEntity : public Entity {
    virtual void draw();
    virtual void update();

private:
    VkBuffer indexBuffer;
    VkBuffer vertexBuffer;

    VkDeviceMemory indexBufferMemory;
    VkDeviceMemory vertexBufferMemory;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

};


class EntityManager {
public:
    static EntityManager& Get();

    void addEntity(Entity* entity);
    void removeEntity(Entity* entity);

    void addRenderer(Vulkan* vulkan);

public:
    EntityManager(EntityManager const&) = delete;
    void operator=(EntityManager const&) = delete;

private:
    EntityManager() {};
    static EntityManager instance;



private:
    std::queue<Entity> AvailableEntitys{};
    std::array<uint32_t, 50> entityList;
    uint32_t LivingEntityCount{};
};




#endif //NINEENGINE_ENTITYMANAGER_H
