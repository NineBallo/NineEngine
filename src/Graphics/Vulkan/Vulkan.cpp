//
// Created by nineball on 12/27/20.
//

#include "Vulkan.h"



///TODO fullscreen implementation.
void Vulkan::initVulkan() {
    vulkanInstance = new VulkanInstance();
    surface = new Surface();
    device = new Device();
    createSwapChain();
    createImageViews();
   // renderPass = new RenderPass(logicalDevice->getLogicalDevice(), &swapChainImageFormat);
    graphicsPipeline = new GraphicsPipeline();
    createFrameBuffers();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

Vulkan::Vulkan(int width, int height, const char *title, bool resizableWindow, bool fullscreen) {

    initWindow(width, height, title, resizableWindow, resizableWindow);
    initVulkan();
    mainloop();

}

Vulkan::~Vulkan() {
    vkGlobalPool& globalPool = vkGlobalPool::Get();



    for (size_t i = 0; i < globalPool.getMaxFramesInFlight(); i++) {
        vkDestroySemaphore(globalPool.getVkDevice(), globalPool.renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(globalPool.getVkDevice(), globalPool.imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(globalPool.getVkDevice(), inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(globalPool.getVkDevice(), globalPool.getCommandPool(), nullptr);

    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(globalPool.getVkDevice(), framebuffer, nullptr);
    }

    delete graphicsPipeline;

    for (auto imageview : swapChainImageViews){
        vkDestroyImageView(globalPool.getVkDevice(), imageview, nullptr);
    }

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

VkSurfaceFormatKHR Vulkan::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Vulkan::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Vulkan::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;

        glfwGetFramebufferSize(window->GetWindowHandle(), &width, &height);

        VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
        };

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

void Vulkan::createSwapChain() {
    VkSwapchainKHR swapChain;

    SwapChainSupportDetails swapChainSupport = device->querySwapChainSupport(vkGlobalPool::Get().getVkPhysicalDevice());

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    ///How many images are in the swapchain
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = vkGlobalPool::Get().getVkSurfaceKhr();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    ///always one unless stereoscopic 3D
    createInfo.imageArrayLayers = 1;
    ///     For rendering directly to image we do this, for post processing it will be changed to
    ///VK_IMAGE_USAGE_TRANSFER_DST_BIT as that allows us to do the render in memory and copy it to the
    ///swapchain instead of directly drawing to it.
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    ///Queue family sharing thing.
    vkGlobalPool::QueueFamilyIndices indices = vkGlobalPool::Get().getQueueFamilyIndices();
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    ///No transform applied, ignore alpha channel
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    ///We dont care about obscured pixels
    createInfo.clipped = VK_TRUE;

    ////A new SwapChain needs to be created for each window resize.
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(vkGlobalPool::Get().getVkDevice(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain!");
    }
    else {
        std::cout << "SwapChain probably successfully created.\n";
    }

    vkGetSwapchainImagesKHR(vkGlobalPool::Get().getVkDevice(), swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vkGlobalPool::Get().getVkDevice(), swapChain, &imageCount, swapChainImages.data());


    vkGlobalPool::Get().setSwapChain(swapChain);
    vkGlobalPool::Get().setSwapChainImageFormat(surfaceFormat.format);
    vkGlobalPool::Get().setSwapChainExtent(extent);
}

void Vulkan::createImageViews() {
    //Want them to be the same length. 1 view, 1 image.
    swapChainImageViews.resize(swapChainImages.size());

    for(size_t i = 0; i < swapChainImages.size(); i++){
        VkImageViewCreateInfo  createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        ////TODO 3D???!?!?
        createInfo.viewType =  VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = vkGlobalPool::Get().getSwapChainImageFormat();

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if(vkCreateImageView(vkGlobalPool::Get().getVkDevice(), &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS){
            throw std::runtime_error("Failed to create image views!");
        }
    }

}

void Vulkan::createFrameBuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
                swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = vkGlobalPool::Get().getVkRenderPass();
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = vkGlobalPool::Get().getSwapChainExtent().width;
        framebufferInfo.height = vkGlobalPool::Get().getSwapChainExtent().height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vkGlobalPool::Get().getVkDevice(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
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

    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = globalPool.getCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(globalPool.getVkDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = globalPool.getVkRenderPass();
        renderPassInfo.framebuffer = swapChainFramebuffers[i];
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

    vkWaitForFences(globalPool.getVkDevice(), 1, &inFlightFences[globalPool.getCurrentFrame()], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(globalPool.getVkDevice(), globalPool.getSwapChain(), UINT64_MAX, globalPool.imageAvailableSemaphores[globalPool.getCurrentFrame()], VK_NULL_HANDLE, &imageIndex);

    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(globalPool.getVkDevice(), 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
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

    vkResetFences(globalPool.getVkDevice(), 1, &inFlightFences[globalPool.getCurrentFrame()]);

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

    vkQueuePresentKHR(globalPool.getPresentQueue(), &presentInfo);

    globalPool.setCurrentFrame((globalPool.getCurrentFrame()+1) % globalPool.getMaxFramesInFlight());
}

void Vulkan::createSyncObjects() {
    vkGlobalPool& globalPool = vkGlobalPool::Get();

    inFlightFences.resize(globalPool.getMaxFramesInFlight());
    imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

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

      //  inFlightFences[i] = fence;

    }
}