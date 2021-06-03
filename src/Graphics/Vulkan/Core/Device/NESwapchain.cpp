//
// Created by nineball on 5/29/21.
//

#include "NESwapchain.h"
//
// Created by nineball on 4/16/21.
//

#include "NESwapchain.h"
#include <iostream>
#include "cstring"
#include <cmath>

namespace NEVK {
    NESwapchain::NESwapchain(NEDevice* device_, VkExtent2D extent, VkSurfaceKHR surface) : device{device_}, mExtent(extent), mSurface(surface) {
        vkGetDeviceQueue(*device, device->getIndices().presentFamily.value(), 0, &presentQueue);
        vkGetDeviceQueue(*device, device->getIndices().graphicsFamily.value(), 0, &graphicsQueue);
        createSwapChain();
        createSyncObjects();
     //   createFrameBuffers();
    }

    NESwapchain::~NESwapchain() {

    }



    void NESwapchain::createSwapChain() {

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(device->getSwapChainDetails().formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(device->getSwapChainDetails().presentModes);
        VkSurfaceCapabilitiesKHR capabilities = device->getSwapChainDetails().capabilities;
        device->setFormat(surfaceFormat.format);

        ///How many images are in the swapchain
        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = window->getSurface();
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = window->getExtent();
        ///always one unless stereoscopic 3D
        createInfo.imageArrayLayers = 1;
        ///     For rendering directly to image we do this, for post processing it will be changed to
        ///VK_IMAGE_USAGE_TRANSFER_DST_BIT as that allows us to do the render in memory and copy it to the
        ///swapchain instead of directly drawing to it.
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        ///Queue family sharing thing.


        if (device->getIndices().graphicsFamily != device->getIndices().presentFamily) {
            uint32_t queueFamilyIndices[] = {device->getIndices().graphicsFamily.value(),
                                             device->getIndices().presentFamily.value()};
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        ///No transform applied, ignore alpha channel
        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode = presentMode;
        ///We dont care about obscured pixels
        createInfo.clipped = VK_TRUE;

        ////A new SwapChain needs to be created for each window resize.
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(*device, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swap chain!");
        }

        ///Populate swapchainVar struct
        vkGetSwapchainImagesKHR(*device, swapchain, &imageCount, nullptr);
        images.resize(imageCount);
        vkGetSwapchainImagesKHR(*device, swapchain, &imageCount, images.data());


        mFormat = surfaceFormat.format;

        createImageViews();
    }

    void NESwapchain::createFrameBuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                    swapChainImageViews[i]
            };
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = *device->getRenderpass();
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = mExtent.width;
            framebufferInfo.height = mExtent.height;
            framebufferInfo.layers = 1;
            if (vkCreateFramebuffer(*device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void NESwapchain::createImageViews() {
        //Want them to be the same length. 1 view, 1 image.
        imageViews.resize(images.size());

        for (size_t i = 0; i < images.size(); i++) {
            imageViews[i] = createImageView(images[i], mFormat);
        }
    }

    VkImageView NESwapchain::createImageView(VkImage image, VkFormat imageFormat) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = image;

        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = imageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(*device, &createInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image view!");
        }
        return imageView;
    }

    VkSurfaceFormatKHR NESwapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
        for (const auto &availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }



    VkPresentModeKHR NESwapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
        for (const auto &availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

//    void NESwapchain::destroy() {
//    std::cout << "Destroying swapchain :(\n";
//    cleanupSwapChain();
//
//    vkDestroySampler(device, textureSampler, nullptr);
//
//    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
//        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
//        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
//        vkDestroyFence(device, inFlightFences[i], nullptr);
//    }
//    }


//    void NESwapchain::cleanupSwapChain() {
//
//    for (auto framebuffer : swapChainFramebuffers) {
//        vkDestroyFramebuffer(device, framebuffer, nullptr);
//    }
//
//    vkFreeCommandBuffers(device, device.getCommandPool(), static_cast<uint32_t>(device.getCommandBuffers().size()), device.getCommandBuffers().data());
//
//    vkDestroyPipeline(device, pipeline, nullptr);
//    vkDestroyPipelineLayout(device, pipeline.pipelineLayout, nullptr);
//    vkDestroyRenderPass(device, pipelineVars.renderPass, nullptr);
//
//    for (auto imageView : swapChainImageViews) {
//        vkDestroyImageView(device, imageView, nullptr);
//    }
//
//    vkDestroySwapchainKHR(device, swapchain, nullptr);
//
//    for (size_t i = 0; i < swapChainImages.size(); i++) {
//        vkDestroyBuffer(device, device.getUniformBuffers()[i], nullptr);
//        vkFreeMemory(device, device.getUniformBuffersMemory()[i], nullptr);
//    }
//    vkDestroyDescriptorPool(device, device.getDescriptorPool(), nullptr);
//    }



//void Swapchain::recreateSwapChain() {
//    int width = 0, height = 0;
//    glfwGetFramebufferSize(window, &width, &height);
//    while (width == 0 || height == 0) {
//        glfwGetFramebufferSize(window, &width, &height);
//        glfwWaitEvents();
//    }
//
//    vkDeviceWaitIdle(device);
//
//    cleanupSwapChain();
//
//    Swapchain::createSwapChain(swapchainVars, deviceVars, windowVars);
//    Pipeline::Renderpass::createRenderPass(pipelineVars.renderPass, deviceVars.device, swapchainVars.swapChainImageFormat);
//    Pipeline::createPipeline(pipelineVars, deviceVars.device, swapchainVars);
//    Pipeline::createFrameBuffers(pipelineVars, deviceVars.device, swapchainVars);
//    Buffers::createUniformBuffers(deviceVars, swapchainVars);
//    Buffers::createDescriptorPool(deviceVars, swapchainVars);
//    Buffers::createDescriptorSets(deviceVars, swapchainVars, pipelineVars);
//    Buffers::createCommandBuffers(deviceVars, swapchainVars, pipelineVars);
//
//    swapchainVars.imagesInFlight.resize(swapchainVars.swapChainImages.size(), VK_NULL_HANDLE);
//}

    void NESwapchain::createSyncObjects() {

        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        imagesInFlight.resize(images.size(), VK_NULL_HANDLE);

        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(*device, &semaphoreInfo, nullptr,
                                  &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(*device, &semaphoreInfo, nullptr,
                                  &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(*device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

                throw std::runtime_error("Failed to create semaphores for a frame!");
            }
        }
    }

    void NESwapchain::startFrame() {
        ///Wait for the fence representing the image we want to render to finish submitting to the gpu before submitting another command buffer

        vkWaitForFences(*device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        ///Get next position/index in swap buffer.
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(*device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
                                                VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
//            recreateSwapChain();
            return;
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to acquire swap chain image!");
        }

        ///If image is being presented then wait for it to finish, if fence does not yet exist then skip.
        if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(*device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }

        ///Assign the respective fence to a specific image.
        imagesInFlight[imageIndex] = inFlightFences[currentFrame];

 //       updateUniformBuffer(imageIndex);
    }

    void NESwapchain::submitCommandBuffer(VkCommandBuffer commandBuffer) {
    //    updateUniformBuffer(imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        ///Submit graphics queue and continue if successful
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }
    }

    void NESwapchain::endFrame() {
        ///Reset fence, prep it for display.
        vkResetFences(*device, 1, &inFlightFences[currentFrame]);

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapChains[] = {swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;
        ///Flip buffers, vibe.
        VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
     //       recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swap chain image!");
        }
        currentFrame = (currentFrame+1) % MAX_FRAMES_IN_FLIGHT;
    }


    int NESwapchain::getFrame() {return currentFrame;}
}