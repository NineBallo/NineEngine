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
#include "Core/Swapchain.h"
#include "Core/Pipeline.h"
#include "Core/Buffers.h"

///TODO idk if ill need these lets see
#include <set>
#include <vector>
#include <array>

using namespace VKBareAPI;

class Vulkan {
public:
    Vulkan();
    ~Vulkan();


    Window::NEWindow windowVars;

    void mainLoop();

public:
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    Device::NEDevice deviceVars;
    Swapchain::NESwapchain swapchainVars;
    Pipeline::NEPipeline pipelineVars;
};


#endif //NINEENGINE_VULKAN_H
