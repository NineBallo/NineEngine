//
// Created by nineball on 4/25/21.
//

#ifndef NINEENGINE_WINDOW_H
#define NINEENGINE_WINDOW_H

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include "../Vulkan/Core/SharedStructs.h"
#include "../Vulkan/Vulkan.h"

namespace Graphics::Window {
    GLFWwindow * createWindow(int Width, int Height, const char *title, bool resizable, Vulkan* vulkan);
    void destroyWindow(GLFWwindow* windowHandle);
    bool shouldExit(GLFWwindow* windowHandle);
}

namespace VKBareAPI::Window {
    void createSurface(VKBareAPI::Window::NEWindow &window, VkInstance instance);
    void destroySurface(VkInstance instance, VkSurfaceKHR surface);
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
}

#endif //NINEENGINE_WINDOW_H
