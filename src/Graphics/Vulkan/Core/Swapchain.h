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

    void createSwapChain(NESwapchain &swapchainVars, Device::NEDevice deviceVars, Window::NEWindow &windowVars);
    void createImageViews(NESwapchain &swapchainVars, VkDevice device);

    void recreateSwapChain(NESwapchain &swapchainVars, Device::NEDevice &deviceVars, Pipeline::NEPipeline &pipelineVars, Window::NEWindow &windowVars);

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, Window::NEWindow windowVars);

    void destroy(NESwapchain &swapchainVars, Device::NEDevice &deviceVars, Pipeline::NEPipeline &pipelineVars);

    void drawFrame(NESwapchain &swapchainVars, Device::NEDevice &deviceVars, Pipeline::NEPipeline &pipelineVars, Window::NEWindow &windowVars);

    void cleanupSwapChain(NESwapchain &swapchainVars, Device::NEDevice &deviceVars, Pipeline::NEPipeline &pipelineVars);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
    VkImageView createImageView(VkImage image, VkFormat format, VkDevice device);
}


#endif //NINEENGINE_SWAPCHAIN_H
