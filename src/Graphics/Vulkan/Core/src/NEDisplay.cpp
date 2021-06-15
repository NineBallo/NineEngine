//
// Created by nineball on 6/14/21.
//

#include "../NEDisplay.h"
#include "../NEShared.h"
#include "iostream"
#include "../../Helper/Display.h"
#include "../../Helper/Device.h"
#include <utility>

NEDisplay::NEDisplay(int _width, int _height, std::string _title, bool _resizable, uint32_t _deviceIndex) :
width(_width), height(_height), title(_title), resizable(_resizable), deviceIndex(_deviceIndex) {
    createWindow();
    createSurface();
}

NEDisplay::~NEDisplay() {};

void NEDisplay::createWindow() {
    glfwInit();

    ///Dont create an opengl context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (resizable) {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    } else {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    }

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
 //  glfwSetWindowUserPointer(window, vulkan);
 //  glfwSetFramebufferSizeCallback(window, VKBareAPI::Window::framebufferResizeCallback);
}

void NEDisplay::startFrame() {
    ///Wait for the fence representing the image we want to render to finish submitting to the gpu before submitting another command buffer

    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    ///Get next position/index in swap buffer.
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
                                            VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
//            recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    ///If image is being presented then wait for it to finish, if fence does not yet exist then skip.
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }

    ///Assign the respective fence to a specific image.
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    //       updateUniformBuffer(imageIndex);
};

void NEDisplay::endFrame() {
    ///Reset fence, prep it for display.
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

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
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
};

int NEDisplay::getFrame() {
    return currentFrame;
};

void NEDisplay::createSyncObjects() {
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
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                              &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                              &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

            throw std::runtime_error("Failed to create semaphores for a frame!");
        }
    }
};

void NEDisplay::recreateSwapchain() {
    device = vkContext.devices[deviceIndex];

    supportDetails = querySwapChainSupport(vkContext.devices[deviceIndex].getGPU(), surface);


    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(supportDetails.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(supportDetails.presentModes);
    capabilities = supportDetails.capabilities;
    chooseSwapExtent();


    QueueFamilyIndices queueFamilys = vkContext.devices[deviceIndex].getQueueFamilys();


    ///How many images are in the swapchain
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
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


    uint32_t queueFamilyIndices[] = {queueFamilys.graphicsFamily.value(), queueFamilys.presentFamily.value()};
    if (queueFamilys.graphicsFamily != queueFamilys.presentFamily) {
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

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain!");
    }
    std::cout << "swapchain is good\n";

    ///Populate swapchainVar struct
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data());

    format = surfaceFormat.format;

    createImageViews(imageViews, format, device);

    createSyncObjects();
};

void NEDisplay::createSurface() {
    if (glfwCreateWindowSurface(vkContext.instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!");
    }
};

void NEDisplay::chooseSwapExtent() {

    if (false){//capabilities.currentExtent.width != UINT32_MAX) {
        std::cout << capabilities.currentExtent.width << "X" << capabilities.currentExtent.height << "\n";
        extent = capabilities.currentExtent;
    } else {

        glfwGetFramebufferSize(window, &width, &height);
        std::cout << "width: " << width << " height: " << height << std::endl;

        VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
        };

        actualExtent.width = std::max(capabilities.minImageExtent.width,
                                      std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height,
                                       std::min(capabilities.maxImageExtent.height, actualExtent.height));
        std::cout << actualExtent.width << "X" << actualExtent.height << "\n";
        extent = actualExtent;
    }
};

bool NEDisplay::shouldExit() {
    if (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        return false;
    } else {
        return true;
    }
};

std::vector<VkImage> NEDisplay::getImages() {return images;}
std::vector<VkFence> NEDisplay::getInFlightFences() {return inFlightFences;}
uint32_t NEDisplay::getImageIndex() {return imageIndex;}
VkExtent2D NEDisplay::getExtent() {return extent;}
VkSurfaceKHR NEDisplay::getSurface() {return surface;}
uint8_t NEDisplay::getFramebufferSize() {return (MAX_FRAMES_IN_FLIGHT + 1);}
std::vector<VkFramebuffer> NEDisplay::getFrameBuffers() {return framebuffers;}
std::vector<VkImageView> NEDisplay::getImageViews() {return imageViews;}
std::vector<VkSemaphore> NEDisplay::getImageAvailableSemaphores() {return imageAvailableSemaphores;}
std::vector<VkSemaphore> NEDisplay::getRenderFinishedSemaphores() {return renderFinishedSemaphores;}
void NEDisplay::setFrameBuffers(std::vector<VkFramebuffer> _framebuffers) { framebuffers = std::move(_framebuffers);}