//
// Created by nineball on 5/29/21.
//

#include "NEWindow.h"
#include <iostream>
#include <utility>

namespace NEVK {
    NEWindow::NEWindow(int width_, int height_, std::string title_, bool resizable_, NEInstance& instance, NEDevice* device)
    : width{width_}, height{height_}, title{std::move(title_)}, resizable{resizable_}, instance{instance}, mDevice{device} {

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

    VkExtent2D NEWindow::chooseSwapExtent() {
        VkSurfaceCapabilitiesKHR capabilities = mDevice->getSwapChainDetails().capabilities;

        if (capabilities.currentExtent.width != UINT32_MAX) {
            std::cout << capabilities.currentExtent.width << "X" << capabilities.currentExtent.height << "\n";
            return capabilities.currentExtent;
        } else {

            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
            };

            actualExtent.width = std::max(capabilities.minImageExtent.width,
                                          std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height,
                                           std::min(capabilities.maxImageExtent.height, actualExtent.height));
            std::cout << actualExtent.width << "X" << actualExtent.height << "\n";
            return actualExtent;
        }
    }

    void NEWindow::recreateSwapchain() {
        chooseSwapExtent();
        swapchain = std::make_shared<NESwapchain>(mDevice, extent, surface);
    }

    VkSurfaceKHR NEWindow::getSurface() {return surface;}
    VkExtent2D NEWindow::getExtent() {return extent;}
    void NEWindow::setExtent(VkExtent2D extent_) {extent = extent_;}
    NESwapchain* NEWindow::getSwapchain() {return swapchain.get();}
}