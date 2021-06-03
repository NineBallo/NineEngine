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
#include "../../../Managers/Coordinator.h"
#include "../../../Managers/Shared.h"

class VkRenderer : public System {
public:
    void draw();
};


namespace NEVK {
    class NERenderer {
    public:
        NERenderer();
        ~NERenderer();

        void renderFrame();

    private:
        void drawFrame();

        void createCommandBuffers(size_t entity);

        void updateUniformBuffers(size_t entity, int currentImage);

        void createUniformBuffers();


        void createTextureImage(VkImage& textureImage, VkDeviceMemory& textureImageMemory);
        void createTextureImageView(VkImage &image, VkImageView &imageView);
        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                                     VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    private:
        Coordinator& coordinator = Coordinator::Get();

        std::shared_ptr<VkRenderer> renderer;
        std::shared_ptr<NEDevice> device;
        std::shared_ptr<NEInstance> instance;
        NESwapchain* swapchain;
        NEPipeline* pipeline;



        VkCommandPool commandPool;
        std::vector<VkCommandBuffer> commandBuffers;
    };
}


#endif //NINEENGINE_RENDERER_H
