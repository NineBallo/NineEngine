//
// Created by nineball on 6/14/21.
//

#ifndef NINEENGINE_NEDISPLAY_H
#define NINEENGINE_NEDISPLAY_H
#include "string"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "vector"
#include "NEShared.h"

class NEDisplay {
public:
    NEDisplay(int _width, int _height, std::string _title, bool _resizable, uint32_t _deviceIndex);
    ~NEDisplay();

    bool shouldExit();
    void startFrame();
    void endFrame();
    void recreateSwapchain();

public:
    uint32_t getImageIndex();
    int getFrame();
    std::vector<VkSemaphore> getImageAvailableSemaphores();
    std::vector<VkSemaphore> getRenderFinishedSemaphores();
    std::vector<VkFence> getInFlightFences();
    VkSurfaceKHR getSurface();
    uint8_t getFramebufferSize();
    VkExtent2D getExtent();
    std::vector<VkFramebuffer> getFrameBuffers();
    std::vector<VkImageView> getImageViews();
    std::vector<VkImage> getImages();

    void setFrameBuffers(std::vector<VkFramebuffer>);
private:
    void createWindow();
    void createSurface();
    void chooseSwapExtent();
    void createSyncObjects();

private:
    uint8_t MAX_FRAMES_IN_FLIGHT = 2;
    int currentFrame = 0;
    bool framebufferResized = false;
    uint32_t imageIndex;

private:
    uint32_t deviceIndex;
    VkDevice device;
    VkQueue presentQueue;

private:
    VkSurfaceKHR surface;
    VkFormat format;
    VkExtent2D extent;
    VkSurfaceCapabilitiesKHR capabilities;
    GLFWwindow* window;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;

private:
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

    SwapChainSupportDetails supportDetails;

private:
    int width;
    int height;
    bool resizable;
    std::string title;
};


#endif //NINEENGINE_NEDISPLAY_H
