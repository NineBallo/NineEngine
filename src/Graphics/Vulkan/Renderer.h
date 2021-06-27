//
// Created by nineball on 5/29/21.
//

#ifndef NINEENGINE_RENDERER_H
#define NINEENGINE_RENDERER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <utility>
#include <memory>
#include "Core/NEInstance.h"
#include "Core/NEDevice.h"
#include "Core/NEDisplay.h"
#include "Coordinator.h"
#include "Shared.h"


class NERenderer : public System {
public:
    NERenderer();
    ~NERenderer();

    NERenderer(const NERenderer&) = delete;
    NERenderer& operator = (const NERenderer&) = delete;

    VkRenderable loadEntityObjects(std::string texture, std::string model);

    bool renderFrame();

    void createPipeline();
    void destroyPipeline();

private:
    void drawFrame(uint32_t display);
    void drawObject(VkRenderable entity);


    void loadModel(VkDeviceMemory &vertexMemory, VkBuffer &vertexBuffer, VkDeviceMemory &indexMemory, VkBuffer &indexBuffer);
    void loadTexture(VkImage &textureImage, VkImageView &textureImageView, VkDeviceMemory &textureImageMemory);

    void updateUniformBuffers(size_t entity, int currentImage);

    void createTextureImage(VkImage& textureImage, VkDeviceMemory& textureImageMemory);
    void createTextureImageView(VkImage &image, VkImageView &imageView);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                                 VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

private:
    Coordinator& coordinator = Coordinator::Get();

    std::shared_ptr<NEInstance> instance;

    NEDevice* device;

    std::array<NEDisplay*, 10> displays;
    std::array<uint32_t, 10> displayToIndex;
    uint32_t displayCount = 0;

    std::array<NEPipeline*, 100> pipelines; //Packed array
    std::array<uint32_t, 100> pipelineToIndex; //Index to specific pipeline in packed array
    uint32_t pipelineCount = 0;

    uint32_t boundPipeline = 0;
};



#endif //NINEENGINE_RENDERER_H
