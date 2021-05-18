//
// Created by nineball on 5/18/21.
//

#include "Buffers.h"
#include <iostream>
#include "string.h"

namespace VKBareAPI::Buffers {

    void create(Device::NEDevice &deviceVars, Swapchain::NESwapchain &swapchainVars, Pipeline::NEPipeline &pipelineVars) {
        createCommandPool(deviceVars);
        createVertexBuffer(deviceVars);
        createIndexBuffer(deviceVars);
        createCommandBuffers(deviceVars, swapchainVars, pipelineVars);
        createSyncObjects(deviceVars,swapchainVars);
    }

    void destroy(Device::NEDevice &deviceVars) {
        std::cout << "Destroying Buffers\n";
        vkDestroyCommandPool(deviceVars.device, deviceVars.commandPool, nullptr);
        vkDestroyBuffer(deviceVars.device, deviceVars.Buffers.vertexBuffer, nullptr);
        vkFreeMemory(deviceVars.device, deviceVars.Buffers.vertexBufferMemory, nullptr);
        vkDestroyBuffer(deviceVars.device, deviceVars.Buffers.indexBuffer, nullptr);
        vkFreeMemory(deviceVars.device, deviceVars.Buffers.indexBufferMemory, nullptr);
    }

    void createCommandPool(Device::NEDevice &deviceVars) {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = deviceVars.indices.graphicsFamily.value();
        poolInfo.flags = 0; // Optional

        VkCommandPool commandPool;
        if (vkCreateCommandPool(deviceVars.device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool!");
        }

        deviceVars.commandPool = commandPool;
    }


    void createCommandBuffers(Device::NEDevice &deviceVars, Swapchain::NESwapchain &swapchainVars, Pipeline::NEPipeline &pipelineVars) {

        deviceVars.Buffers.commandBuffers.resize(swapchainVars.swapChainFramebuffers.size());

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = deviceVars.commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) deviceVars.Buffers.commandBuffers.size();

        if (vkAllocateCommandBuffers(deviceVars.device, &allocInfo, deviceVars.Buffers.commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers!");
        }

        for (size_t i = 0; i < deviceVars.Buffers.commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Optional
            beginInfo.pInheritanceInfo = nullptr; // Optional

            if (vkBeginCommandBuffer(deviceVars.Buffers.commandBuffers[i], &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("Failed to begin recording command buffer!");
            }

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = pipelineVars.renderPass;
            renderPassInfo.framebuffer = swapchainVars.swapChainFramebuffers[i];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = swapchainVars.swapChainExtent;

            VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(deviceVars.Buffers.commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(deviceVars.Buffers.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineVars.pipeline);

            VkBuffer vertexBuffers[] = {deviceVars.Buffers.vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(deviceVars.Buffers.commandBuffers[i], 0, 1, vertexBuffers, offsets);

            vkCmdBindIndexBuffer(deviceVars.Buffers.commandBuffers[i], deviceVars.Buffers.indexBuffer, 0, VK_INDEX_TYPE_UINT16);

            vkCmdDrawIndexed(deviceVars.Buffers.commandBuffers[i], static_cast<uint32_t>(VKBareAPI::Buffers::indices.size()), 1, 0, 0, 0);

            vkCmdEndRenderPass(deviceVars.Buffers.commandBuffers[i]);

            if (vkEndCommandBuffer(deviceVars.Buffers.commandBuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
    }

    ///TODO one buffer command, too much duplicate code here
    void createVertexBuffer(Device::NEDevice &deviceVars) {
        VkDeviceSize bufferSize = sizeof(VKBareAPI::Buffers::vertices[0]) * VKBareAPI::Buffers::vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, deviceVars, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(deviceVars.device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, VKBareAPI::Buffers::vertices.data(), (size_t) bufferSize);
        vkUnmapMemory(deviceVars.device, stagingBufferMemory);

        createBuffer(bufferSize, deviceVars, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, deviceVars.Buffers.vertexBuffer,
                     deviceVars.Buffers.vertexBufferMemory);

        copyBuffer(stagingBuffer, deviceVars.Buffers.vertexBuffer, bufferSize, deviceVars);
        vkDestroyBuffer(deviceVars.device, stagingBuffer, nullptr);
        vkFreeMemory(deviceVars.device, stagingBufferMemory, nullptr);
    }

    void createIndexBuffer(Device::NEDevice &deviceVars) {
        VkDeviceSize bufferSize = sizeof(VKBareAPI::Buffers::indices[0]) * VKBareAPI::Buffers::indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        createBuffer(bufferSize, deviceVars,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(deviceVars.device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, VKBareAPI::Buffers::indices.data(), (size_t) bufferSize);
        vkUnmapMemory(deviceVars.device, stagingBufferMemory);

        createBuffer(bufferSize, deviceVars, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, deviceVars.Buffers.indexBuffer,
                     deviceVars.Buffers.indexBufferMemory);

        copyBuffer(stagingBuffer, deviceVars.Buffers.indexBuffer, bufferSize, deviceVars);

        vkDestroyBuffer(deviceVars.device, stagingBuffer, nullptr);
        vkFreeMemory(deviceVars.device, stagingBufferMemory, nullptr);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, Device::NEDevice &deviceVars) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = deviceVars.commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(deviceVars.device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0; // Optional
        copyRegion.dstOffset = 0; // Optional
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(deviceVars.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(deviceVars.graphicsQueue);

        vkFreeCommandBuffers(deviceVars.device, deviceVars.commandPool, 1, &commandBuffer);
    }

    VkBuffer createBuffer(VkDeviceSize size, Device::NEDevice &deviceVars, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(deviceVars.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create vertex buffer!");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(deviceVars.device, buffer, &memoryRequirements);

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, deviceVars.physicalDevice, properties);

        if (vkAllocateMemory(deviceVars.device, &allocateInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        vkBindBufferMemory(deviceVars.device, buffer, bufferMemory, 0);
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type!");
    }

    void createSyncObjects(Device::NEDevice &deviceVars, Swapchain::NESwapchain &swapchainVars) {

        swapchainVars.inFlightFences.resize(swapchainVars.MAX_FRAMES_IN_FLIGHT);
        swapchainVars.imagesInFlight.resize(swapchainVars.swapChainImages.size(), VK_NULL_HANDLE);

        swapchainVars.imageAvailableSemaphores.resize(swapchainVars.MAX_FRAMES_IN_FLIGHT);
        swapchainVars.renderFinishedSemaphores.resize(swapchainVars.MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < swapchainVars.MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(deviceVars.device, &semaphoreInfo, nullptr,
                                  &swapchainVars.imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(deviceVars.device, &semaphoreInfo, nullptr,
                                  &swapchainVars.renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(deviceVars.device, &fenceInfo, nullptr, &swapchainVars.inFlightFences[i]) != VK_SUCCESS) {

                throw std::runtime_error("Failed to create semaphores for a frame!");
            }
        }
    }
}