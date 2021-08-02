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

    VkPhysicalDeviceVulkan12Features vk12Features{};
    vk12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

    vk12Features.descriptorBindingPartiallyBound = VK_TRUE;
    vk12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    vk12Features.runtimeDescriptorArray = VK_TRUE;

 //   selector.add_required_extension_features(vk12Features);


 vkb::PhysicalDevice physicalDevice;

 auto selectorReturn = selector
         .set_minimum_version(1, 2)
         .set_surface(surface)
         .add_required_extension_features(vk12Features)
         .select();

 ///Test if all desired extensions are supported, if not then fallback to "legacy" mode...
 if(selectorReturn) {
    physicalDevice = selectorReturn.value();
 }
 else {
     std::cout << "Needed vulkan features unsupported, falling back to legacy backend\n";
     physicalDevice = selector
             .set_minimum_version(1, 1)
             .set_surface(surface)
             .add_required_extension_features(vk12Features)
             .select()
             .value();
 }

    mGPU = physicalDevice.physical_device;


    vkGetPhysicalDeviceProperties(mGPU, &mGPUProperties);
    std::cout << "The GPU has a minimum buffer alignment of " << mGPUProperties.limits.minUniformBufferOffsetAlignment << std::endl;


    mGPUFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    mGPUFeatures.pNext = &mGPUFeaturesVK12;
    mGPUFeaturesVK12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

    vkGetPhysicalDeviceFeatures2(mGPU, &mGPUFeatures);


    if(mGPUFeaturesVK12.descriptorBindingPartiallyBound  &&
       mGPUFeaturesVK12.runtimeDescriptorArray           &&
       mGPUFeaturesVK12.shaderSampledImageArrayNonUniformIndexing) {
        mBindless = true;
    }
    else {
        std::cout << "Needed vulkan features unsupported, falling back to legacy backend\n";
        mBindless = false;
    }

    mSampleCount = getMaxSampleCount();

    return physicalDevice;
}

void NEDevice::init_LogicalDevice(vkb::PhysicalDevice &physicalDevice) {
    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();

    mDevice = vkbDevice.device;

    mGraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    mGraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    mPresentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();
    mPresentQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::present).value();

    mDeletionQueue.push_function([&, this]() {
        vkDestroyDevice(mDevice, nullptr);
    });
}

void NEDevice::init_Allocator(VkInstance instance) {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = mGPU;
    allocatorInfo.device = mDevice;
    allocatorInfo.instance = instance;
    vmaCreateAllocator(&allocatorInfo, &mAllocator);

    mDeletionQueue.push_function([=, this]() {
        vmaDestroyAllocator(mAllocator);
    });
}

