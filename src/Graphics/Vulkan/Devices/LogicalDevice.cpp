//
// Created by nineball on 12/24/20.
//

#include "LogicalDevice.h"

LogicalDevice::LogicalDevice(bool _enableValidationLayers)
        : PhysicalDevice() {
    createLogicalDevice();
    enableValidationLayers = _enableValidationLayers;
}

LogicalDevice::~LogicalDevice() {
    vkDestroyDevice(logicalDevice, nullptr);
}

void LogicalDevice::createLogicalDevice() {
    //Filling in vulkan info to create the Present and Graphics queues
    indices = findQueueFamilies(physicalDevice);
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

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
}


void LogicalDevice::populateVkDeviceQueueCreateInfo(VkDeviceQueueCreateInfo &queueCreateInfo, uint32_t family) {
    ///TODO Possibly implement a multiple queue system idk...

    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
    //The most interesting option here, I wonder if multiple queues have a perf/latency impact.
    queueCreateInfo.queueCount = 1;

    //0.0f-1.0f range for queue priority
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;
}
