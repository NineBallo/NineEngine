//
// Created by nineball on 7/6/21.
//

#include "Display.h"
#include <iostream>
#include <VkBootstrap.h>
#include <math.h>
#include <Initializers.h>

Display::Display(const displayCreateInfo& createInfo) {
    createWindow(createInfo.extent, createInfo.title, createInfo.resizable);

    if(createInfo.instance != VK_NULL_HANDLE) {
        createSurface(createInfo.instance);
    }

    if(createInfo.device != VK_NULL_HANDLE && createInfo.GPU != VK_NULL_HANDLE && createInfo.queue != VK_NULL_HANDLE) {
        createSwapchain(createInfo.device, createInfo.GPU, createInfo.queue);
    }

    mExtent = createInfo.extent;
    mInstance = createInfo.instance;
}

Display::~Display() {
    vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);

    if(mRenderpass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(mDevice, mRenderpass, nullptr);;
    }

    if(!mFramebuffers.empty() && !mImageViews.empty()) {
        for(int i = 0; i < mFramebuffers.size(); i++) {
            vkDestroyFramebuffer(mDevice, mFramebuffers[i], nullptr);
            vkDestroyImageView(mDevice, mImageViews[i], nullptr);
        }
    }

    if(mSurface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
    }

    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

void Display::createWindow(VkExtent2D extent, const std::string& title, bool resizable) {
    glfwInit();

    ///Dont create an opengl context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (resizable) {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    } else {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    }

    mWindow = glfwCreateWindow(extent.width, extent.height, title.c_str(), nullptr, nullptr);

    //  glfwSetWindowUserPointer(window, vulkan);
    //  glfwSetFramebufferSizeCallback(window, VKBareAPI::Window::framebufferResizeCallback);
}

void Display::createSurface(VkInstance instance) {
    if (glfwCreateWindowSurface(instance, mWindow, nullptr, &mSurface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!\n");
    }
}

void Display::createSwapchain(VkDevice device, VkPhysicalDevice GPU, VkQueue presentQueue) {
    vkb::SwapchainBuilder swapchainBuilder{GPU, device, mSurface};
    vkb::Swapchain vkbSwapchain = swapchainBuilder
            .use_default_format_selection()
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(mExtent.width, mExtent.height)
            .build()
            .value();

    mSwapchain = vkbSwapchain.swapchain;
    mImages = vkbSwapchain.get_images().value();
    mImageViews = vkbSwapchain.get_image_views().value();
    mFormat = vkbSwapchain.image_format;

    mDevice = device;

    mPresentQueue = presentQueue;
}

void Display::createFramebuffers(VkRenderPass renderpass) {
    mRenderpass = renderpass;
    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = nullptr;

    fb_info.renderPass = mRenderpass;
    fb_info.attachmentCount = 1;
    fb_info.width = mExtent.width;
    fb_info.height = mExtent.height;
    fb_info.layers = 1;

    //Swapchain size
    const uint32_t swapchain_imagecount = mImages.size();
    mFramebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

    //Create a corresponding framebuffer for each image
    for (int i = 0; i < swapchain_imagecount; i++) {

        fb_info.pAttachments = &mImageViews[i];
        if(vkCreateFramebuffer(mDevice, &fb_info, nullptr, &mFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer\n");
        };
    }
}

void Display::createSyncStructures() {
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = nullptr;

    //Create it already signaled to streamline renderloop
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &mRenderFence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create synchronization fences\n");
    };

    //For the semaphores we don't need any flags
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    if (vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &mPresentSemaphore) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create present semaphore\n");
    };

    if (vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &mRenderSemaphore) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render semaphore\n");
    }
}

void Display::createCommandBuffer(VkCommandPool cmdPool) {
    //Create Command Buffer
    VkCommandBufferAllocateInfo cmdAllocInfo = init::command_buffer_allocate_info(cmdPool, 1);
    if (vkAllocateCommandBuffers(mDevice, &cmdAllocInfo, &mPrimaryCommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create display primary command buffer\n");
    }
}

VkCommandBuffer Display::startFrame() {
    //Wait for frame to be ready/(returned to "back")
    vkWaitForFences(mDevice, 1, &mRenderFence, true, 1000000000);
    vkResetFences(mDevice, 1, &mRenderFence);

    //Get current swapchain index
    vkAcquireNextImageKHR(mDevice, mSwapchain, 1000000000, mPresentSemaphore, nullptr, &mSwapchainImageIndex);

    //Wipe and prep command buffer to be handed to the renderer
    vkResetCommandBuffer(mPrimaryCommandBuffer, 0);

    VkCommandBufferBeginInfo cmdBeginInfo = {};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;
    cmdBeginInfo.pInheritanceInfo = nullptr;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(mPrimaryCommandBuffer, &cmdBeginInfo);

    //Clear framebuffer
    VkClearValue clearValue;
    float flash = abs(sin(mFrameNumber / 120.f));
    clearValue.color = { { 0.0f, 0.0f, flash, 1.0f } };

    //Lets start painting
    VkRenderPassBeginInfo rpInfo = {};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.pNext = nullptr;

    rpInfo.renderPass = mRenderpass;
    rpInfo.renderArea.offset.x = 0;
    rpInfo.renderArea.offset.y = 0;
    rpInfo.renderArea.extent = mExtent;
    rpInfo.framebuffer = mFramebuffers[mSwapchainImageIndex];

    //Set the value to clear to in renderpass
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(mPrimaryCommandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkRect2D scissor = {0,0, mExtent.width, mExtent.height};
    vkCmdSetScissor(mPrimaryCommandBuffer, 0, 1, &scissor);

    VkViewport viewport;
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = mExtent.width;
    viewport.height = mExtent.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(mPrimaryCommandBuffer, 0, 1, &viewport);
    //Farewell command buffer o/; May your commands be true, and your errors gentle.
    return mPrimaryCommandBuffer;
}

void Display::endFrame() {
    //Though wise men at their end know dark is right,
    //Because their words had forked no lightning they
    //Do not go gentle into that good night.

    vkCmdEndRenderPass(mPrimaryCommandBuffer);
    vkEndCommandBuffer(mPrimaryCommandBuffer);

    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pNext = nullptr;

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    submit.pWaitDstStageMask = &waitStage;

    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &mPresentSemaphore;

    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &mRenderSemaphore;

    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &mPrimaryCommandBuffer;

    //Submit command buffer and execute it.
    vkQueueSubmit(mPresentQueue, 1, &submit, mRenderFence);


    //Grave men, near death, who see with blinding sight
    //Blind eyes could blaze like meteors and be gay,
    //Rage, rage against the dying of the light.
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;

    presentInfo.pSwapchains = &mSwapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores = &mRenderSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &mSwapchainImageIndex;

    vkQueuePresentKHR(mPresentQueue, &presentInfo);


    mFrameNumber++;
}


bool Display::shouldExit() {
    if (!glfwWindowShouldClose(mWindow)) {
        glfwPollEvents();
        return false;
    } else {
        return true;
    }
}


VkSurfaceKHR Display::surface() {return mSurface;}
uint16_t Display::frameNumber() {return mFrameNumber;}
VkFormat Display::format() {return mFormat;}
VkExtent2D Display::extent() {return mExtent;}