VkRenderPass NEDevice::createDefaultRenderpass(VkFormat format) {
    if(mDefaultRenderpass != VK_NULL_HANDLE) return mDefaultRenderpass;

//Color
     VkAttachmentDescription color_attachment = {};
    ///TODO MSAA
    color_attachment.samples = mSampleCount;

    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.format = format;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = format;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depth_attachment = {};
    depth_attachment.flags = 0;
    depth_attachment.format = VK_FORMAT_D32_SFLOAT;
    depth_attachment.samples = mSampleCount;
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
    subpass.pResolveAttachments = &colorAttachmentResolveRef;
    //hook the depth attachment into the subpass
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    //Tape em all together
    VkAttachmentDescription attachments[3] = { color_attachment, depth_attachment, colorAttachmentResolve};

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    render_pass_info.attachmentCount = 3;
    render_pass_info.pAttachments = attachments;

    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    VkRenderPass renderPass;

    if(vkCreateRenderPass(mDevice, &render_pass_info, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Renderpass failed to initialize\n");
    }

    mDeletionQueue.push_function([=, this]() {
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


    VkPushConstantRange push_constants;
    push_constants.offset = 0;
    push_constants.size = sizeof(PushData);
    push_constants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pipelineLayoutInfo.pPushConstantRanges = &push_constants;
    pipelineLayoutInfo.pushConstantRangeCount = 1;


    VkDescriptorSetLayout layouts[3] = {mGlobalSetLayout, mObjectSetLayout};
    uint8_t layoutSize = 2;

        if((flags & NE_SHADER_TEXTURE_BIT) == NE_SHADER_TEXTURE_BIT) {
            if(mBindless){
                layouts[layoutSize] = mTextureSetLayout;
            }
            else{
                layouts[layoutSize] = mSingleTextureSetLayout;
            }

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
    builder.mMultisampling = vkinit::multisampling_state_create_info(mSampleCount, false);

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
    if(vmaCreateBuffer(mAllocator, &bufferInfo, &vmaallocInfo,
                             &newBuffer.mBuffer,
                             &newBuffer.mAllocation,
                             nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create buffer");
    }

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
                    { VK_DESCRIPTOR_TYPE_SAMPLER, 10},
                    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_TEXTURES}
            };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = mBindless ? 0 : VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
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

    mGlobalSetLayout = createDescriptorSetLayout(0, bindings, 2);

    VkDescriptorSetLayoutBinding objectBind = createDescriptorSetBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
    mObjectSetLayout = createDescriptorSetLayout(0, &objectBind, 1);


    ///TODO make the descriptor count variable
    if(mBindless) {
        VkDescriptorSetLayoutBinding textureSamplerBind = createDescriptorSetBinding(VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
        VkDescriptorSetLayoutBinding textureBind = createDescriptorSetBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
        textureBind.descriptorCount = MAX_TEXTURES;
        VkDescriptorSetLayoutBinding textureBindings[] = { textureSamplerBind, textureBind };

        VkDescriptorBindingFlags flags[3];
        flags[0] = 0;
        flags[1] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

        VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags{};
        binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        binding_flags.bindingCount = 2;
        binding_flags.pBindingFlags = flags;

        mTextureSetLayout = createDescriptorSetLayout(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT, textureBindings, 2, &binding_flags);

    }
    else {
        VkDescriptorSetLayoutBinding singleTexBindings[2];
        singleTexBindings[0] = createDescriptorSetBinding(VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
        singleTexBindings[1] = createDescriptorSetBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 1);

        mSingleTextureSetLayout = createDescriptorSetLayout(0, singleTexBindings, 2);
    }

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

VkDescriptorSetLayout NEDevice::createDescriptorSetLayout(VkDescriptorSetLayoutCreateFlags flags,
                                                          VkDescriptorSetLayoutBinding *bindingArray, uint8_t arraySize,
                                                          void* pNext) {
    VkDescriptorSetLayout layout;

    VkDescriptorSetLayoutCreateInfo setInfo = {};
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setInfo.pNext = nullptr;

    //we are going to have 1 binding
    setInfo.bindingCount = arraySize;
    //no flags
    setInfo.flags = flags;
    //point to the camera buffer binding
    setInfo.pBindings = bindingArray;

    setInfo.pNext = pNext;

    vkCreateDescriptorSetLayout(mDevice, &setInfo, nullptr, &layout);

    mDeletionQueue.push_function([=, this]() {
        vkDestroyDescriptorSetLayout(mDevice, layout, nullptr);
    });

    return layout;
}

VkDescriptorSet NEDevice::createDescriptorSet(VkDescriptorSetLayout layout, void* pNext) {
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

    allocInfo.pNext = pNext;

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

    mDeletionQueue.push_function([=, this]() {
        vkDestroyFence(mDevice, mUploadContext.mUploadFence, nullptr);
    });

    mUploadContext.mCommandPool = createCommandPool(mGraphicsQueueFamily);

    mDeletionQueue.push_function([=, this]() {
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

VkSampler NEDevice::createSampler() {
    //create a sampler for the texture
    VkSamplerCreateInfo samplerInfo = init::samplerCreateInfo(VK_FILTER_NEAREST);

    VkSampler sampler;
    vkCreateSampler(mDevice, &samplerInfo, nullptr, &sampler);

    return sampler;
}

VkSampleCountFlagBits NEDevice::getMaxSampleCount() {
    VkSampleCountFlags counts = mGPUProperties.limits.framebufferColorSampleCounts
                              & mGPUProperties.limits.framebufferDepthSampleCounts;

    VkSampleCountFlagBits count;

    std::cout << "Max MSAA supported for device is: ";
    if(counts & VK_SAMPLE_COUNT_64_BIT) { count = VK_SAMPLE_COUNT_64_BIT; std::cout << "64x\n";}
    else if(counts & VK_SAMPLE_COUNT_32_BIT) { count = VK_SAMPLE_COUNT_32_BIT; std::cout << "32x\n";}
    else if(counts & VK_SAMPLE_COUNT_16_BIT) { count = VK_SAMPLE_COUNT_16_BIT; std::cout << "16x\n";}
    else if(counts & VK_SAMPLE_COUNT_8_BIT) { count = VK_SAMPLE_COUNT_8_BIT; std::cout << "8x\n";}
    else if(counts & VK_SAMPLE_COUNT_4_BIT) { count = VK_SAMPLE_COUNT_4_BIT; std::cout << "4x\n";}
    else if(counts & VK_SAMPLE_COUNT_2_BIT) { count = VK_SAMPLE_COUNT_2_BIT; std::cout << "2x\n";}
    else { count = VK_SAMPLE_COUNT_1_BIT; std::cout << "MSAA not supported\n";}

    return count;
}

///Getters
VkDevice NEDevice::device() {return mDevice;}
VkPhysicalDevice NEDevice::GPU() {return mGPU;}
VkQueue NEDevice::presentQueue() {return mPresentQueue;}
VkQueue NEDevice::graphicsQueue() {return mGraphicsQueue;}
uint32_t NEDevice::presentQueueFamily() {return mPresentQueueFamily;}
uint32_t NEDevice::graphicsQueueFamily() {return mGraphicsQueueFamily;}
VkRenderPass NEDevice::defaultRenderpass() {return mDefaultRenderpass;}
VmaAllocator NEDevice::allocator() {return mAllocator;}
VkDescriptorPool NEDevice::descriptorPool() {return mDescriptorPool;}
VkDescriptorSetLayout NEDevice::globalSetLayout() {return mGlobalSetLayout;}
VkDescriptorSetLayout NEDevice::objectSetLayout() {return mObjectSetLayout;}
VkDescriptorSetLayout NEDevice::singleTextureSetLayout() {return mSingleTextureSetLayout;}
VkDescriptorSetLayout NEDevice::textureSetLayout() {return mTextureSetLayout;}
bool NEDevice::bindless() {return mBindless;}
VkSampleCountFlagBits NEDevice::sampleCount() {return mSampleCount;}