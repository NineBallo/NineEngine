//
// Created by nineball on 4/25/21.
//

#include "Window.h"
#include <iostream>

GLFWwindow * Graphics::Window::createWindow(int width, int height, const char *title, bool resizable) {
    glfwInit();

    ///Dont create an opengl context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (resizable) {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    } else {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    }

    return glfwCreateWindow(width, height, title, nullptr, nullptr);
}

void Graphics::Window::destroyWindow(GLFWwindow *windowHandle) {
    glfwDestroyWindow(windowHandle);
    glfwTerminate();
}

bool Graphics::Window::shouldExit(GLFWwindow* windowHandle){
    if (!glfwWindowShouldClose(windowHandle)) {
        glfwPollEvents();
        return false;
    } else {
        return true;
    }
}


VkSurfaceKHR VKBareAPI::Surface::createSurface(VkInstance instance, GLFWwindow *windowHandle) {
    VkSurfaceKHR surface;

    if (glfwCreateWindowSurface(instance, windowHandle, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    return surface;
}

void VKBareAPI::Surface::destroySurface(VkInstance instance, VkSurfaceKHR surface) {
    vkDestroySurfaceKHR(instance, surface, nullptr);
}