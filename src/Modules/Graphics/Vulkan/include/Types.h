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

#define MAX_TEXTURES 768

#define NE_SHADER_TEXTURE_BIT 1 << 1 //0001
#define NE_SHADER_COLOR_BIT 1 << 2   //0010
#define NE_FLAG_BINDING_BIT 1 << 3  //0100

using Flags = uint32_t;
using TextureID = uint32_t;

struct AllocatedBuffer {
    VkBuffer mBuffer;
    VmaAllocation mAllocation;
};

struct AllocatedImage {
    VkImage mImage;
    VmaAllocation mAllocation;

};

struct Texture {
    AllocatedImage mImage;
    VkImageView mImageView;
    VkSampler mSampler;
    uint32_t mMipLevels;

    //Optional for legacy
    VkDescriptorSet mTextureSet;
};

struct UploadContext {
    VkFence mUploadFence;
    VkCommandPool mCommandPool;
};

struct GPUCameraData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
};

struct PushData {
    int textureIndex;
    int entityID;
};

struct GPUSceneData {
    //Need to use all vec4's for alignment purposes so not all values are used
    glm::vec4 fogColor; // w is for exponent
    glm::vec4 fogDistances; //x for min, y for max, zw unused.
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection; //w for sun power
    glm::vec4 sunlightColor;
};

struct GPUTransformData {
    glm::mat4 modelMatrix;
};

struct GPUTextureData {
    glm::mat4 modelMatrix;
};

struct GPUObjectData {
    glm::mat4 modelMatrix;
};

struct GPUMaterialData {
    glm::mat4 modelMatrix;
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

    AllocatedBuffer mObjectBuffer;
    VkDescriptorSet mObjectDescriptor;

    AllocatedBuffer mCameraBuffer;
    VkDescriptorSet mGlobalDescriptor;

    VkDescriptorSet mTextureDescriptor;
};

struct FrameBufferInfo {
    std::vector<VkFramebuffer> frameBuffers;
    //Keyed to the SC idx
    std::vector<VkImageView> colorImageViews, depthImageViews, resolveImageViews;
    std::vector<AllocatedImage> colorImages, depthImages, resolveImages;
};




#endif //NINEENGINE_TYPES_H
