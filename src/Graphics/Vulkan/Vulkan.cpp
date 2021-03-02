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
    createCommandBuffers();
    createSyncObjects();
}

Vulkan::Vulkan(int width, int height, const char *title, bool resizableWindow, bool fullscreen) {
    initWindow(width, height, title, resizableWindow, resizableWindow);
    initVulkan();
    mainloop();
}

void Vulkan::createVertexBuffer() {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(vkGlobalPool::Get().vertices[0]) * vkGlobalPool::Get().vertices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(vkGlobalPool::Get().getVkDevice(), &bufferInfo, nullptr, &vkGlobalPool::Get().getVertexBuffer()) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }
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

        vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

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