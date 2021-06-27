//
// Created by nineball on 6/14/21.
//

#include "../NEDisplay.h"
#include "../NEShared.h"
#include "iostream"
#include "../../Helper/Display.h"
#include "../../Helper/Device.h"
#include <utility>

NEDisplay::NEDisplay(int width, int height, std::string title, bool resizable, VkInstance instance) :
        mWidth(width), mHeight(height), mTitle(std::move(title)), mResizable(resizable),
        mPhysicalDevice(VK_NULL_HANDLE), mDevice(VK_NULL_HANDLE), mInstance(instance) {

    createWindow();
    createSurface();
}

NEDisplay::~NEDisplay() {std::cout << "display gone\n";}

void NEDisplay::createWindow() {
    glfwInit();

    ///Dont create an opengl context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (mResizable) {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    } else {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    }

    mWindow = glfwCreateWindow(mWidth, mHeight, mTitle.c_str(), nullptr, nullptr);
 //  glfwSetWindowUserPointer(window, vulkan);
 //  glfwSetFramebufferSizeCallback(window, VKBareAPI::Window::framebufferResizeCallback);
}

void NEDisplay::startFrame() {
    ///Wait for the fence representing the image we want to render to finish submitting to the gpu before submitting another command buffer

    vkWaitForFences(mDevice, 1, &mInFlightFences[mCurrentFrame], VK_TRUE, UINT64_MAX);
    ///Get next position/index in swap buffer.
    VkResult result = vkAcquireNextImageKHR(mDevice, mSwapchain, UINT64_MAX, mImageAvailableSemaphores[mCurrentFrame],
                                            VK_NULL_HANDLE, &mImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
//            recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    ///If image is being presented then wait for it to finish, if fence does not yet exist then skip.
    if (mImagesInFlight[mImageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(mDevice, 1, &mImagesInFlight[mImageIndex], VK_TRUE, UINT64_MAX);
    }

    ///Assign the respective fence to a specific image.
    mImagesInFlight[mImageIndex] = mInFlightFences[mCurrentFrame];

    //       updateUniformBuffer(imageIndex);
};

void NEDisplay::endFrame() {
    ///Reset fence, prep it for display.
    vkResetFences(mDevice, 1, &mInFlightFences[mCurrentFrame]);

    VkSemaphore signalSemaphores[] = {mRenderFinishedSemaphores[mCurrentFrame]};

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {mSwapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &mImageIndex;
    presentInfo.pResults = nullptr;

    ///Flip buffers, vibe.
    VkResult result = vkQueuePresentKHR(mPresentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || mFramebufferResized) {
        mFramebufferResized = false;
        //       recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image!");
    }
    mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
};

int NEDisplay::currentFrame() {
    return mCurrentFrame;
};

void NEDisplay::createSyncObjects() {
    mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    mImagesInFlight.resize(mImages.size(), VK_NULL_HANDLE);

    mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    mRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr,
                              &mImageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr,
                              &mRenderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(mDevice, &fenceInfo, nullptr, &mInFlightFences[i]) != VK_SUCCESS) {

            throw std::runtime_error("Failed to create semaphores for a frame!");
        }
    }
};

void NEDisplay::recreateSwapchain(NEDevice *device) {
    mPhysicalDevice = device->GPU();
    mDevice = *device;


    mSupportDetails = querySwapChainSupport(mPhysicalDevice, mSurface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(mSupportDetails.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(mSupportDetails.presentModes);
    mCapabilities = mSupportDetails.capabilities;
    chooseSwapExtent();

    ///How many images are in the swapchain
    uint32_t imageCount = mCapabilities.minImageCount + 1;
    if (mCapabilities.maxImageCount > 0 && imageCount > mCapabilities.maxImageCount) {
        imageCount = mCapabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = mSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = mExtent;
    ///always one unless stereoscopic 3D
    createInfo.imageArrayLayers = 1;
    ///     For rendering directly to image we do this, for post processing it will be changed to
    ///VK_IMAGE_USAGE_TRANSFER_DST_BIT as that allows us to do the render in memory and copy it to the
    ///swapchain instead of directly drawing to it.
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    ///Queue family sharing thing.
    uint32_t queueFamilyIndices[] = {mQueueFamilys.graphicsFamily.value(), mQueueFamilys.presentFamily.value()};
    if (mQueueFamilys.graphicsFamily != mQueueFamilys.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    ///No transform applied, ignore alpha channel
    createInfo.preTransform = mCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    ///We dont care about obscured pixels
    createInfo.clipped = VK_TRUE;

    ////A new SwapChain needs to be created for each window resize
    createInfo.oldSwapchain = mLastSwapchain;

    if (vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain!");
    } else {
        mLastSwapchain = mSwapchain;
    }
    std::cout << "swapchain is good\n";

    ///Populate swapchainVar struct
    vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, nullptr);
    mImages.resize(imageCount);
    vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, mImages.data());

    mFormat = surfaceFormat.format;

    createImageViews(mImageViews, mFormat, mDevice);

    createSyncObjects();

    createFrameBuffers();
};

void NEDisplay::createSurface() {
    if (glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!");
    }
};

void NEDisplay::chooseSwapExtent() {

    if (mCapabilities.currentExtent.width != UINT32_MAX) {
        std::cout << mCapabilities.currentExtent.width << "X" << mCapabilities.currentExtent.height << "\n";
        mExtent = mCapabilities.currentExtent;
    } else {

        glfwGetFramebufferSize(mWindow, &mWidth, &mHeight);
        std::cout << "width: " << mWidth << " height: " << mHeight << std::endl;

        VkExtent2D actualExtent = {
                static_cast<uint32_t>(mWidth),
                static_cast<uint32_t>(mHeight)
        };

        actualExtent.width = std::max(mCapabilities.minImageExtent.width,
                                      std::min(mCapabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(mCapabilities.minImageExtent.height,
                                       std::min(mCapabilities.maxImageExtent.height, actualExtent.height));
        std::cout << actualExtent.width << "X" << actualExtent.height << "\n";
        mExtent = actualExtent;
    }
};

bool NEDisplay::shouldExit() {
    if (!glfwWindowShouldClose(mWindow)) {
        glfwPollEvents();
        return false;
    } else {
        return true;
    }
};

void NEDisplay::createFrameBuffers() {
    mFramebuffers.resize(mImageViews.size());
    for (size_t i = 0; i < mImageViews.size(); i++) {
        VkImageView attachments[] = {
                mImageViews[i]
        };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = mRenderpass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = mExtent.width;
        framebufferInfo.height = mExtent.height;
        framebufferInfo.layers = 1;
        if (vkCreateFramebuffer(mDevice, &framebufferInfo, nullptr, &mFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}


std::vector<VkImage> NEDisplay::images() {return mImages;}
std::vector<VkFence> NEDisplay::inFlightFences() {return mInFlightFences;}
uint32_t NEDisplay::imageIndex() {return mImageIndex;}
VkExtent2D NEDisplay::extent() {return mExtent;}
VkSurfaceKHR NEDisplay::surface() {return mSurface;}
uint8_t NEDisplay::framebufferSize() {return (MAX_FRAMES_IN_FLIGHT + 1);}
std::vector<VkFramebuffer> NEDisplay::frameBuffers() {return mFramebuffers;}
std::vector<VkImageView> NEDisplay::imageViews() {return mImageViews;}
std::vector<VkSemaphore> NEDisplay::imageAvailableSemaphores() {return mImageAvailableSemaphores;}
std::vector<VkSemaphore> NEDisplay::renderFinishedSemaphores() {return mRenderFinishedSemaphores;}