//
// Created by nineball on 12/27/20.
//

#ifndef NINEENGINE_VULKAN_H
#define NINEENGINE_VULKAN_H

#include "Devices/Device.h"
#include "Bootstrap/VulkanInstance.h"
#include "Bootstrap/Surface.h"
#include "Pipelines/GraphicsPipeline.h"
#include "../../Devices/Window.h"
#include "RenderPass/Swapchain.h"
#include "vkGlobalPool.h"
#include <algorithm>

class Vulkan {
public:
    Vulkan(int width, int height, const char *title, bool resizableWindow, bool fullscreen);
    ~Vulkan();

    void initWindow(int width, int height, const char *title, bool resizableWindow, bool fullscreen);
    void initVulkan();

    void mainloop();

    void createCommandPool();
    void createCommandBuffers();

    void createSyncObjects();
    void drawFrame();



private:
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

private:

    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
private:
    Swapchain *swapchain;
    VulkanInstance *vulkanInstance;
    Device *device;
    Surface *surface;
    Window *window;
    GraphicsPipeline *graphicsPipeline;
};


#endif //NINEENGINE_VULKAN_H
