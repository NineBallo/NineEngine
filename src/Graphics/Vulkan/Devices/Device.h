//
// Created by nineball on 1/12/21.
//

#ifndef NINEENGINE_DEVICE_H
#define NINEENGINE_DEVICE_H

#include "../vkGlobalPool.h"
#include <string>
#include <iostream>

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


class Device {
public:
    Device();

    ~Device();

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

private:

    void pickPhysicalDevice();

    bool isDeviceSuitable(VkPhysicalDevice device);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);


    void createLogicalDevice();

    void populateVkDeviceQueueCreateInfo(VkDeviceQueueCreateInfo &CreateInfo, uint32_t family);








    const std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

private:
    //Device stuff

    VkDevice logicalDevice;
    VkPhysicalDevice physicalDevice;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    ////TODO Condense all validation layer jazz into one file to reduce duplicate code.
    //Validation layer stuff
    bool enableValidationLayers = false;
    std::vector<const char *> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };
};


#endif //NINEENGINE_DEVICE_H
