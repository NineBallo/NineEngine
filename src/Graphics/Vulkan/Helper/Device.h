//
// Created by nineball on 6/14/21.
//

#ifndef NINEENGINE_DEVICE_H
#define NINEENGINE_DEVICE_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "vector"
#include "../Core/NEShared.h"

bool checkDeviceExtensionSupport(VkPhysicalDevice candidate, std::vector<const char *> deviceExtensions);
uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice candidate, VkSurfaceKHR surface);
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice candidate, VkSurfaceKHR surface);
bool isDeviceSuitable(VkPhysicalDevice candidate, VkSurfaceKHR surface);
bool checkDeviceExtensionSupport(VkPhysicalDevice candidate, VkSurfaceKHR surface);


#endif //NINEENGINE_DEVICE_H
