//
// Created by nineball on 4/16/21.
//

#ifndef NINEENGINE_SWAPCHAIN_H
#define NINEENGINE_SWAPCHAIN_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include "SharedStructs.h"

namespace VKBareAPI::Swapchain {

    void createSwapChain(NESwapchain &swapchainVars, Device::NEDevice deviceVars, Window::NEWindow windowVars);
    void createImageViews(NESwapchain &swapchainVars, VkDevice device);

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, Window::NEWindow windowVars);

    void destroy(NESwapchain &swapchainVars, Device::NEDevice &deviceVars);


    void drawFrame(NESwapchain &swapchainVars, Device::NEDevice &deviceVars);
}


#endif //NINEENGINE_SWAPCHAIN_H
