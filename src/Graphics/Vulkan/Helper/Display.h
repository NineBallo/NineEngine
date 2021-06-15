//
// Created by nineball on 6/14/21.
//

#ifndef NINEENGINE_DISPLAY_H
#define NINEENGINE_DISPLAY_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

void createImageViews(std::vector<VkImageView> &imageViews, VkFormat format, VkDevice device);
VkImageView createImageView(VkImage image, VkFormat imageFormat, VkDevice device);
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);



#endif //NINEENGINE_DISPLAY_H
