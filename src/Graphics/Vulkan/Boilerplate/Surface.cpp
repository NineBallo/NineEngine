//
// Created by nineball on 12/24/20.
//

#include "Surface.h"


Surface::Surface(VkInstance *_instance, GLFWwindow *_window) {
    window = _window;
    instance = _instance;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

Surface::~Surface() {
    vkDestroySurfaceKHR(*instance, surface, nullptr);
}

VkSurfaceKHR *Surface::getVkSurfaceKHRPTR() {
    return &surface;
}