//
// Created by nineball on 7/16/21.
//

#ifndef NINEENGINE_DEVICE_H
#define NINEENGINE_DEVICE_H
#include "Types.h"
#include "Common.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include "Common.h"


class NEDevice {
public:
    NEDevice();
    ~NEDevice();
    operator VkDevice() const { if(mDevice) return mDevice; else return VK_NULL_HANDLE;}

    //Mandatory
    vkb::PhysicalDevice init_PhysicalDevice(VkSurfaceKHR surface, vkb::Instance &vkb_inst);
    void init_LogicalDevice(vkb::PhysicalDevice &physicalDevice);
    void init_Allocator(VkInstance instance);
    void init_descriptors();
    void init_upload_context();

    VkRenderPass createDefaultRenderpass(VkFormat format);

public:
    //Helper funtions
    VkDescriptorSet createDescriptorSet(VkDescriptorSetLayout layout, void* pNext = nullptr);
    VkDescriptorSetLayoutBinding createDescriptorSetBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
    VkDescriptorSetLayout createDescriptorSetLayout(VkDescriptorSetLayoutCreateFlags flags, VkDescriptorSetLayoutBinding* bindingArray,
                                                    uint8_t arraySize, void* pNext = nullptr);

    size_t padUniformBufferSize(size_t originalSize);

    std::pair<VkPipeline, VkPipelineLayout> createPipeline(std::vector<uint32_t> vertexShaderSpirv, std::vector<uint32_t> fragmentShaderSpirv, uint32_t flags);
    bool loadShaderModule(std::vector<uint32_t> spirv, VkShaderModule &outShaderModule);

    AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    VkCommandPool createCommandPool(uint32_t queueFamily);

    void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

    VkSampler createSampler();

    template<typename T>
    void uploadToBuffer(std::vector<T>& data, AllocatedBuffer& buffer, size_t size);

public:
    //Getters
    VkDevice device();
    VkPhysicalDevice GPU();

    VkQueue presentQueue();
    uint32_t presentQueueFamily();

    VkQueue graphicsQueue();
    uint32_t graphicsQueueFamily();

    VkDescriptorPool descriptorPool();
    VkDescriptorSetLayout globalSetLayout();
    VkDescriptorSetLayout objectSetLayout();
    VkDescriptorSetLayout singleTextureSetLayout();
    VkDescriptorSetLayout textureSetLayout();

    VkRenderPass defaultRenderpass();
    VmaAllocator allocator();

    bool bindless();
    VkSampleCountFlagBits sampleCount();

private:
    VkSampleCountFlagBits getMaxSampleCount();

private:
    //Actual Device this will not be exposed
    VkDevice mDevice = VK_NULL_HANDLE;
    VkPhysicalDevice mGPU = VK_NULL_HANDLE;

    VkPhysicalDeviceProperties mGPUProperties;
    VkPhysicalDeviceFeatures2 mGPUFeatures;
    VkPhysicalDeviceVulkan12Features mGPUFeaturesVK12;
    VkSampleCountFlagBits mSampleCount;


    VkQueue mGraphicsQueue = VK_NULL_HANDLE;
    uint32_t mGraphicsQueueFamily = 0;

    VkQueue mPresentQueue = VK_NULL_HANDLE;
    uint32_t mPresentQueueFamily = 0;

    VkDescriptorPool mDescriptorPool;
    VkDescriptorSetLayout mGlobalSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout mObjectSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout mSingleTextureSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout mTextureSetLayout = VK_NULL_HANDLE;

    VkRenderPass mDefaultRenderpass = VK_NULL_HANDLE;

    ///Memory allocator
    VmaAllocator mAllocator = nullptr;

private:
    bool mBindless {};

private:
    UploadContext mUploadContext {};
    DeletionQueue mDeletionQueue {};
};

template<typename T>
void NEDevice::uploadToBuffer(std::vector<T> &data, AllocatedBuffer &buffer, size_t size) {

    AllocatedBuffer stagingBuffer = createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                 VMA_MEMORY_USAGE_CPU_ONLY);

    //Copy data
    void* mapped;
    vmaMapMemory(mAllocator, stagingBuffer.mAllocation, &mapped);
    memcpy(mapped, data.data(), size);
    vmaUnmapMemory(mAllocator, stagingBuffer.mAllocation);

    immediateSubmit([=](VkCommandBuffer cmd) {
        VkBufferCopy copy;
        copy.dstOffset = 0;
        copy.srcOffset = 0;
        copy.size = size;
        vkCmdCopyBuffer(cmd, stagingBuffer.mBuffer, buffer.mBuffer, 1, &copy);
    });

    vmaDestroyBuffer(mAllocator, stagingBuffer.mBuffer, stagingBuffer.mAllocation);
}

#endif //NINEENGINE_DEVICE_H
