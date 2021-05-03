//
// Created by nineball on 4/16/21.
//

#ifndef NINEENGINE_VULKAN_H
#define NINEENGINE_VULKAN_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Core/Instance.h"
#include "Core/Device.h"

///TODO idk if ill need these lets see
#include <set>
#include <vector>
#include <array>

class Vulkan {
public:
    Vulkan();
    ~Vulkan();


    GLFWwindow* windowHandle;
private:
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkSurfaceKHR surface;

    Device::DeviceQueues deviceQueues;
};


#endif //NINEENGINE_VULKAN_H
