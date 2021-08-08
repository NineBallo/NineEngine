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
#include <memory>
#include <optional>

#include "../../Common/ImGuiHelpers.h"
class NEGUI;

#include "Types.h"
#include "Device.h"
#define MAX_FRAMES 3

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
    void createSwapchain(const std::shared_ptr<NEDevice>& device, VkPresentModeKHR presentMode);

    void finishInit();

    void createFramebuffers(VkExtent2D FBSize, VkFormat format, uint32_t flags, bool MSAA);
    void createSyncStructures(FrameData &frame);
    void createDescriptors();
    void initImGUI();

    void addTexture(Texture& tex, uint32_t dstIdx);

    //Render methods
    VkCommandBuffer startFrame();
    void endFrame();
    bool shouldExit();

    void toggleFullscreen();
    void resizeFrameBuffer(VkExtent2D FBSize, uint32_t flags);

public:
    VkSurfaceKHR surface();
    VkFormat format();
    VkExtent2D extent();
    GLFWwindow* window();
    FrameData currentFrame();
    uint32_t frameIndex();
    VkRenderPass texturePass();
    VkRenderPass swapchainPass();
    float aspect();
    std::shared_ptr<NEDevice> device();
    VkDescriptorPool guiDescriptorPool();

private:
    void recreateSwapchain();
    void populateFrameData();
    void createWindow(VkExtent2D extent, const std::string& title, bool resizable);
    VkCommandBuffer createCommandBuffer(VkCommandPool commandPool);

    void setupBindRenderpass(VkCommandBuffer cmd, uint32_t flags, VkExtent2D extent);
    void setPipelineDynamics(VkCommandBuffer cmd, VkExtent2D extent);

    void createImage(VkExtent2D extent, uint32_t mipLevels, AllocatedImage &image, VkImageView &imageView, VkImageAspectFlagBits aspect, VkFormat format, VkImageUsageFlagBits usage, VkSampleCountFlagBits sampleCount);


    //Swapchain variables
    VkSwapchainKHR mSwapchain {VK_NULL_HANDLE};
    VkFormat mFormat;

    std::unordered_map<uint32_t, RenderPassInfo> mRenderPasses;

    //Window variables
    VkExtent2D mExtent;
    GLFWwindow* mWindow = nullptr;
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    std::string mTitle;

private:
    //Frame "Data"
    std::array<FrameData, MAX_FRAMES> mFrames;

    GPUSceneData mSceneData {};
    AllocatedBuffer mSceneParameterBuffer;
    VkPresentModeKHR mPresentMode;

    //Stuff
    VkSampler mSimpleSampler;
    VkDescriptorPool mGuiDescriptorPool;
    //Sync
    uint32_t mSwapchainImageIndex {0};
    uint32_t mCurrentFrame {0};
    uint32_t mFrameCount {0};
    uint32_t mSwapchainImageCount {0};
    float mAspect {800.f/600.f};
    bool mFullScreen {false};


private:
    //Mainly destruction variables
    std::shared_ptr<NEDevice> mDevice;
    std::unique_ptr<NEGUI> mGUI;
    VkInstance mInstance = VK_NULL_HANDLE;
    DeletionQueue mDeletionQueue;
    DeletionQueue mSwapchainQueue;
};



#endif //NINEENGINE_DISPLAY_H
