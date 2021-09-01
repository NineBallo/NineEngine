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
#include "Mesh.h"
#include "ECS.h"
#include "Engine.h"

#define NE_RENDERMODE_TOSWAPCHAIN_BIT 1 << 0
#define NE_RENDERMODE_TOTEXTURE_BIT   1 << 1

#define NE_FLAG_MSAA8x_BIT 1 << 2
#define NE_FLAG_MSAA4x_BIT 1 << 3
#define NE_FLAG_MSAA2x_BIT 1 << 4

class NEDevice {
public:
    //Constructors && Destructors
    NEDevice();
    ~NEDevice();

    //Operator overloads
    operator VkDevice() const { if(mDevice) return mDevice; else return VK_NULL_HANDLE;}

    //Init methods, required for rendering
    vkb::PhysicalDevice init_PhysicalDevice(VkSurfaceKHR surface, vkb::Instance &vkb_inst);
    void init_LogicalDevice(vkb::PhysicalDevice &physicalDevice);
    void init_Allocator(VkInstance instance);
    void init_descriptors();
    void init_upload_context();


public:

//Helper methods primarily for the display class
    //Gets the requested renderpass if available, if not it creates it then returns the renderpass
    VkRenderPass getRenderPass(uint32_t flags, VkFormat format = VK_FORMAT_UNDEFINED);

    //Gets the requested pipeline data if available, if not it creates it then returns the pipelinedata
    std::pair<VkPipeline, VkPipelineLayout> getPipeline(uint32_t rendermode, uint32_t features);

    //Textures
    TextureID loadTexture(std::string pathToImage);

    TextureID getTextureID(std::string name);
    Texture& getTexture(TextureID id);

    void deleteTexture(TextureID id);

    //Meshes
    MeshGroupID createMesh(const std::string& filepath, const std::string& meshName = nullptr);
    void deleteMesh(MeshGroupID id);

    ///What is oatmeal?
    ///TODO move texture and mesh code over; Then it should be good to go assuming no bugs in the ecs.


    //Buffers
    template<typename T>
    void uploadToBuffer(std::vector<T>& data, AllocatedBuffer& buffer, size_t size);
    AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    size_t padUniformBufferSize(size_t originalSize);

    //Upload stuff
    void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

    //Resource creation
    VkCommandPool createCommandPool(uint32_t queueFamily);
    VkSampler createSampler(uint32_t mipLevels = 1);
    VkDescriptorSet createDescriptorSet(VkDescriptorSetLayout layout, void* pNext = nullptr);
    std::pair<VkPipeline, VkPipelineLayout> createPipeline(VkRenderPass renderPass, std::vector<uint32_t> vertexShaderSpirv,
                                                           std::vector<uint32_t> fragmentShaderSpirv, uint32_t flags);


    //Image manipulation
    void transitionImageLayout(VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout, uint32_t mipLevel);
    void copyBufferToImage(VkBuffer buffer, VkImage image, VkExtent2D extent);
    void generateMipmaps(VkImage image, VkFormat imageFormat, VkExtent2D texSize, uint32_t mipLevels);

private:
//Internal abstraction
    //Loads a texture, but from a file.
    bool loadTextureFromFile(const char* file, Texture& outTex);

    //Descriptor set layout methods
    VkDescriptorSetLayout createDescriptorSetLayout(VkDescriptorSetLayoutCreateFlags flags, VkDescriptorSetLayoutBinding* bindingArray,
                                                    uint8_t arraySize, void* pNext = nullptr);



    //RenderPass creation
    VkRenderPass createRenderpass(VkFormat format, uint32_t flags);

    //Pipeline creation TODO move pipeline creation to the same format as the renderpass creation
    bool loadShaderModule(std::vector<uint32_t> spirv, VkShaderModule &outShaderModule);

    //Device limits
    VkSampleCountFlagBits getMaxSampleCount();
    float getMaxAnisotropy();

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

    VmaAllocator allocator();

