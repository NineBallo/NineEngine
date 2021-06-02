//
// Created by nineball on 5/29/21.
//

#ifndef NINEENGINE_NESWAPCHAIN_H
#define NINEENGINE_NESWAPCHAIN_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include "NEDevice.h"
#include "NEWindow.h"



namespace NEVK {
    class NEDevice;
    class NEWindow;


    class NESwapchain {
    public:
        NESwapchain(NEDevice* device_, NEWindow* window_);
        ~NESwapchain();

        void startFrame();
        void endFrame();
        int getFrame();

        void submitCommandBuffer(VkCommandBuffer commandBuffer);
        VkImageView createImageView(VkImage image, VkFormat imageFormat);
        void createFrameBuffers();

        std::vector<VkFramebuffer> framebuffers;

        VkExtent2D getExtent() {return extent;}
    private:
        void createSwapChain();
        void createSyncObjects();


        void updateUniformBuffer(int currentImage);
        void createImageViews();

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
        VkExtent2D chooseSwapExtent();

        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkFramebuffer> swapChainFramebuffers;

    private:
        int MAX_FRAMES_IN_FLIGHT = 2;
        int currentFrame = 0;
        bool framebufferResized = false;
        uint32_t imageIndex;

        NEWindow* window;
        NEDevice* device;

        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;


        VkQueue graphicsQueue;
        VkQueue presentQueue;

        VkSwapchainKHR swapchain;


        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;


    };
}



#endif //NINEENGINE_NESWAPCHAIN_H
