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
#include <glm/glm.hpp>
#include <glm/ext.hpp>

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

struct Material {
    VkPipeline mPipeline;
    VkPipelineLayout mPipelineLayout;
};

struct Camera {
    //x (left to right), y (up down), z (forwards and back)
    glm::vec3 Pos {0, 0, 0};
    glm::vec3 Angle {0, 0,0};
    float degrees {70.f};
    float aspect {800.f / 600.f};
    float znear {0.1f};
    float zfar {400.0f};
};

struct FrameData {
    VkSemaphore mPresentSemaphore, mRenderSemaphore;
    VkFence mRenderFence;

    VkCommandPool mCommandPool;
    VkCommandBuffer mCommandBuffer;
};


#endif //NINEENGINE_TYPES_H
