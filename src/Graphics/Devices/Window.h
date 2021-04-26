//
// Created by nineball on 4/25/21.
//

#ifndef NINEENGINE_WINDOW_H
#define NINEENGINE_WINDOW_H

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

namespace Graphics::Window {
    GLFWwindow * createWindow(int Width, int Height, const char *title, bool resizable);
    void destroyWindow(GLFWwindow* windowHandle);

    bool shouldExit(GLFWwindow* windowHandle);
}

namespace VKBareAPI::Surface {
    VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow* windowHandle);
    void destroySurface(VkInstance instance, VkSurfaceKHR surface);
}

#endif //NINEENGINE_WINDOW_H
