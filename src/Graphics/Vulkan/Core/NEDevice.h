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


class NEWindow;
class NEPipeline;
struct Vertex;
struct SwapChainSupportDetails;


struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class NEDevice {
public:
    NEDevice(std::shared_ptr<NEInstance>& instance);
    ~NEDevice();

    operator VkDevice() const { if(device) return device; else return VK_NULL_HANDLE;}

public:
    void createBuffers();
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    VkBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void submitCommandBuffer(VkCommandBuffer commandBuffer);

    void createVertexBuffers(VkDeviceMemory &vertexBufferMemory, VkBuffer &vertexBuffer, const std::vector<Vertex>& vertices);
    void createIndexBuffers(VkDeviceMemory &indexBufferMemory, VkBuffer &indexBuffer, std::vector<uint16_t> index);

    void createTextureImage(VkImage &textureImage, VkDeviceMemory &textureImageMemory, const std::string& texPath);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
            VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);
    void createTextureImageView(VkImage &image, VkImageView &imageView);

    void pickPhysicalDevice();
    void createLogicalDevice();

    NERenderpass* getRenderpass();
    NEPipeline* getPipeline();
    VkQueue getPresentQueue();
    SwapChainSupportDetails getSupportDetails();
    QueueFamilyIndices getQueueFamilys();
    VkDescriptorPool getDescriptorPool();
    VkPhysicalDevice getGPU();
    VkCommandPool getCommandPool();
    VkDevice getDevice() {return device;}
private:
    void createCommandPool();
    void createDescriptorPool(short size);
    void createUniformBuffers();

private:
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

private:
    VkImage defaultImage = VK_NULL_HANDLE;
    VkImageView defaultImageView = VK_NULL_HANDLE;
    VkDeviceMemory defaultImageMemory = VK_NULL_HANDLE;

private:
    VkImageView textureImageView = VK_NULL_HANDLE;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    QueueFamilyIndices queueFamilys;
    SwapChainSupportDetails swapChainSupportDetails;

private:
    bool enableValidationLayers = true;
    NEDisplay* display;
    std::shared_ptr<NERenderpass> renderpass;
    std::shared_ptr<NEPipeline> pipeline;

    const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

};




#endif //NINEENGINE_NEDEVICE_H
