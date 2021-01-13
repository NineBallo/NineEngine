//
// Created by nineball on 1/8/21.
//

#ifndef NINEENGINE_VKGLOBALPOOL_H
#define NINEENGINE_VKGLOBALPOOL_H
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <set>
#include <vector>

///Singleton to help handle the sheer amount of shared variables
///Will only be created if a vulkan instance is created

class vkGlobalPool{


    //SINGLETON-CLASS-CREATION-START---------------------------------------------//

public:
    vkGlobalPool(const vkGlobalPool&) = delete;

    static vkGlobalPool& Get(){
        return s_Instance;
    }

private:
    const int MAX_FRAMES_IN_FLIGHT = 2;

    vkGlobalPool() : renderFinishedSemaphores(MAX_FRAMES_IN_FLIGHT), imageAvailableSemaphores(MAX_FRAMES_IN_FLIGHT),
    inFlightFences(MAX_FRAMES_IN_FLIGHT) {};

    static vkGlobalPool s_Instance;

    //SINGLETON-CLASS-CREATION-END-----------------------------------------------//




    //VARIABLES-START------------------------------------------------------------//

public:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value();
        }
    };

public:
    const VkSurfaceKHR getVkSurfaceKhr() const {
        return vkSurfaceKhr;
    }

    void setVkSurfaceKhr(const VkSurfaceKHR _vkSurfaceKhr) {
        vkGlobalPool::vkSurfaceKhr = _vkSurfaceKhr;
    }

    GLFWwindow *getWindow() const {
        return window;
    }

    void setWindow(GLFWwindow *_window) {
        vkGlobalPool::window = _window;
    }

    const VkDevice getVkDevice() const {
        return vkDevice;
    }

    void setVkDevice(const VkDevice _vkDevice) {
        vkGlobalPool::vkDevice = _vkDevice;
    }

    const VkPhysicalDevice getVkPhysicalDevice() const {
        return vkPhysicalDevice;
    }

    void setVkPhysicalDevice(const VkPhysicalDevice _vkPhysicalDevice) {
        vkGlobalPool::vkPhysicalDevice = _vkPhysicalDevice;
    }

    const VkRenderPass getVkRenderPass() const {
        return vkRenderPass;
    }

    void setVkRenderPass(const VkRenderPass _vkRenderPass) {
        vkGlobalPool::vkRenderPass = _vkRenderPass;
    }

    const VkPipeline getVkPipeline() const {
        return vkPipeline;
    }

    void setVkPipeline(const VkPipeline _vkPipeline) {
        vkGlobalPool::vkPipeline = _vkPipeline;
    }

    const VkShaderModule getVertShaderModule() const {
        return vertShaderModule;
    }

    void setVertShaderModule(const VkShaderModule _vertShaderModule) {
        vkGlobalPool::vertShaderModule = _vertShaderModule;
    }

    const VkShaderModule getFragShaderModule() const {
        return fragShaderModule;
    }

    void setFragShaderModule(const VkShaderModule _fragShaderModule) {
        vkGlobalPool::fragShaderModule = _fragShaderModule;
    }

    const VkPipelineLayout getPipelineLayout() const {
        return pipelineLayout;
    }

    void setPipelineLayout(const VkPipelineLayout _pipelineLayout) {
        vkGlobalPool::pipelineLayout = _pipelineLayout;
    }

    const VkInstance getVkInstance() const {
        return vkInstance;
    }

    void setVkInstance(const VkInstance _vkInstance) {
        vkGlobalPool::vkInstance = _vkInstance;
    }

    const VkSwapchainKHR getSwapChain() const {
        return swapChain;
    }

    void setSwapChain(const VkSwapchainKHR _swapChain) {
        vkGlobalPool::swapChain = _swapChain;
    }

    VkFormat getSwapChainImageFormat() const {
        return swapChainImageFormat;
    }

    void setSwapChainImageFormat(VkFormat _swapChainImageFormat) {
        vkGlobalPool::swapChainImageFormat = _swapChainImageFormat;
    }

    const VkExtent2D getSwapChainExtent() const {
        return swapChainExtent;
    }

    void setSwapChainExtent(const VkExtent2D _swapChainExtent) {
        vkGlobalPool::swapChainExtent = _swapChainExtent;
    }

    const std::vector<VkImage>  getSwapChainImages() const {
        return  swapChainImages;
    }

    const std::vector<VkImageView> getSwapChainImageViews() const {
        return swapChainImageViews;
    }

    const std::vector<VkFramebuffer> getSwapChainFrameBuffers() const {
        return swapChainFramebuffers;
    }

    const QueueFamilyIndices getQueueFamilyIndices() const {
        return queueFamilyIndices;
    }

    void setQueueFamilyIndices(const QueueFamilyIndices _queueFamilyIndices) {
        vkGlobalPool::queueFamilyIndices = _queueFamilyIndices;
    }

    const VkCommandPool getCommandPool() const {
        return commandPool;
    }

    void setCommandPool(const VkCommandPool _commandPool) {
        vkGlobalPool::commandPool = _commandPool;
    }
    const VkSemaphore getImageAvailableSemaphore(int i) {
        return imageAvailableSemaphores[i];
    }

    void setImageAvailableSemaphore(const VkSemaphore _imageAvailableSemaphore, int i) {
        vkGlobalPool::imageAvailableSemaphores[i] = _imageAvailableSemaphore;
    }

    const VkSemaphore getRenderFinishedSemaphore(int i) {
        return renderFinishedSemaphores[i];
    }

    void setRenderFinishedSemaphore(const VkSemaphore _renderFinishedSemaphore, int i) {
        vkGlobalPool::renderFinishedSemaphores[i] = _renderFinishedSemaphore;
    }
    const VkQueue getGraphicsQueue() const {
        return graphicsQueue;
    }

    void setGraphicsQueue(const VkQueue _graphicsQueue) {
        vkGlobalPool::graphicsQueue = _graphicsQueue;
    }

    const VkQueue getPresentQueue() const {
        return presentQueue;
    }

    void setPresentQueue(const VkQueue _presentQueue) {
        vkGlobalPool::presentQueue = _presentQueue;
    }

    const int getMaxFramesInFlight() const {
        return MAX_FRAMES_IN_FLIGHT;
    }

    size_t getCurrentFrame() const {
        return currentFrame;
    }

    void setCurrentFrame(size_t currentFrame) {
        vkGlobalPool::currentFrame = currentFrame;
    }

    const VkFence getInFlightFences(int i) const {
        return inFlightFences[i];
    }

    void setInFlightFences(VkFence _inFlightFences, int i) {
        vkGlobalPool::inFlightFences[i] = _inFlightFences;
    }

    const VkFence getImageInFlight(int i) const {
        return imagesInFlight[i];
    }

    void setImageInFlight(VkFence _imageInFlight, int i) {
        vkGlobalPool::imagesInFlight[i] = _imageInFlight;
    }
