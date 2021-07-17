//
// Created by nineball on 7/6/21.
//

#ifndef NINEENGINE_DISPLAY_H
#define NINEENGINE_DISPLAY_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <deque>
#include <functional>

#include "memory"
#include "Types.h"
#include "Device.h"

struct displayCreateInfo {
    VkExtent2D extent = {800, 600};

    bool resizable = false;

    std::string title;
    VkInstance instance = VK_NULL_HANDLE;
    std::shared_ptr<NEDevice> device;
    VkPresentModeKHR presentMode;
};



class NEDisplay {
public:
    NEDisplay(const displayCreateInfo& createInfo);
    ~NEDisplay();

    NEDisplay(const NEDisplay&) = delete;
    NEDisplay& operator = (const NEDisplay&) = delete;


    //Initialization methods (Necessary that all of these are done before the render methods are called)
    void createSurface(VkInstance);
    void createSwapchain(std::shared_ptr<NEDevice> device, VkPresentModeKHR presentMode);

    void createFramebuffers(VkRenderPass renderpass);
    void createSyncStructures();

    VkCommandBuffer createCommandBuffer();

    //Render methods
    VkCommandBuffer startFrame();
    void endFrame();
    bool shouldExit();

public:
    VkSurfaceKHR surface();
    uint16_t frameNumber();
    VkFormat format();
    VkExtent2D extent();
    GLFWwindow* window();

private:
    void createWindow(VkExtent2D extent, const std::string& title, bool resizable);

    //Depth
    VkImageView mDepthImageView;
    AllocatedImage mDepthImage;
    VkFormat mDepthFormat;

    //Swapchain variables
    VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;
    VkFormat mFormat;
    std::vector<VkImage> mImages;
    std::vector<VkImageView> mImageViews;
    std::vector<VkFramebuffer> mFramebuffers;
    VkRenderPass mRenderpass = VK_NULL_HANDLE;

    //Device variables
    VkCommandBuffer mPrimaryCommandBuffer;
    VkCommandPool mPrimaryCommandPool;

    //Sync
    VkSemaphore mPresentSemaphore = VK_NULL_HANDLE, mRenderSemaphore = VK_NULL_HANDLE;
    VkFence mRenderFence = VK_NULL_HANDLE;
    uint16_t mFrameNumber = 0;
    uint32_t mSwapchainImageIndex = 0;

    //Window variables
    VkExtent2D mExtent;
    GLFWwindow* mWindow = nullptr;
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;

private:
    //Mainly destruction variables
    std::shared_ptr<NEDevice> mDevice;
    VkInstance mInstance = VK_NULL_HANDLE;
    DeletionQueue mDeletionQueue;
};



#endif //NINEENGINE_DISPLAY_H
