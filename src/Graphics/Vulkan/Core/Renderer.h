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
#include "Device/NEDevice.h"

namespace NEVK {
    class NERenderer {
    public:
        NERenderer();
        ~NERenderer();

        void renderFrame();

    private:
        void drawFrame();

        void createCommandBuffers();
        void updateUniformBuffers();
        void createUniformBuffers();


        void createTextureImage(VkImage& textureImage, VkDeviceMemory& textureImageMemory);
        void createTextureImageView(VkImage &image, VkImageView &imageView);
        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                                     VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    private:
        std::shared_ptr<NEInstance> instance;
        std::shared_ptr<NEDevice> device;
        NESwapchain* swapchain;
        NEPipeline* pipeline;

        VkCommandPool commandPool;
        std::vector<VkCommandBuffer> commandBuffers;
    };
}


#endif //NINEENGINE_RENDERER_H
