//
// Created by nineball on 7/16/21.
//

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
#include <iostream>
#include <fstream>
#include <utility>
#include <Pipeline.h>
#include "ECS.h"
#include "Device.h"
#include "Initializers.h"
#include "Mesh.h"
#include "../shaders/Shaders.h"

NEDevice::NEDevice() {

}

NEDevice::~NEDevice() {
    mDeletionQueue.flush();
}

vkb::PhysicalDevice NEDevice::init_PhysicalDevice(VkSurfaceKHR surface, vkb::Instance &vkb_inst) {
    ///Create/Select rootDevice;
    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    vkb::PhysicalDevice physicalDevice = selector
            .set_minimum_version(1, 1)
            .set_surface(surface)
            .select()
            .value();

    mGPU = physicalDevice.physical_device;

    vkGetPhysicalDeviceProperties(mGPU, &mGPUProperties);
    std::cout << "The GPU has a minimum buffer alignment of " << mGPUProperties.limits.minUniformBufferOffsetAlignment << std::endl;

    return physicalDevice;
}

bool NEDevice::init_LogicalDevice(vkb::PhysicalDevice &physicalDevice) {
    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();

    mDevice = vkbDevice.device;

    mGraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    mGraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    mPresentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();
    mPresentQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::present).value();

    mDeletionQueue.push_function([=]() {
        vkDestroyDevice(mDevice, nullptr);
    });
}

void NEDevice::init_Allocator(VkInstance instance) {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = mGPU;
    allocatorInfo.device = mDevice;
    allocatorInfo.instance = instance;
    vmaCreateAllocator(&allocatorInfo, &mAllocator);

    mDeletionQueue.push_function([=]() {
        vmaDestroyAllocator(mAllocator);
    });
}

VkRenderPass NEDevice::createDefaultRenderpass(VkFormat format) {
    if(mDefaultRenderpass != VK_NULL_HANDLE) return mDefaultRenderpass;

//Color
     VkAttachmentDescription color_attachment = {};
    ///TODO MSAA
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;

    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    color_attachment.format = format;

//Other stuff

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


    VkAttachmentDescription depth_attachment = {};
    depth_attachment.flags = 0;
    depth_attachment.format = VK_FORMAT_D32_SFLOAT;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref = {};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

//Subpasses
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    //hook the depth attachment into the subpass
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    //Tape em all together
    VkAttachmentDescription attachments[2] = { color_attachment, depth_attachment };

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    render_pass_info.attachmentCount = 2;
    render_pass_info.pAttachments = &attachments[0];

    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    VkRenderPass renderPass;

    if(vkCreateRenderPass(mDevice, &render_pass_info, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Renderpass failed to initialize\n");
    }

    mDeletionQueue.push_function([=]() {
        vkDestroyRenderPass(mDevice, renderPass, nullptr);
    });

    mDefaultRenderpass = renderPass;
    return renderPass;
}

std::pair<VkPipeline, VkPipelineLayout> NEDevice::createPipeline(std::vector<uint32_t> vertexShaderSpirv, std::vector<uint32_t> fragmentShaderSpirv, uint32_t flags) {
    VkShaderModule vertexShader;
    if (!loadShaderModule(std::move(vertexShaderSpirv), vertexShader))
    {
        std::cout << "Error when building the requested vertex shader" << std::endl;
    }

    VkShaderModule fragmentShader;
    if (!loadShaderModule(std::move(fragmentShaderSpirv), fragmentShader))
    {
        std::cout << "Error when building the fragment shader" << std::endl;

    }


    //Pipeline creation
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipeline_layout_create_info();

    if ((flags & NE_SHADER_PUSHCONSTANTS_BIT) == NE_SHADER_PUSHCONSTANTS_BIT) {
        VkPushConstantRange push_constant;
        push_constant.offset = 0;
        push_constant.size = sizeof(MeshPushConstants);
        push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        pipelineLayoutInfo.pPushConstantRanges = &push_constant;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
    }
    else {
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        pipelineLayoutInfo.pPushConstantRanges = 0;
    }

    VkDescriptorSetLayout layouts[3] = {mGlobalSetLayout, mObjectSetLayout};
    uint8_t layoutSize = 2;

        if((flags & NE_SHADER_TEXTURE_BIT) == NE_SHADER_TEXTURE_BIT) {
            layouts[layoutSize] = mSingleTextureSetLayout;
            layoutSize++;
        }

    pipelineLayoutInfo.setLayoutCount = layoutSize;
    pipelineLayoutInfo.pSetLayouts = layouts;

    VkPipelineLayout layout;
    if (vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) {
        throw std::runtime_error("Pipeline layout creation failed\n");
    }

    PipelineBuilder builder;
    builder.mShaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
    builder.mShaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));

    builder.mVertexInputInfo = vkinit::vertex_input_state_create_info();

    VertexInputDescription vertexDescription = Vertex::get_vertex_description();
    builder.mVertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
    builder.mVertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();

    builder.mVertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
    builder.mVertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

    //How to group shapes.
    builder.mInputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    builder.mViewport.x = 0.f;
    builder.mViewport.y = 0.f;
    builder.mViewport.width = 1;
    builder.mViewport.height = 1;
    builder.mViewport.minDepth = 0.f;
    builder.mViewport.maxDepth = 1.f;

    builder.mScissor.offset = {0, 0 };
    builder.mScissor.extent = {1, 1};

    //Wireframe, points, or filled I think...
    builder.mRasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    builder.mMultisampling = vkinit::multisampling_state_create_info();

    builder.mColorBlendAttachment = vkinit::color_blend_attachment_state();

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT};

    builder.mDynamicState = vkinit::dynamic_state_create_info(dynamicStates);

    builder.mDepthStencil = init::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    std::pair<VkPipeline, VkPipelineLayout> temp;
    VkPipeline pipeline;

    builder.mPipelineLayout = layout;
    pipeline = builder.build_pipeline(mDevice, mDefaultRenderpass);

    temp = {pipeline, layout};

    vkDestroyShaderModule(mDevice, vertexShader, nullptr);
    vkDestroyShaderModule(mDevice, fragmentShader, nullptr);

    return temp;
}

