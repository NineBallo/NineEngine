//
// Created by nineball on 5/29/21.
//

#ifndef NINEENGINE_NEDEVICE_H
#define NINEENGINE_NEDEVICE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <memory>
#include <vector>
#include <optional>
#include "NEShared.h"

#include "NEDisplay.h"
#include "NEPipeline.h"
#include "NERenderpass.h"
#include "NEInstance.h"


class NEDisplay;
class NEPipeline;
struct Vertex;
struct SwapChainSupportDetails;

class NEDevice {
public:

    NEDevice(VkInstance instance, VkSurfaceKHR surface, short frameBufferSize);
    ~NEDevice();

    operator VkDevice() const { if(mDevice) return mDevice; else return VK_NULL_HANDLE;}

    NEDevice(const NEDevice&) = delete;
    NEDevice& operator = (const NEDevice&) = delete;

public:
    void createBuffers(short frameBufferSize);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    VkBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void submitCommandBuffer(VkCommandBuffer commandBuffer);

    void createVertexBuffers(VkDeviceMemory &vertexBufferMemory, VkBuffer &vertexBuffer, const std::vector<Vertex>& vertices);
    void createIndexBuffers(VkDeviceMemory &indexBufferMemory, VkBuffer &indexBuffer, std::vector<uint16_t> index);

    std::vector<VkCommandBuffer> createCommandBuffer(uint32_t count);

    void createTextureImage(VkImage &textureImage, VkDeviceMemory &textureImageMemory, const std::string& texPath);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
            VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);
    void createTextureImageView(VkImage &image, VkImageView &imageView);

    void pickPhysicalDevice(VkInstance pT);
    void createLogicalDevice();

    VkRenderPass renderpass();
    NEPipeline* pipeline();
    VkQueue presentQueue();
    VkQueue graphicsQueue();
    SwapChainSupportDetails supportDetails();
    QueueFamilyIndices queueFamilys();
    VkDescriptorPool descriptorPool();
    VkPhysicalDevice GPU();
    VkCommandPool commandPool();

private:
    void createCommandPool();
    void createDescriptorPool(short size);
    void createUniformBuffers();

private:
    VkDevice mDevice = VK_NULL_HANDLE;
    VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
    VkCommandPool mCommandPool = VK_NULL_HANDLE;
    VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
    VkQueue mGraphicsQueue = VK_NULL_HANDLE;
    VkQueue mPresentQueue = VK_NULL_HANDLE;

private:
    VkImage mDefaultImage = VK_NULL_HANDLE;
    VkImageView mDefaultImageView = VK_NULL_HANDLE;
    VkDeviceMemory mDefaultImageMemory = VK_NULL_HANDLE;

private:
    VkImageView mTextureImageView = VK_NULL_HANDLE;
    std::vector<VkBuffer> mUniformBuffers;
    std::vector<VkDeviceMemory> mUniformBuffersMemory;

    QueueFamilyIndices mQueueFamilys;
    SwapChainSupportDetails mSwapChainSupportDetails;

private:
    float queuePriority = 1.0f;
    bool enableValidationLayers = true;

    NEDisplay *RootDisplay;

    std::shared_ptr<NERenderpass> mRenderpass;
    std::shared_ptr<NEPipeline> mPipeline;

    const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
};




#endif //NINEENGINE_NEDEVICE_H
