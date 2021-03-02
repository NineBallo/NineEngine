//
// Created by nineball on 12/27/20.
//

#include "Vulkan.h"


///TODO fullscreen implementation.
void Vulkan::initVulkan() {
    vulkanInstance = new VulkanInstance();
    surface = new Surface();
    device = new Device();
    swapchain = new Swapchain();
    graphicsPipeline = new GraphicsPipeline();

    createCommandPool();
    createVertexBuffer();
    createIndexBuffer();
    createCommandBuffers();
    createSyncObjects();
}

Vulkan::Vulkan(int width, int height, const char *title, bool resizableWindow, bool fullscreen) {
    initWindow(width, height, title, resizableWindow, resizableWindow);
    initVulkan();
    mainloop();
}




uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vkGlobalPool::Get().getVkPhysicalDevice(), &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

void Vulkan::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(vkGlobalPool::Get().getVkDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex buffer!");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(vkGlobalPool::Get().getVkDevice(), buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(vkGlobalPool::Get().getVkDevice(), &allocateInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(vkGlobalPool::Get().getVkDevice(), buffer, bufferMemory, 0);
}

void Vulkan::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = vkGlobalPool::Get().getCommandPool();
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(vkGlobalPool::Get().getVkDevice(), &allocInfo, &commandBuffer);

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

    vkQueueSubmit(vkGlobalPool::Get().getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkGlobalPool::Get().getGraphicsQueue());


    vkFreeCommandBuffers(vkGlobalPool::Get().getVkDevice(), vkGlobalPool::Get().getCommandPool(), 1, &commandBuffer);
}
void Vulkan::createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(vkGlobalPool::Get().indices[0]) * vkGlobalPool::Get().indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(vkGlobalPool::Get().getVkDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vkGlobalPool::Get().indices.data(), (size_t) bufferSize);
    vkUnmapMemory(vkGlobalPool::Get().getVkDevice(), stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkGlobalPool::Get().getIndexBuffer(),
                 vkGlobalPool::Get().getIndexBufferMemory());

    copyBuffer(stagingBuffer, vkGlobalPool::Get().getIndexBuffer(), bufferSize);

    vkDestroyBuffer(vkGlobalPool::Get().getVkDevice(), stagingBuffer, nullptr);
    vkFreeMemory(vkGlobalPool::Get().getVkDevice(), stagingBufferMemory, nullptr);
}

void Vulkan::createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vkGlobalPool::Get().vertices[0]) * vkGlobalPool::Get().vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(vkGlobalPool::Get().getVkDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vkGlobalPool::Get().vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(vkGlobalPool::Get().getVkDevice(), stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkGlobalPool::Get().getVertexBuffer(),
                 vkGlobalPool::Get().getVertexBufferMemory());

    copyBuffer(stagingBuffer, vkGlobalPool::Get().getVertexBuffer(), bufferSize);
    vkDestroyBuffer(vkGlobalPool::Get().getVkDevice(), stagingBuffer, nullptr);
    vkFreeMemory(vkGlobalPool::Get().getVkDevice(), stagingBufferMemory, nullptr);
}



