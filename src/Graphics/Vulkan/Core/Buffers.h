//
// Created by nineball on 5/18/21.
//

#ifndef NINEENGINE_BUFFERS_H
#define NINEENGINE_BUFFERS_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "SharedStructs.h"

namespace VKBareAPI::Buffers {

    void create(Device::NEDevice &deviceVars, Swapchain::NESwapchain &swapchainVars, Pipeline::NEPipeline &pipelineVars);
    void destroy(Device::NEDevice &deviceVars);


    void createCommandPool(Device::NEDevice &deviceVars);
    void createCommandBuffers(Device::NEDevice &deviceVars, Swapchain::NESwapchain &swapchainVars, Pipeline::NEPipeline &pipelineVars);

    void createVertexBuffer(Device::NEDevice &deviceVars);
    void createIndexBuffer(Device::NEDevice &deviceVars);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, Device::NEDevice &deviceVars);

    VkBuffer createBuffer(VkDeviceSize size, Device::NEDevice &deviceVars, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    uint32_t findMemoryType(uint32_t typeFilter, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags properties);

    void createSyncObjects(Device::NEDevice &deviceVars, Swapchain::NESwapchain &swapchainVars);

    void createUniformBuffers(Device::NEDevice &deviceVars, Swapchain::NESwapchain &swapchainVars);

    void updateUniformBuffer(uint32_t currentImage, Device::NEDevice &deviceVars, Swapchain::NESwapchain &swapchainVars);

    void createDescriptorPool(Device::NEDevice &deviceVars, Swapchain::NESwapchain &swapchainVars);
    void createDescriptorSets(Device::NEDevice &deviceVars, Swapchain::NESwapchain &swapchainVars, Pipeline::NEPipeline &pipeline);
}

#endif //NINEENGINE_BUFFERS_H
