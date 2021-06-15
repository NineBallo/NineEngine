//
// Created by nineball on 6/14/21.
//

#include "../Device.h"
#include "iostream"
#include "set"
#include "vector"
#include "../../Core/NEShared.h"

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

bool checkDeviceExtensionSupport(VkPhysicalDevice candidate) {
    //Get all extensions
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, availableExtensions.data());

    //Get length of extension list.
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    //If the extension does exist it will erase from the required extensions;
    for (const auto &extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    //Return true if empty/all extensions supported.
    return requiredExtensions.empty();
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice candidate, VkSurfaceKHR surface) {

    QueueFamilyIndices queryIndices;

    ///Get device queue properties
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueFamilyCount, queueFamilies.data());

    ///Find a queue family that supports VK_QUEUE_GRAPHICS_BIT.
    int i = 0;
    for (const auto &queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queryIndices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface, &presentSupport);

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queryIndices.presentFamily = i;
        }
        if (queryIndices.isComplete()) {
            break;
        }
        i++;
    }
    return queryIndices;
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice candidate, VkSurfaceKHR surface) {
    SwapChainSupportDetails supportDetails;

    ///Thingy that gets surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(candidate, surface, &supportDetails.capabilities);


    ///Get supported surface formats
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(candidate, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        supportDetails.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(candidate, surface, &formatCount, supportDetails.formats.data());
    }

    ///Get supported resent formats
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(candidate, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        supportDetails.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(candidate, surface, &presentModeCount, supportDetails.presentModes.data());
    }

    return supportDetails;
}

bool isDeviceSuitable(VkPhysicalDevice candidate, VkSurfaceKHR surface) {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(candidate, surface);

    ///Can it even have video out, etc...
    bool extensionsSupported = checkDeviceExtensionSupport(candidate);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails supportDetails = querySwapChainSupport(candidate, surface);
        swapChainAdequate = !supportDetails.formats.empty() && !supportDetails.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(candidate, &supportedFeatures);

    ///Does it have a value?
    return queueFamilyIndices.graphicsFamily.has_value() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;;
}