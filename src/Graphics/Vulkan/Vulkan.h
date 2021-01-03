//
// Created by nineball on 12/27/20.
//

#ifndef NINEENGINE_VULKAN_H
#define NINEENGINE_VULKAN_H

#include "Devices/LogicalDevice.h"
#include "Boilerplate/VulkanInstance.h"
#include "Boilerplate/Surface.h"
#include "../../Devices/Window.h"
#include <algorithm>

class Vulkan {
public:
    Vulkan(int width, int height, const char *title, bool resizableWindow, bool fullscreen);

    ~Vulkan();

    void initWindow(int width, int height, const char *title, bool resizableWindow, bool fullscreen);

    void initVulkan();

    void mainloop();

    void createSwapChain();

    void createImageViews();

private:
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

private:
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    VulkanInstance *vulkanInstance;
    LogicalDevice *logicalDevice;
    Surface *surface;
    Window *window;
};


#endif //NINEENGINE_VULKAN_H
