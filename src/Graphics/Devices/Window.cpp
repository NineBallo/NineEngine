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


void VKBareAPI::Window::createSurface(VKBareAPI::Window::NEWindow &windowVars, VkInstance instance) {

    if (glfwCreateWindowSurface(instance, windowVars.window, nullptr, &windowVars.surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!");
    }
}

void VKBareAPI::Window::destroySurface(VkInstance instance, VkSurfaceKHR surface) {
    vkDestroySurfaceKHR(instance, surface, nullptr);
}