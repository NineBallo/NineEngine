//
// Created by nineball on 5/29/21.
//

#ifndef NINEENGINE_NEWINDOW_H
#define NINEENGINE_NEWINDOW_H

#include "string"
#include "NESwapchain.h"
#include "../NEInstance.h"
#include <memory>
class NESwapchain;
class NEDevice;
namespace NEVK {
    class NEWindow {
    public:
        NEWindow(int width_, int height_, std::string title_, bool resizable_, NEInstance& instance, NEDevice* device);
        ~NEWindow();

        operator GLFWwindow*() const { return window; }

    public:
        void recreateSwapchain();


        void createSurface();
        VkSurfaceKHR getSurface();
        bool shouldExit();

        void setExtent(VkExtent2D);
        VkExtent2D getExtent();
        NESwapchain* getSwapchain();


    private:
        std::shared_ptr<NESwapchain> swapchain;
        VkExtent2D chooseSwapExtent();

    private:
        int width;
        int height;
        bool resizable;
        std::string title;

        VkExtent2D extent;
        VkFormat format;

        VkSurfaceKHR surface = VK_NULL_HANDLE;
        GLFWwindow* window;

        NEDevice* mDevice;
        NEInstance& instance;
    };
}


#endif //NINEENGINE_NEWINDOW_H