Vulkan::~Vulkan() {
    vkGlobalPool& globalPool = vkGlobalPool::Get();


    for (size_t i = 0; i < globalPool.getMaxFramesInFlight(); i++) {
        vkDestroySemaphore(globalPool.getVkDevice(), globalPool.renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(globalPool.getVkDevice(), globalPool.imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(globalPool.getVkDevice(), inFlightFences[i], nullptr);
    }
    vkDestroyCommandPool(globalPool.getVkDevice(), globalPool.getCommandPool(), nullptr);

    delete swapchain;
    vkDestroyBuffer(globalPool.getVkDevice(), globalPool.getVertexBuffer(), nullptr);
    vkFreeMemory(globalPool.getVkDevice(), globalPool.getVertexBufferMemory(), nullptr);
    vkDestroyBuffer(globalPool.getVkDevice(), globalPool.getIndexBuffer(), nullptr);
    vkFreeMemory(globalPool.getVkDevice(), globalPool.getIndexBufferMemory(), nullptr);
    delete graphicsPipeline;
    vkDestroySwapchainKHR(globalPool.getVkDevice(), globalPool.getSwapChain(), nullptr);
    delete device;
    delete surface;
    delete vulkanInstance;
    delete window;
    glfwTerminate();
}

void Vulkan::initWindow(int width, int height, const char *title, bool resizableWindow, bool fullscreen) {
    window = new Window(width, height, title, resizableWindow);
    vkGlobalPool::Get().setWindow(window->GetWindowHandle());
}

void Vulkan::mainloop() {
    while (!glfwWindowShouldClose(window->GetWindowHandle())) {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(vkGlobalPool::Get().getVkDevice());
}

void Vulkan::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = vkGlobalPool::Get().getQueueFamilyIndices().graphicsFamily.value();
    poolInfo.flags = 0; // Optional

    VkCommandPool commandPool;
    if (vkCreateCommandPool(vkGlobalPool::Get().getVkDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }

    vkGlobalPool::Get().setCommandPool(commandPool);
}

void Vulkan::createCommandBuffers() {
    vkGlobalPool& globalPool = vkGlobalPool::Get();

    commandBuffers.resize(vkGlobalPool::Get().getSwapChainFrameBuffers().size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = globalPool.getCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(globalPool.getVkDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }

    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = globalPool.getVkRenderPass();
        renderPassInfo.framebuffer = vkGlobalPool::Get().getSwapChainFrameBuffers()[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = globalPool.getSwapChainExtent();

        VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, globalPool.getVkPipeline());

        VkBuffer vertexBuffers[] = {vkGlobalPool::Get().getVertexBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(commandBuffers[i], vkGlobalPool::Get().getIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);

        vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(vkGlobalPool::Get().indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

void Vulkan::drawFrame() {
    vkGlobalPool& globalPool = vkGlobalPool::Get();

    ///Wait for the fence representing the image we want to render to finish submitting to the gpu before submitting another command buffer
    vkWaitForFences(globalPool.getVkDevice(), 1, &inFlightFences[globalPool.getCurrentFrame()], VK_TRUE, UINT64_MAX);

    ///Get next position/index in swap buffer.
    uint32_t imageIndex;
    vkAcquireNextImageKHR(globalPool.getVkDevice(), globalPool.getSwapChain(), UINT64_MAX, globalPool.imageAvailableSemaphores[globalPool.getCurrentFrame()], VK_NULL_HANDLE, &imageIndex);

    ///If image is being presented then wait for it to finish, if fence does not yet exist then skip.
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(globalPool.getVkDevice(), 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }

    ///Assign the respective fence to a specific image.
    imagesInFlight[imageIndex] = inFlightFences[globalPool.getCurrentFrame()];


    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {globalPool.imageAvailableSemaphores[globalPool.getCurrentFrame()]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {globalPool.renderFinishedSemaphores[globalPool.getCurrentFrame()]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    ///Reset fence, prep it for display.
    vkResetFences(globalPool.getVkDevice(), 1, &inFlightFences[globalPool.getCurrentFrame()]);

    ///Submit graphics queue and continue if successful
    if (vkQueueSubmit(globalPool.getGraphicsQueue(), 1, &submitInfo, inFlightFences[globalPool.getCurrentFrame()]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {globalPool.getSwapChain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr;

    ///Flip buffers, vibe.
    vkQueuePresentKHR(globalPool.getPresentQueue(), &presentInfo);

    globalPool.setCurrentFrame((globalPool.getCurrentFrame()+1) % globalPool.getMaxFramesInFlight());
}

void Vulkan::createSyncObjects() {
    vkGlobalPool& globalPool = vkGlobalPool::Get();

    inFlightFences.resize(globalPool.getMaxFramesInFlight());
    imagesInFlight.resize(globalPool.getSwapChainImages().size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < globalPool.getMaxFramesInFlight(); i++) {
        if (vkCreateSemaphore(globalPool.getVkDevice(), &semaphoreInfo, nullptr,
                              &globalPool.imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(globalPool.getVkDevice(), &semaphoreInfo, nullptr,
                              &globalPool.renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(globalPool.getVkDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create semaphores for a frame!");
        }
    }
}