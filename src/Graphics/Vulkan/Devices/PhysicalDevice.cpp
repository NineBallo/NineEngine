//
// Created by nineball on 12/23/20.
//

#include "PhysicalDevice.h"

PhysicalDevice::PhysicalDevice() {
    pickPhysicalDevice();
}

void PhysicalDevice::pickPhysicalDevice() {
    ///Get all devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkGlobalPool::Get().getVkInstance(), &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with vulkan support. LMAO, loser.");
    } else {
        std::cout << deviceCount << " Device(s) Found!\n";
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vkGlobalPool::Get().getVkInstance(), &deviceCount, devices.data());

    ///Check if device can be used
    for (const auto &device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}


bool PhysicalDevice::isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    ///Can it even have video out, etc...
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    ///Does it have a value?
    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool PhysicalDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    //Get all extensions
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    //Get length of extension list.
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    //If the extension does exist it will erase from the required extensions;
    for (const auto &extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    //Return true if empty/all extensions supported.
    return requiredExtensions.empty();
}




SwapChainSupportDetails PhysicalDevice::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGlobalPool &variables = vkGlobalPool::Get();

    ///Thingy that gets surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, variables.getVkSurfaceKhr(), &details.capabilities);


    ///Get supported surface formats
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, variables.getVkSurfaceKhr(), &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, variables.getVkSurfaceKhr(), &formatCount, details.formats.data());
    }

    ///Get supported resent formats
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, variables.getVkSurfaceKhr(), &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, variables.getVkSurfaceKhr(), &presentModeCount, details.presentModes.data());
    }

    return details;
}