bool NEDevice::loadShaderModule(std::vector<uint32_t> spirv, VkShaderModule &outShaderModule) {
    //std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    //if (!file.is_open()) {
    //    return false;
    //}
//
    ////Create buffer for asm
    //size_t fileSize = (size_t)file.tellg();
    //std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
//
    ////Read asm into buffer
    //file.seekg(0);
    //file.read((char*)buffer.data(), fileSize);
//
    ////close
    //file.close();

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;

    createInfo.codeSize = spirv.size() * sizeof(uint32_t);
    createInfo.pCode = spirv.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(mDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return false;
    }
    outShaderModule = shaderModule;
    return true;
}

///User of this function must handle destruction...
VkCommandPool NEDevice::createCommandPool(uint32_t queueFamily) {
    VkCommandPool cmdPool;
    VkCommandPoolCreateInfo commandPoolInfo = init::command_pool_create_info(queueFamily,VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    if (vkCreateCommandPool(mDevice, &commandPoolInfo, nullptr, &cmdPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create commandPool\n");
    }

    return cmdPool;
}

AllocatedBuffer NEDevice::createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
    //allocate vertex buffer
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;

    bufferInfo.size = allocSize;
    bufferInfo.usage = usage;


    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = memoryUsage;

    AllocatedBuffer newBuffer {};

    //allocate the buffer
    vmaCreateBuffer(mAllocator, &bufferInfo, &vmaallocInfo,
                             &newBuffer.mBuffer,
                             &newBuffer.mAllocation,
                             nullptr);

    return newBuffer;
}

void NEDevice::init_descriptors() {
    //create a descriptor pool that will hold 10 uniform buffers
    std::vector<VkDescriptorPoolSize> sizes =
            {
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
                    //wawawwa image sampler wawawawawawawawaw
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10}
            };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = 0;
    pool_info.maxSets = 10;
    pool_info.poolSizeCount = (uint32_t)sizes.size();
    pool_info.pPoolSizes = sizes.data();

    vkCreateDescriptorPool(mDevice, &pool_info, nullptr, &mDescriptorPool);

    mDeletionQueue.push_function([&]() {
        vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
    });

    VkDescriptorSetLayoutBinding cameraBind = createDescriptorSetBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
    VkDescriptorSetLayoutBinding sceneBind = createDescriptorSetBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);
    VkDescriptorSetLayoutBinding bindings[] = { cameraBind, sceneBind };

    mGlobalSetLayout = createDescriptorSetLayout(bindings, 2);

    VkDescriptorSetLayoutBinding objectBind = createDescriptorSetBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
    mObjectSetLayout = createDescriptorSetLayout(&objectBind, 1);

    VkDescriptorSetLayoutBinding textureBind = createDescriptorSetBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
    mSingleTextureSetLayout = createDescriptorSetLayout(&textureBind, 1);

}

VkDescriptorSetLayoutBinding NEDevice::createDescriptorSetBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding) {

    //information about the binding.
    VkDescriptorSetLayoutBinding setBind = {};
    setBind.binding = binding;
    setBind.descriptorCount = 1;
    // it's a uniform buffer binding
    setBind.descriptorType = type;

    // we use it from the vertex shader
    setBind.stageFlags = stageFlags;

    return setBind;

}

VkDescriptorSetLayout NEDevice::createDescriptorSetLayout(VkDescriptorSetLayoutBinding *bindingArray,
                                                          uint8_t arraySize) {
    VkDescriptorSetLayout layout;

    VkDescriptorSetLayoutCreateInfo setInfo = {};
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setInfo.pNext = nullptr;

    //we are going to have 1 binding
    setInfo.bindingCount = arraySize;
    //no flags
    setInfo.flags = 0;
    //point to the camera buffer binding
    setInfo.pBindings = bindingArray;


    vkCreateDescriptorSetLayout(mDevice, &setInfo, nullptr, &layout);

    mDeletionQueue.push_function([=]() {
        vkDestroyDescriptorSetLayout(mDevice, layout, nullptr);
    });

    return layout;
}

