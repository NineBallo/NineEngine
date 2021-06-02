//
// Created by nineball on 5/29/21.
//

#include "NEWindow.h"
#include <iostream>
#include <utility>

namespace NEVK {
    NEWindow::NEWindow(int width_, int height_, std::string title_, bool resizable_, NEInstance& instance)
    : width{width_}, height{height_}, title{std::move(title_)}, resizable{resizable_}, instance{instance} {

        glfwInit();
        ///Dont create an opengl context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        if (resizable) {
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        } else {
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        }

        window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        //  glfwSetWindowUserPointer(window, vulkan);
        //  glfwSetFramebufferSizeCallback(window, VKBareAPI::Window::framebufferResizeCallback);
    }

    NEWindow::~NEWindow() {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void NEWindow::createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface!");
        }
    }

    bool NEWindow::shouldExit() {
        if (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            return false;
        } else {
            return true;
        }
    }

    void NEWindow::createSwapchain(NEDevice* device) {
        swapchain = std::make_shared<NESwapchain>(device, this);
    }

    VkSurfaceKHR NEWindow::getSurface() {return surface;}
    VkExtent2D NEWindow::getExtent() {return extent;}
    NESwapchain* NEWindow::getSwapchain() {return swapchain.get();}
}