    bool bindless();
    VkSampleCountFlagBits sampleCount();

private:
    //Actual Device this will not be exposed, will be index + 1
    VkDevice mDevice = VK_NULL_HANDLE;
    VkPhysicalDevice mGPU = VK_NULL_HANDLE;

    //Property's relating to the device and device limits
    VkPhysicalDeviceProperties mGPUProperties {};
    VkPhysicalDeviceFeatures2 mGPUFeatures {};
    VkPhysicalDeviceVulkan12Features mGPUFeaturesVK12 {};
    VkSampleCountFlagBits mSampleCount {};
    float mMaxAnisotropy {0};
    bool mBindless {};


    //Device queues
    VkQueue mGraphicsQueue {VK_NULL_HANDLE};
    uint32_t mGraphicsQueueFamily {0};

    VkQueue mPresentQueue{VK_NULL_HANDLE};
    uint32_t mPresentQueueFamily {0};


    //Descriptors layouts that will be used later.
    VkDescriptorPool mDescriptorPool;
    VkDescriptorSetLayout mGlobalSetLayout {VK_NULL_HANDLE};
    VkDescriptorSetLayout mObjectSetLayout {VK_NULL_HANDLE};
    VkDescriptorSetLayout mSingleTextureSetLayout {VK_NULL_HANDLE};
    VkDescriptorSetLayout mTextureSetLayout {VK_NULL_HANDLE};


    ///Memory allocator
    VmaAllocator mAllocator {nullptr};

    //The context used for one time immediate submit commands
    UploadContext mUploadContext {};


private: ///resource management

//All meshes uploaded to device.
    //Maps for packed array
    std::array<MeshGroupID, MAX_ENTITYS> mPosToMeshID;
    std::array<uint64_t, MAX_ENTITYS> mMeshIDToPos;
    //Packed array of the actual data
    std::array<MeshGroup, MAX_ENTITYS> mMeshGroups;
    //Number of meshes allocated on the current device or last idx + 1
    MeshGroupID mMeshCount;



//A texture is the image projected on an object.
    //<path, name>
    std::unordered_map<std::string, std::string> mTexPathToName;
    //<TexPos, name>
    std::array<TextureID, MAX_TEXTURES> mTexIDToName;
    //maps to maintain packed array indexes
    std::array<uint32_t, MAX_TEXTURES> mTexIDToPos;
    std::array<TextureID, MAX_TEXTURES> mPosToTexID;
    //packed array of texture structures
    std::array<Texture, MAX_TEXTURES> mTextures;
    //Number of textures currently allocated
    uint32_t mTexCount;



//A material is a struct of data pertaining to that material. Rendermode, static-color, etc...
//   std::array<uint32_t, MAX_MATERIALS> mMatIDToPos;
//   std::array<MaterialID, MAX_MATERIALS> mPosToMatID;
//   std::array<Material, MAX_MATERIALS> mMaterials;
//   uint32_t mMatCount;

//List of all allocated RenderPasses, keyed by the RenderPass flags.
    std::unordered_map<uint32_t, uint32_t> mRenderModeToPos;
    std::array<VkRenderPass, MAX_RENDERPASSES> mRenderPassList;
    uint32_t mRenderPassCount;

//List of all pipelines, keyed with the rendermode and features
    std::array<uint32_t, MAX_RENDERPASSES> mRenderModeToPipelineList;
    std::array<std::array<uint32_t, MAX_PIPELINES>, MAX_RENDERPASSES> mPipelineFeaturesToPos;

    //"3D" array, each rendermode has an array of possible pipelines.
    // Each pipeline array has a std::pair with a pipeline and pipelineLayout
    std::array<
                std::array<
                    std::pair<VkPipeline, VkPipelineLayout>,
                    MAX_PIPELINES>,

    MAX_RENDERPASSES> mPipelines;

    Engine &mEngine;

    //Pipeline array size, keyed with rendermode.
    std::array<uint32_t, MAX_RENDERPASSES> mPipelineCount;

    //This will be flushed on destruction
    DeletionQueue mDeletionQueue {};
};

//Templated functions cannot go in a cpp file, so they're chilling here.
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
