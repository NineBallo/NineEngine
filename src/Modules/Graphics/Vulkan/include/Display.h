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
    //Constructor && Destructor
    NEDisplay(const displayCreateInfo& createInfo);
    ~NEDisplay();

    //Operator overloads...
    NEDisplay(const NEDisplay&) = delete;
    NEDisplay& operator = (const NEDisplay&) = delete;

    //Initialization methods (Necessary that all of these are done before the render methods are called)
    void startInit(const std::shared_ptr<NEDevice>& device);
    void finishInit();
    void createSurface(VkInstance);

    //Swapchain/Framebuffer Recreation
    void resizeFrameBuffer(VkExtent2D FBSize, uint32_t flags);

    //If imgui is enabled/requested...
    void initImGUI();
    void createDescriptors();

public:

//Runtime external methods
    //Render Methods
    VkCommandBuffer startFrame();
    void endFrame();

    //Upload tex to gpu TODO move this to device, this is not a per display resource
    TextureID loadTexture(std::string filepath, std::string name = nullptr);

    //Window methods
    bool shouldExit();
    void toggleFullscreen();

    void drawFrame();
    void presentFrame();

private:
    //Internal init abstraction
    void createWindow(VkExtent2D extent, const std::string& title, bool resizable);
    void createSwapchain(VkPresentModeKHR presentMode);
    void createSyncStructures(FrameData &frame);
    void createFramebuffers(VkExtent2D FBSize, VkFormat format, uint32_t flags, bool MSAA);
    void populateFrameData();
    void cleanupFramebuffer(uint32_t FBflags);

    //Internal Swapchain/Framebuffer Recreation
    void recreateSwapchain();

    //Rendering Abstraction
    void setupBindRenderpass(VkCommandBuffer cmd, uint32_t flags, VkExtent2D extent);
    void setPipelineDynamics(VkCommandBuffer cmd, VkExtent2D extent);

    //TODO might wanna move create cmd buf to device and evaluate createImage's current position.
    void createImage(VkExtent2D extent, uint32_t mipLevels, AllocatedImage &image, VkImageView &imageView, VkImageAspectFlagBits aspect, VkFormat format, VkImageUsageFlagBits usage, VkSampleCountFlagBits sampleCount);
    VkCommandBuffer createCommandBuffer(VkCommandPool commandPool);

public:
    //Getters
    VkSurfaceKHR surface();
    VkFormat format();
    VkExtent2D extent();
    GLFWwindow* window();
    FrameData currentFrame();
    uint32_t frameIndex();
    VkRenderPass texturePass();
    VkRenderPass swapchainPass();
    std::shared_ptr<NEDevice> device();
    VkDescriptorPool guiDescriptorPool();

private:
    //Window variables
    VkExtent2D mExtent;
    GLFWwindow* mWindow = nullptr;
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    std::string mTitle;
    bool mFullScreen {false};

    //Swapchain variables
    VkSwapchainKHR mSwapchain {VK_NULL_HANDLE};
    VkSwapchainKHR mOldSwapchain {VK_NULL_HANDLE};
    VkFormat mFormat;
    VkPresentModeKHR mPresentMode;

    //Per frame data
    std::array<FrameData, MAX_FRAMES> mFrames;
    GPUSceneData mSceneData {};
    AllocatedBuffer mSceneParameterBuffer;
    std::unordered_map<uint32_t, FrameBufferInfo> mFrameBufferList;

    //Sync
    uint32_t mSwapchainImageIndex {0};
    uint32_t mCurrentFrame {0};
    uint32_t mFrameCount {0};
    uint32_t mSwapchainImageCount {0};

    //Stuff
    VkSampler mSimpleSampler {VK_NULL_HANDLE};
    VkDescriptorPool mGuiDescriptorPool {VK_NULL_HANDLE};

    //Ref's to "friend" classes. (not friend in the C++ sense however)
    std::shared_ptr<NEDevice> mDevice;
    std::unique_ptr<NEGUI> mGUI;

    //For Draw
    Entity mCameraEntity = 0;

    //Textures...
    std::array<uint32_t, MAX_TEXTURES> mTextureToBinding;
    std::array<TextureID, MAX_TEXTURES> mBindingsToTexture;
    uint32_t mTextureCount;

    SubscribeData SData;
    ECS &mECS;

//Variables needed for cleanup and destruction
    VkInstance mInstance {VK_NULL_HANDLE};
    //This deletion queue gets flushed on display destruction
    DeletionQueue mDeletionQueue;
    //This deletion queue gets flushed on swapchain recreation
    DeletionQueue mSwapchainQueue;
};



#endif //NINEENGINE_DISPLAY_H
