//
// Created by nineball on 6/14/21.
//

#ifndef NINEENGINE_NEDISPLAY_H
#define NINEENGINE_NEDISPLAY_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include "NEShared.h"
#include "NEDevice.h"
class NEDevice;

class NEDisplay {
public:
    NEDisplay(int width, int height, std::string title, bool resizable, VkInstance instance);
    ~NEDisplay();

   NEDisplay(const NEDisplay&) = delete;
   NEDisplay& operator = (const NEDisplay&) = delete;

    bool shouldExit();
    void startFrame();
    void endFrame();
    void recreateSwapchain(NEDevice *device);

public:
    uint32_t imageIndex();
    int currentFrame();
    std::vector<VkSemaphore> imageAvailableSemaphores();
    std::vector<VkSemaphore> renderFinishedSemaphores();
    std::vector<VkFence> inFlightFences();
    VkSurfaceKHR surface();
    uint8_t framebufferSize();
    VkExtent2D extent();
    std::vector<VkFramebuffer> frameBuffers();
    std::vector<VkImageView> imageViews();
    std::vector<VkImage> images();

    void createFrameBuffers();

private:
    void createWindow();
    void createSurface();
    void chooseSwapExtent();
    void createSyncObjects();

private:
    uint8_t MAX_FRAMES_IN_FLIGHT = 2;
    int mCurrentFrame = 0;
    bool mFramebufferResized = false;
    uint32_t mImageIndex;

private:
    VkPhysicalDevice mPhysicalDevice;
    QueueFamilyIndices mQueueFamilys;
    VkDevice mDevice;
    VkQueue mPresentQueue;
    VkRenderPass mRenderpass;
    VkInstance mInstance;

private:
    VkSurfaceKHR mSurface;
    VkFormat mFormat;
    VkExtent2D mExtent;
    VkSurfaceCapabilitiesKHR mCapabilities;
    GLFWwindow* mWindow;

    VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;
    VkSwapchainKHR mLastSwapchain = VK_NULL_HANDLE;

    std::vector<VkImage> mImages;
    std::vector<VkImageView> mImageViews;
    std::vector<VkFramebuffer> mFramebuffers;

private:
    std::vector<VkFence> mInFlightFences;
    std::vector<VkFence> mImagesInFlight;
    std::vector<VkSemaphore> mImageAvailableSemaphores;
    std::vector<VkSemaphore> mRenderFinishedSemaphores;

    SwapChainSupportDetails mSupportDetails;


    std::vector<VkCommandBuffer> commandBuffers;

private:
    int mWidth;
    int mHeight;
    bool mResizable;
    std::string mTitle;
};


#endif //NINEENGINE_NEDISPLAY_H
