//
// Created by nineball on 4/16/21.
//

#ifndef NINEENGINE_DEVICE_H
#define NINEENGINE_DEVICE_H


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <array>
#include <set>
#include "SharedStructs.h"

namespace VKBareAPI::Device {

    void createDevices(NEDevice &devices, VkInstance instance, VkSurfaceKHR surface, Swapchain::SwapChainSupportDetails &swapChainSupportDetails);

    VkPhysicalDevice pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    NEDevice createLogicalDevice(VkPhysicalDevice device, VkSurfaceKHR surface);

    bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    Swapchain::SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

    void destroy(VkDevice device);
}



#endif //NINEENGINE_DEVICE_H

