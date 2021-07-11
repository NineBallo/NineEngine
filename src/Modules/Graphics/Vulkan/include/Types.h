//
// Created by nineball on 7/7/21.
//

#ifndef NINEENGINE_TYPES_H
#define NINEENGINE_TYPES_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

struct device {
    VkDevice device;
    VkPhysicalDevice GPU;

    VkQueue graphicsQueue;
    uint32_t graphicsQueueFamily;

    VkQueue presentQueue;
    uint32_t presentQueueFamily;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkRenderPass renderpass;
};

struct renderpassBuilder {
    VkAttachmentDescription color_attachment = {};
    VkAttachmentReference color_attachment_ref = {};
    VkSubpassDescription subpass = {};
};

#endif //NINEENGINE_TYPES_H