private:
    QueueFamilyIndices queueFamilyIndices;

    ///Vulkan-Instance-Creation?-Variables--------------///
    VkInstance vkInstance;
    VkSurfaceKHR vkSurfaceKhr;
    GLFWwindow *window;

    ///Device Variables---------------------------------///
    VkDevice vkDevice;
    VkPhysicalDevice vkPhysicalDevice;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    ///Pipeline Variables------------------------------///
    VkPipeline vkPipeline;
    VkPipelineLayout pipelineLayout;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    VkRenderPass vkRenderPass;

    ///Swapbuffer Variables----------------------------///
    VkSwapchainKHR swapChain;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    ///Swapbuffer Vectors-----------------------------///
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    ///Rendering-------------------------------------///
    VkCommandPool commandPool;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
public:
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

private:
    ///Presentation----------------------------------///
    size_t currentFrame = 0;

    //VARIABLES-END--------------------------------------------------------------//




    //FUNCTIONS-START------------------------------------------------------------//

public:
    void findQueueFamilies(VkPhysicalDevice device) {
        ///Get device queue properties
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        ///Find a queue family that supports VK_QUEUE_GRAPHICS_BIT.
        int i = 0;
        for (const auto &queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                queueFamilyIndices.graphicsFamily = i;
            }
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vkGlobalPool::Get().getVkSurfaceKhr(), &presentSupport);
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                queueFamilyIndices.presentFamily = i;
            }
            if (queueFamilyIndices.isComplete()) {
                break;
            }
            i++;
        }
    }

    //FUNCTIONS-END------------------------------------------------------------//




};


#endif //NINEENGINE_VKGLOBALPOOL_H
