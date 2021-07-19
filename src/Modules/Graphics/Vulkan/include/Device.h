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

    vkb::PhysicalDevice init_PhysicalDevice(VkSurfaceKHR surface, vkb::Instance &vkb_inst);
    bool init_LogicalDevice(vkb::PhysicalDevice &physicalDevice);
    void init_Allocator(VkInstance instance);


public:
    //Modifiers
    VkRenderPass createDefaultRenderpass(VkFormat format);
    VkCommandPool createCommandPool(uint32_t queueFamily);
    //VkCommandBuffer createCommandBuffer(VkCommandPool commandPool);


public:
    //Getters
    VkDevice device();
    VkPhysicalDevice GPU();

    VkQueue presentQueue();
    VkQueue graphicsQueue();

    uint32_t presentQueueFamily();
    uint32_t graphicsQueueFamily();

    VkRenderPass defaultRenderpass();

    VmaAllocator allocator();

private:
    //Actual Device this will not be exposed
    VkDevice mDevice = VK_NULL_HANDLE;
    VkPhysicalDevice mGPU = VK_NULL_HANDLE;

    VkQueue mGraphicsQueue = VK_NULL_HANDLE;
    uint32_t mGraphicsQueueFamily = 0;

    VkQueue mPresentQueue = VK_NULL_HANDLE;
    uint32_t mPresentQueueFamily = 0;

    VkRenderPass mDefaultRenderpass = VK_NULL_HANDLE;

    ///Memory allocator
    VmaAllocator mAllocator = nullptr;


private:
    DeletionQueue mDeletionQueue {};
};













#endif //NINEENGINE_DEVICE_H
