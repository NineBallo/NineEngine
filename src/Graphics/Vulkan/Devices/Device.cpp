//
// Created by nineball on 1/12/21.
//

#include "Device.h"

Device::Device() {
    pickPhysicalDevice();
    createLogicalDevice();
}

Device::~Device() {
    vkDestroyDevice(logicalDevice, nullptr);
}

////------------------------------------------------------------------------------------------------------------////
///PHYSICAL DEVICE
void Device::pickPhysicalDevice() {
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

bool Device::isDeviceSuitable(VkPhysicalDevice device) {
    vkGlobalPool::Get().findQueueFamilies(device);

    ///Can it even have video out, etc...
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    ///Does it have a value?
    return  vkGlobalPool::Get().getQueueFamilyIndices().graphicsFamily.has_value() && extensionsSupported && swapChainAdequate;
}

bool Device::checkDeviceExtensionSupport(VkPhysicalDevice device) {
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

SwapChainSupportDetails Device::querySwapChainSupport(VkPhysicalDevice device) {
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
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, variables.getVkSurfaceKhr(), &presentModeCount,
                                                  details.presentModes.data());
    }

    return details;
}
////------------------------------------------------------------------------------------------------------------////



////------------------------------------------------------------------------------------------------------------////
///LOGICAL DEVICE

void Device::createLogicalDevice() {
    vkGlobalPool::QueueFamilyIndices indices = vkGlobalPool::Get().getQueueFamilyIndices();

    //Filling in vulkan info to create the Present and Graphics queues
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies ={indices.graphicsFamily.value(), indices.presentFamily.value()};

    for (uint32_t QueueFamily: uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        populateVkDeviceQueueCreateInfo(queueCreateInfo, QueueFamily);

        queueCreateInfos.push_back(queueCreateInfo);
    };


    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    //Reads da vector for the queues to create
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    //Extra jazz im assuming, DLSS and the like.
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());;
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);

    vkGlobalPool::Get().setVkDevice(logicalDevice);
    vkGlobalPool::Get().setVkPhysicalDevice(physicalDevice);

    vkGlobalPool::Get().setGraphicsQueue(graphicsQueue);
    vkGlobalPool::Get().setPresentQueue(presentQueue);
}

void Device::populateVkDeviceQueueCreateInfo(VkDeviceQueueCreateInfo &queueCreateInfo, uint32_t family) {
    ///TODO Possibly implement a multiple queue system idk...

    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = vkGlobalPool::Get().getQueueFamilyIndices().graphicsFamily.value();
    //The most interesting option here, I wonder if multiple queues have a perf/latency impact.
    queueCreateInfo.queueCount = 1;

    //0.0f-1.0f range for queue priority
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;
}