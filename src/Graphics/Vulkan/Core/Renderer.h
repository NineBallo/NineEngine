//
// Created by nineball on 5/29/21.
//

#ifndef NINEENGINE_RENDERER_H
#define NINEENGINE_RENDERER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "vector"
#include "memory"
#include "NEInstance.h"
#include "NEDevice.h"
#include "NEDisplay.h"
#include "../../../Managers/Coordinator.h"
#include "../../../Managers/Shared.h"


class NERenderer : public System {
public:
    NERenderer();
    ~NERenderer();

    bool renderFrame();

private:
    void drawFrame();

    void updateUniformBuffers(size_t entity, int currentImage);

    void createCommandBuffers(VkRenderable &VKEntity);

    void loadTexture(VkImage &textureImage, VkImageView &textureImageView, VkDeviceMemory &textureImageMemory);
    void loadModel(VkRenderable &VKEntity);

    void createTextureImage(VkImage& textureImage, VkDeviceMemory& textureImageMemory);
    void createTextureImageView(VkImage &image, VkImageView &imageView);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                                 VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

    VkRenderable createEntity();
private:
    Coordinator& coordinator = Coordinator::Get();

    std::shared_ptr<NEInstance> instance;

    NEDevice* device;
    NEDisplay* display;
    NEPipeline* pipeline;

};



#endif //NINEENGINE_RENDERER_H
