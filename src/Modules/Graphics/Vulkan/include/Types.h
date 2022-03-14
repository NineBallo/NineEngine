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

#define NE_FLAG_TEXTURE_BIT 1 << 0 //     1
#define NE_FLAG_COLOR_BIT   1 << 1 //    10
#define NE_FLAG_SHADOW_BIT  1 << 2 //   100
#define NE_FLAG_BINDING_BIT 1 << 3 //  1000

///TODO move to engine settings
#define NE_FLAG_MSAA8x_BIT 1 << 4 //   1000
#define NE_FLAG_MSAA4x_BIT 1 << 5 //  10000
#define NE_FLAG_MSAA2x_BIT 1 << 6 // 100000

#define NE_RENDERMODE_TOSWAPCHAIN_BIT 1 << 0 //   1
#define NE_RENDERMODE_TOTEXTURE_BIT   1 << 1 //  10
#define NE_RENDERMODE_TOSHADOWMAP_BIT 1 << 2 // 100


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
    VkDescriptorSet mTextureSet = {};
};

struct DirectionalLight {
    glm::vec3 Color;
    float AmbientIntensity;
    glm::vec3 Direction;
    float DiffuseIntensity;
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
    glm::vec4 ambientColor{0.018f, 0.018f, 0.011f, 1.f};
    glm::vec4 sunlightDirection{0.f, 100.f, 0.f, 1.f}; //w for sun power
    glm::vec4 sunlightColor{0.286f, 0.262f, 0.250f, 1.f};
    glm::mat4 lightMatrix{1.f};
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

    VkDescriptorSet mDirectionalShadowDescriptor;
};

struct FrameBufferInfo {
    std::vector<VkFramebuffer> frameBuffers;
    //Keyed to the SC idx
    std::vector<VkImageView> colorImageViews, depthImageViews, resolveImageViews;
    std::vector<AllocatedImage> colorImages, depthImages, resolveImages;
};




#endif //NINEENGINE_TYPES_H
