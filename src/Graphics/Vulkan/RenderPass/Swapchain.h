//
// Created by nineball on 1/13/21.
//

#ifndef NINEENGINE_SWAPCHAIN_H
#define NINEENGINE_SWAPCHAIN_H

#include "../vkGlobalPool.h"


class Swapchain {
public:
    Swapchain();
    ~Swapchain();

private:
    void createSwapChain();
    void createImageViews();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

private:
  //std::vector<VkImage> swapChainImages;
  //std::vector<VkImageView> swapChainImageViews;
};


#endif //NINEENGINE_SWAPCHAIN_H
