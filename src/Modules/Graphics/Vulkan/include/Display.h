//
// Created by nineball on 7/6/21.
//

#ifndef NINEENGINE_DISPLAY_H
#define NINEENGINE_DISPLAY_H
#include "chrono"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <deque>
#include <functional>

#include "memory"
#include "Types.h"
#include "Device.h"
#define MAX_FRAMES 2

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
    void createSyncStructures(FrameData &frame);

    void initImGUI();

    void createDescriptors();



    VkCommandBuffer createCommandBuffer(VkCommandPool commandPool);

    //Render methods
    VkCommandBuffer startFrame();
    void endFrame();
    bool shouldExit();

public:
    VkSurfaceKHR surface();
    VkFormat format();
    VkExtent2D extent();
    GLFWwindow* window();
    FrameData currentFrame();
    uint32_t frameIndex();

private:
    void populateFrameData();
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

    //Window variables
    VkExtent2D mExtent;
    GLFWwindow* mWindow = nullptr;
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    std::string mTitle;

private:
    //Frame "Data"
    std::array<FrameData, MAX_FRAMES> mFrames;
    std::chrono::time_point<std::chrono::steady_clock> currentTick;
    std::chrono::time_point<std::chrono::steady_clock> lastTick;

    GPUSceneData mSceneData {};
    AllocatedBuffer mSceneParameterBuffer;


    //Sync
    uint32_t mSwapchainImageIndex = 0;
    uint8_t mCurrentFrame = 0;
    uint32_t mFrameCount = 0;

private:
    //Mainly destruction variables
    std::shared_ptr<NEDevice> mDevice;
    VkInstance mInstance = VK_NULL_HANDLE;
    DeletionQueue mDeletionQueue;
};



#endif //NINEENGINE_DISPLAY_H
