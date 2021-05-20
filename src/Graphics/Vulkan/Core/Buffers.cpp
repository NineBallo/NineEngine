//
// Created by nineball on 5/18/21.
//

#include "Buffers.h"
#include <iostream>
#include "string.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

namespace VKBareAPI::Buffers {

    void create(Device::NEDevice &deviceVars, Swapchain::NESwapchain &swapchainVars, Pipeline::NEPipeline &pipelineVars) {
        createCommandPool(deviceVars);
        createVertexBuffer(deviceVars);
        createIndexBuffer(deviceVars);
        createUniformBuffers(deviceVars, swapchainVars);
        createDescriptorPool(deviceVars, swapchainVars);
        createDescriptorSets(deviceVars, swapchainVars, pipelineVars);
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

            vkCmdBindDescriptorSets(deviceVars.Buffers.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineVars.pipelineLayout, 0, 1, &deviceVars.Buffers.descriptorSets[i], 0, nullptr);

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

    void createUniformBuffers(Device::NEDevice &deviceVars, Swapchain::NESwapchain &swapchainVars) {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        deviceVars.Buffers.uniformBuffers.resize(swapchainVars.swapChainImages.size());
        deviceVars.Buffers.uniformBuffersMemory.resize(swapchainVars.swapChainImages.size());

        for (size_t i = 0; i < swapchainVars.swapChainImages.size(); i++) {
            createBuffer(bufferSize, deviceVars, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         deviceVars.Buffers.uniformBuffers[i], deviceVars.Buffers.uniformBuffersMemory[i]);
        }

    }

    void updateUniformBuffer(uint32_t currentImage, Device::NEDevice &deviceVars, Swapchain::NESwapchain &swapchainVars) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};

        //rotate 90degrees/sec
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        //change camera position to 45degrees above
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        ubo.proj = glm::perspective(glm::radians(45.0f), swapchainVars.swapChainExtent.width / (float) swapchainVars.swapChainExtent.height, 0.1f, 10.0f);
        //was made for opengl, need to flip scaling factor of y
        ubo.proj[1][1] *= -1;

        void* data;
        vkMapMemory(deviceVars.device, deviceVars.Buffers.uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(deviceVars.device, deviceVars.Buffers.uniformBuffersMemory[currentImage]);

    }

    void createDescriptorPool(Device::NEDevice &deviceVars, Swapchain::NESwapchain &swapchainVars) {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(swapchainVars.swapChainImages.size());

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(swapchainVars.swapChainImages.size());

        if (vkCreateDescriptorPool(deviceVars.device, &poolInfo, nullptr, &deviceVars.descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }

    }


    void createDescriptorSets(Device::NEDevice &deviceVars, Swapchain::NESwapchain &swapchainVars, Pipeline::NEPipeline &pipeline) {
        std::vector<VkDescriptorSetLayout> layouts(swapchainVars.swapChainImages.size(), pipeline.descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = deviceVars.descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapchainVars.swapChainImages.size());
        allocInfo.pSetLayouts = layouts.data();

        deviceVars.Buffers.descriptorSets.resize(swapchainVars.swapChainImages.size());
        if (vkAllocateDescriptorSets(deviceVars.device, &allocInfo, deviceVars.Buffers.descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < swapchainVars.swapChainImages.size(); i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = deviceVars.Buffers.uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = deviceVars.Buffers.descriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;
            descriptorWrite.pImageInfo = nullptr; // Optional
            descriptorWrite.pTexelBufferView = nullptr; // Optional

            vkUpdateDescriptorSets(deviceVars.device, 1, &descriptorWrite, 0, nullptr);
        }


    }

}