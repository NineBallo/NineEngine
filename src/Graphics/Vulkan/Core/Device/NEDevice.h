//
// Created by nineball on 5/29/21.
//

#ifndef NINEENGINE_NEDEVICE_H
#define NINEENGINE_NEDEVICE_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>
#include "vector"
#include "optional"
#include "NEWindow.h"
#include "NEPipeline.h"
#include "NERenderpass.h"
#include "../NEInstance.h"

class NEWindow;
class NEPipeline;

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

namespace NEVK {
    class NEDevice {
    public:
        NEDevice(std::shared_ptr<NEInstance>& instance);
        ~NEDevice();
        operator VkDevice() const { return device; }


    public:
        void createWindow();

        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        VkBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);

        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        SwapChainSupportDetails getSwapChainDetails();
        VkPhysicalDevice getPhysicalDevice();
        NEWindow* getWindow();
        QueueFamilyIndices getIndices();
        NERenderpass* getRenderpass();
        NEPipeline* getPipeline();

        void setFormat(VkFormat format);

    private:
     //   void createBuffers();
     //   void destroyBuffers();
        void pickPhysicalDevice();
        void createLogicalDevice();
        bool isDeviceSuitable(VkPhysicalDevice candidate);
        bool checkDeviceExtensionSupport(VkPhysicalDevice candidate);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice candidate);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice candidate);


    private:
        SwapChainSupportDetails swapChainSupportDetails;


        bool enableValidationLayers = true;
        std::shared_ptr<NEWindow> window;
        std::shared_ptr<NERenderpass> renderpass;
        std::shared_ptr<NEPipeline> pipeline;
        std::shared_ptr<NEInstance>& instance;

        VkFormat format;
        VkDevice device = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkCommandPool commandPool = VK_NULL_HANDLE;

        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;

        QueueFamilyIndices indices;

        std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    };
}



#endif //NINEENGINE_NEDEVICE_H
