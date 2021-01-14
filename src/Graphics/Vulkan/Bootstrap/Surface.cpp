//
// Created by nineball on 12/24/20.
//

#include "Surface.h"


Surface::Surface() {

    vkGlobalPool& variables = vkGlobalPool::Get();

    if (glfwCreateWindowSurface(variables.getVkInstance(), variables.getWindow(), nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    variables.setVkSurfaceKhr(surface);
}

Surface::~Surface() {
    vkGlobalPool& variables = vkGlobalPool::Get();
    vkDestroySurfaceKHR(variables.getVkInstance(), variables.getVkSurfaceKhr(), nullptr);
}