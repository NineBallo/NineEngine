//
// Created by nineball on 12/23/20.
//

#include "PhysicalDevice.h"

PhysicalDevice::PhysicalDevice(VkInstance *_instance, VkSurfaceKHR *_surface) {
    instance = _instance;
    surface = _surface;
    pickPhysicalDevice();
}

void PhysicalDevice::pickPhysicalDevice() {
    ///Get all devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(*instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with vulkan support. LMAO, loser.");
    } else {
        std::cout << deviceCount << " Device(s) Found!\n";
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(*instance, &deviceCount, devices.data());

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


QueueFamilyIndices PhysicalDevice::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    ///Get device queue properties
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    ///Find a queue family that supports VK_QUEUE_GRAPHICS_BIT.
    int i = 0;
    for (const auto &queueFamily : queueFamilies) {

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, *surface, &presentSupport);
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }
        i++;
    }

    return indices;

}

SwapChainSupportDetails PhysicalDevice::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    ///Thingy that gets surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, *surface, &details.capabilities);


    ///Get supported surface formats
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, *surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, *surface, &formatCount, details.formats.data());
    }

    ///Get supported resent formats
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, *surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, *surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}