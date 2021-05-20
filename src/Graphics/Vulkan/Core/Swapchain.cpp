//
// Created by nineball on 4/16/21.
//

#include "Swapchain.h"
#include <iostream>

#include "Pipeline.h"
#include "Buffers.h"
#include "Renderpass.h"

#include "unistd.h"

namespace VKBareAPI::Swapchain {
    void createSwapChain(NESwapchain &swapchainVars, Device::NEDevice deviceVars, Window::NEWindow &windowVars){
        swapchainVars.swapChainSupportDetails = querySwapChainSupport(deviceVars.physicalDevice, windowVars.surface);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainVars.swapChainSupportDetails.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainVars.swapChainSupportDetails.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapchainVars.swapChainSupportDetails.capabilities, windowVars);

        ///How many images are in the swapchain
        uint32_t imageCount = swapchainVars.swapChainSupportDetails.capabilities.minImageCount + 1;
        if (swapchainVars.swapChainSupportDetails.capabilities.maxImageCount > 0 && imageCount > swapchainVars.swapChainSupportDetails.capabilities.maxImageCount) {
            imageCount = swapchainVars.swapChainSupportDetails.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = windowVars.surface;
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

        uint32_t queueFamilyIndices[] = {deviceVars.indices.graphicsFamily.value(), deviceVars.indices.presentFamily.value()};
        if (deviceVars.indices.graphicsFamily != deviceVars.indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        ///No transform applied, ignore alpha channel
        createInfo.preTransform = swapchainVars.swapChainSupportDetails.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode = presentMode;
        ///We dont care about obscured pixels
        createInfo.clipped = VK_TRUE;

        ////A new SwapChain needs to be created for each window resize.
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(deviceVars.device, &createInfo, nullptr, &swapchainVars.swapchain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swap chain!");
        }

        ///Populate swapchainVar struct
        vkGetSwapchainImagesKHR(deviceVars.device, swapchainVars.swapchain, &imageCount, nullptr);
        swapchainVars.swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(deviceVars.device, swapchainVars.swapchain, &imageCount, swapchainVars.swapChainImages.data());

        swapchainVars.swapChainImageFormat = surfaceFormat.format;
        swapchainVars.swapChainExtent = extent;

        createImageViews(swapchainVars, deviceVars.device);
    }

    void createImageViews(NESwapchain &swapchainVars, VkDevice device) {
        //Want them to be the same length. 1 view, 1 image.
        swapchainVars.swapChainImageViews.resize(swapchainVars.swapChainImages.size());

        for(size_t i = 0; i < swapchainVars.swapChainImages.size(); i++){
            VkImageViewCreateInfo  createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapchainVars.swapChainImages[i];
            ////TODO 3D???!?!?
            createInfo.viewType =  VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapchainVars.swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if(vkCreateImageView(device, &createInfo, nullptr, &swapchainVars.swapChainImageViews[i]) != VK_SUCCESS){
                throw std::runtime_error("Failed to create image views!");
            }
        }
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, Window::NEWindow windowVars) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            std::cout << capabilities.currentExtent.width << "X" << capabilities.currentExtent.height << "\n";
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(windowVars.window, &width, &height);

            VkExtent2D actualExtent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
            };

            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
            std::cout << actualExtent.width << "X" << actualExtent.height << "\n";
            return actualExtent;
        }
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    void destroy(NESwapchain &swapchainVars, Device::NEDevice &deviceVars, Pipeline::NEPipeline &pipelineVars){
        std::cout << "Destroying swapchain :(\n";
        cleanupSwapChain(swapchainVars, deviceVars, pipelineVars);

        for (size_t i = 0; i < swapchainVars.MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(deviceVars.device, swapchainVars.renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(deviceVars.device, swapchainVars.imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(deviceVars.device, swapchainVars.inFlightFences[i], nullptr);
        }
    }


    void drawFrame(NESwapchain &swapchainVars, Device::NEDevice &deviceVars, Pipeline::NEPipeline &pipelineVars, Window::NEWindow &windowVars){

        ///Wait for the fence representing the image we want to render to finish submitting to the gpu before submitting another command buffer
        vkWaitForFences(deviceVars.device, 1, &swapchainVars.inFlightFences[swapchainVars.currentFrame], VK_TRUE, UINT64_MAX);

        ///Get next position/index in swap buffer.
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(deviceVars.device, swapchainVars.swapchain, UINT64_MAX, swapchainVars.imageAvailableSemaphores[swapchainVars.currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain(swapchainVars, deviceVars, pipelineVars, windowVars);
            return;
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to acquire swap chain image!");
        }

        Buffers::updateUniformBuffer(imageIndex, deviceVars, swapchainVars);

        ///If image is being presented then wait for it to finish, if fence does not yet exist then skip.
        if (swapchainVars.imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(deviceVars.device, 1, &swapchainVars.imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }
        ///Assign the respective fence to a specific image.
        swapchainVars.imagesInFlight[imageIndex] = swapchainVars.inFlightFences[swapchainVars.currentFrame];


        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {swapchainVars.imageAvailableSemaphores[swapchainVars.currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &deviceVars.Buffers.commandBuffers[imageIndex];

        VkSemaphore signalSemaphores[] = {swapchainVars.renderFinishedSemaphores[swapchainVars.currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        ///Reset fence, prep it for display.
        vkResetFences(deviceVars.device, 1, &swapchainVars.inFlightFences[swapchainVars.currentFrame]);

        ///Submit graphics queue and continue if successful
        if (vkQueueSubmit(deviceVars.graphicsQueue, 1, &submitInfo, swapchainVars.inFlightFences[swapchainVars.currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapchainVars.swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        presentInfo.pResults = nullptr;

        ///Flip buffers, vibe.
        result = vkQueuePresentKHR(deviceVars.presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || swapchainVars.framebufferResized) {
            swapchainVars.framebufferResized = false;
            recreateSwapChain(swapchainVars, deviceVars, pipelineVars, windowVars);
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swap chain image!");
        }

        swapchainVars.currentFrame = (swapchainVars.currentFrame+1) % swapchainVars.MAX_FRAMES_IN_FLIGHT;
    }

    void cleanupSwapChain(NESwapchain &swapchainVars, Device::NEDevice &deviceVars, Pipeline::NEPipeline &pipelineVars) {

        for (auto framebuffer : swapchainVars.swapChainFramebuffers) {
            vkDestroyFramebuffer(deviceVars.device, framebuffer, nullptr);
        }

        vkFreeCommandBuffers(deviceVars.device, deviceVars.commandPool, static_cast<uint32_t>(deviceVars.Buffers.commandBuffers.size()), deviceVars.Buffers.commandBuffers.data());

        vkDestroyPipeline(deviceVars.device, pipelineVars.pipeline, nullptr);
        vkDestroyPipelineLayout(deviceVars.device, pipelineVars.pipelineLayout, nullptr);
        vkDestroyRenderPass(deviceVars.device, pipelineVars.renderPass, nullptr);

        for (auto imageView : swapchainVars.swapChainImageViews) {
            vkDestroyImageView(deviceVars.device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(deviceVars.device, swapchainVars.swapchain, nullptr);

        for (size_t i = 0; i < swapchainVars.swapChainImages.size(); i++) {
            vkDestroyBuffer(deviceVars.device, deviceVars.Buffers.uniformBuffers[i], nullptr);
            vkFreeMemory(deviceVars.device, deviceVars.Buffers.uniformBuffersMemory[i], nullptr);
        }
        vkDestroyDescriptorPool(deviceVars.device, deviceVars.descriptorPool, nullptr);
    }

    void recreateSwapChain(NESwapchain &swapchainVars, Device::NEDevice &deviceVars, Pipeline::NEPipeline &pipelineVars, Window::NEWindow &windowVars) {
        int width = 0, height = 0;
        glfwGetFramebufferSize(windowVars.window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(windowVars.window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(deviceVars.device);

        cleanupSwapChain(swapchainVars, deviceVars, pipelineVars);

        Swapchain::createSwapChain(swapchainVars, deviceVars, windowVars);
        Pipeline::Renderpass::createRenderPass(pipelineVars.renderPass, deviceVars.device, swapchainVars.swapChainImageFormat);
        Pipeline::createPipeline(pipelineVars, deviceVars.device, swapchainVars);
        Pipeline::createFrameBuffers(pipelineVars, deviceVars.device, swapchainVars);
        Buffers::createUniformBuffers(deviceVars, swapchainVars);
        Buffers::createDescriptorPool(deviceVars, swapchainVars);
        Buffers::createDescriptorSets(deviceVars, swapchainVars, pipelineVars);
        Buffers::createCommandBuffers(deviceVars, swapchainVars, pipelineVars);

        swapchainVars.imagesInFlight.resize(swapchainVars.swapChainImages.size(), VK_NULL_HANDLE);
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }
}