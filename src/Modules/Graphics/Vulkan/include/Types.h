//
// Created by nineball on 7/7/21.
//

#ifndef NINEENGINE_TYPES_H
#define NINEENGINE_TYPES_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <deque>
#include <functional>
#include <vk_mem_alloc.h>

struct AllocatedBuffer {
    VkBuffer mBuffer;
    VmaAllocation mAllocation;
};

struct AllocatedImage {
    VkImage mImage;
    VmaAllocation mAllocation;
};

struct device {
    VkDevice device;
    VkPhysicalDevice GPU;

    VkQueue graphicsQueue;
    uint32_t graphicsQueueFamily;

    VkQueue presentQueue;
    uint32_t presentQueueFamily;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkRenderPass renderpass;

    ///Memory allocator
    VmaAllocator allocator;
};


struct VkData {
    uint32_t materialIndex;

};

struct material {
    VkPipeline mPipeline;
    VkPipelineLayout mPipelineLayout;
};



#endif //NINEENGINE_TYPES_H
