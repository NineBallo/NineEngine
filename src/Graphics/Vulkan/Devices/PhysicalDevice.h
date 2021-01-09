//
// Created by nineball on 12/23/20.
//

#ifndef VULKANATTEMPT_PHYSICALDEVICES_H
#define VULKANATTEMPT_PHYSICALDEVICES_H

///this lets glfw load the Graphics library for us.
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <set>
#include "../vkGlobalPool.h"

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};



class PhysicalDevice {
public:
    PhysicalDevice();
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

protected:
    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);

    //int rateDeviceSuitability(VkPhysicalDevice device);

    const std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

protected:
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

private:
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

};


#endif //VULKANATTEMPT_PHYSICALDEVICES_H