VkDescriptorSet NEDevice::createDescriptorSet(VkDescriptorSetLayout layout) {
    VkDescriptorSet descriptorSet;

    //allocate one descriptor set for each frame
    VkDescriptorSetAllocateInfo allocInfo ={};
    allocInfo.pNext = nullptr;
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    //using the pool we just set
    allocInfo.descriptorPool = mDescriptorPool;
    //only 1 descriptor
    allocInfo.descriptorSetCount = 1;
    //using the global data layout
    allocInfo.pSetLayouts = &layout;

    vkAllocateDescriptorSets(mDevice, &allocInfo, &descriptorSet);

    return descriptorSet;
}

size_t NEDevice::padUniformBufferSize(size_t originalSize) {
    // Calculate required alignment based on minimum device offset alignment
    size_t minUboAlignment = mGPUProperties.limits.minUniformBufferOffsetAlignment;
    size_t alignedSize = originalSize;
    if (minUboAlignment > 0) {
        alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }
    return alignedSize;
}

void NEDevice::init_upload_context() {
    VkFenceCreateInfo fenceCreateInfo = init::fenceCreateInfo();
    vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &mUploadContext.mUploadFence);

    mDeletionQueue.push_function([=]() {
        vkDestroyFence(mDevice, mUploadContext.mUploadFence, nullptr);
    });

    mUploadContext.mCommandPool = createCommandPool(mGraphicsQueueFamily);

    mDeletionQueue.push_function([=]() {
        vkDestroyCommandPool(mDevice, mUploadContext.mCommandPool, nullptr);
    });
}

void NEDevice::immediateSubmit(std::function<void(VkCommandBuffer)> &&function) {

    //allocate the default command buffer that we will use for the instant commands
    VkCommandBufferAllocateInfo cmdAllocInfo = init::command_buffer_allocate_info(mUploadContext.mCommandPool, 1);

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(mDevice, &cmdAllocInfo, &cmd);

    //begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that

    VkCommandBufferBeginInfo cmdBeginInfo = init::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

   vkBeginCommandBuffer(cmd, &cmdBeginInfo);

    //execute the function
    function(cmd);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = init::submitInfo(&cmd, 1);


    //submit command buffer to the queue and execute it.
    // _uploadFence will now block until the graphic commands finish execution
    vkQueueSubmit(mGraphicsQueue, 1, &submit, mUploadContext.mUploadFence);

    vkWaitForFences(mDevice, 1, &mUploadContext.mUploadFence, true, 9999999999);
    vkResetFences(mDevice, 1, &mUploadContext.mUploadFence);

    //clear the command pool. This will free the command buffer too
    vkResetCommandPool(mDevice, mUploadContext.mCommandPool, 0);
}

void NEDevice::createSampler(Material *material, Texture *texture) {
    //create a sampler for the texture
    VkSamplerCreateInfo samplerInfo = init::samplerCreateInfo(VK_FILTER_NEAREST);

    VkSampler blockySampler;
    vkCreateSampler(mDevice, &samplerInfo, nullptr, &blockySampler);

    //allocate the descriptor set for single-texture to use on the material
    material->mTextureSet = createDescriptorSet(mSingleTextureSetLayout);

    //write to the descriptor set so that it points to our empire_diffuse texture
    VkDescriptorImageInfo imageBufferInfo;
    imageBufferInfo.sampler = blockySampler;
    imageBufferInfo.imageView = texture->mImageView;
    imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet texture1 = init::writeDescriptorImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, material->mTextureSet, &imageBufferInfo, 0);

    vkUpdateDescriptorSets(mDevice, 1, &texture1, 0, nullptr);
}

///Getters
VkDevice NEDevice::device() {
    return mDevice;
}
VkPhysicalDevice NEDevice::GPU() {
    return mGPU;
}
VkQueue NEDevice::presentQueue() {
    return mPresentQueue;
}
VkQueue NEDevice::graphicsQueue() {
    return mGraphicsQueue;
}

uint32_t NEDevice::presentQueueFamily() {
    return mPresentQueueFamily;
}

uint32_t NEDevice::graphicsQueueFamily() {
    return mGraphicsQueueFamily;
}

VkRenderPass NEDevice::defaultRenderpass() {
    return mDefaultRenderpass;
}
VmaAllocator NEDevice::allocator() {
    return mAllocator;
}

VkDescriptorPool NEDevice::descriptorPool() {
    return mDescriptorPool;
}

VkDescriptorSetLayout NEDevice::globalSetLayout() {
    return mGlobalSetLayout;
}

VkDescriptorSetLayout NEDevice::objectSetLayout() {
    return mObjectSetLayout;
}

VkDescriptorSetLayout NEDevice::singleTextureSetLayout() {
    return mSingleTextureSetLayout;
}