//
// Created by nineball on 7/16/21.
//

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
#include <iostream>
#include <fstream>
#include <utility>

#include "Pipeline.h"
#include "Device.h"
#include "Initializers.h"
#include "Mesh.h"
#include "../shaders/Shaders.h"

NEDevice::NEDevice() {

}

NEDevice::~NEDevice() {
    mDeletionQueue.flush();
}

//Init methods
vkb::PhysicalDevice NEDevice::init_PhysicalDevice(VkSurfaceKHR surface, vkb::Instance &vkb_inst) {
    ///Create/Select rootDevice;
    vkb::PhysicalDeviceSelector selector{ vkb_inst };

    VkPhysicalDeviceVulkan12Features vk12Features{};
    vk12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

    vk12Features.descriptorBindingPartiallyBound = VK_TRUE;
    vk12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    vk12Features.runtimeDescriptorArray = VK_TRUE;

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

 vkb::PhysicalDevice physicalDevice;

 auto selectorReturn = selector
         .set_minimum_version(1, 2)
         .set_surface(surface)
         .set_required_features_12(vk12Features)
         .set_required_features(deviceFeatures)
         .select();

 ///Test if all desired extensions are supported, if not then fallback to "legacy" mode...
 if(selectorReturn) {
    physicalDevice = selectorReturn.value();
 }
 else {
     std::cout << "Needed vulkan features unsupported, falling back to legacy backend\n";
     vkb::PhysicalDeviceSelector legacySelector{ vkb_inst };
     physicalDevice = legacySelector
             .set_minimum_version(1, 1)
             .set_surface(surface)
             .set_required_features(deviceFeatures)
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
    mMaxAnisotropy = getMaxAnisotropy();

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

void NEDevice::init_descriptors() {
    //create a descriptor pool that will hold 10 uniform buffers
    std::vector<VkDescriptorPoolSize> sizes =
            {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
            //wawawwa image sampler wawawawawawawawaw
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_TEXTURES}
            };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 100;
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
        VkDescriptorSetLayoutBinding textureBind = createDescriptorSetBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
        textureBind.descriptorCount = MAX_TEXTURES;
        VkDescriptorSetLayoutBinding textureBindings[] = { textureBind };

        VkDescriptorBindingFlags flags[3];
        flags[0] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

        VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags{};
        binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        binding_flags.bindingCount = 1;
        binding_flags.pBindingFlags = flags;

        mTextureSetLayout = createDescriptorSetLayout(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT, textureBindings, 1, &binding_flags);
    }
    else {
        VkDescriptorSetLayoutBinding singleTexBindings[2];
        singleTexBindings[0] = createDescriptorSetBinding(VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
        singleTexBindings[1] = createDescriptorSetBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 1);

        mSingleTextureSetLayout = createDescriptorSetLayout(0, singleTexBindings, 2);
    }
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

VkRenderPass NEDevice::getRenderPass(uint32_t flags, VkFormat format) {
    ///TODO proper msaa support built into the gui/ Engine.mSettings

    if(mRenderPassList.contains(flags)) {
        return mRenderPassList[flags];
    }
    else if(format != VK_FORMAT_UNDEFINED) {
        mRenderPassList[flags] = createRenderpass(format, flags | NE_FLAG_MSAA8x_BIT);
        return mRenderPassList[flags];
    }
    else{
        std::cout << "RenderPass not created, and not format was provided to crate one.\n";
        return VK_NULL_HANDLE;
    }
}

std::pair<VkPipeline, VkPipelineLayout> NEDevice::getPipeline(uint32_t rendermode, uint32_t features) {
    if(mBindless) {
    //    features |= NE_FLAG_BINDING_BIT;
    }

    if(mRenderPassList.contains(rendermode)) {
       if(mPipelineList[rendermode].contains(features)) {
           return mPipelineList[rendermode][features];
       }
       else {
           //Load shader
           std::pair<std::string, std::string> shaders = assembleShaders(features);
           std::vector<uint32_t> vertex, fragment;
           vertex = compileShader("vertex", shaderc_glsl_vertex_shader, shaders.first, true);
           fragment = compileShader("fragment", shaderc_glsl_fragment_shader, shaders.second, true);

           //create and return pipeline
           mPipelineList[rendermode][features] = createPipeline(mRenderPassList[rendermode],
                                                                vertex, fragment,
                                                                features | NE_FLAG_MSAA8x_BIT);
           return mPipelineList[rendermode][features];
       }
    } else {
        std::cout << "No renderpass to fulfill pipeline with requested rendermode";
        return{};
    }
}

VkRenderPass NEDevice::createRenderpass(VkFormat format, uint32_t flags) {
    bool ToSC = false;
    bool ToTex = false;

    bool MSAA;

    VkSampleCountFlagBits MSAAStrength;

    //RenderMode
    if((flags & NE_RENDERMODE_TOSWAPCHAIN_BIT) == NE_RENDERMODE_TOSWAPCHAIN_BIT) {
        ToSC = true;
    }
    else if ((flags & NE_RENDERMODE_TOTEXTURE_BIT) == NE_RENDERMODE_TOTEXTURE_BIT) {
        ToTex = true;
    }
    else {
        throw std::runtime_error("Didnt specify renderpass type");
    }

    //MSAA
    if((flags & NE_FLAG_MSAA8x_BIT) == NE_FLAG_MSAA8x_BIT) {
        MSAAStrength = VK_SAMPLE_COUNT_8_BIT;
        MSAA = true;
    }
    else if ((flags & NE_FLAG_MSAA4x_BIT) == NE_FLAG_MSAA4x_BIT) {
        MSAAStrength = VK_SAMPLE_COUNT_4_BIT;
        MSAA = true;
    }
    else if ((flags & NE_FLAG_MSAA2x_BIT) == NE_FLAG_MSAA2x_BIT) {
        MSAAStrength = VK_SAMPLE_COUNT_2_BIT;
        MSAA = true;
    }
    else {
        MSAAStrength = VK_SAMPLE_COUNT_1_BIT;
        MSAA = false;
    }

    //Depth
    VkAttachmentDescription depth_attachment = {};
    depth_attachment.flags = 0;
    depth_attachment.format = VK_FORMAT_D32_SFLOAT;
    depth_attachment.samples = MSAAStrength;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref = {};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    //Color
    VkAttachmentDescription color_attachment = {};

    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    color_attachment.samples = MSAAStrength;
    color_attachment.format = format;

    if(MSAA) {
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    else {
        if(ToSC) {
            color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }
        else if (ToTex) {
            color_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    }


    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    //Only needed for MSAA
    VkAttachmentDescription colorAttachmentResolve{};
    VkAttachmentReference colorAttachmentResolveRef{};

    if(MSAA) {
        colorAttachmentResolve.format = format;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        if(ToSC) {
            colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }
        else if(ToTex) {
            colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

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

    render_pass_info.attachmentCount = MSAA ? 3 : 2;
    render_pass_info.pAttachments = attachments;

    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    VkRenderPass renderPass;

    if(vkCreateRenderPass(mDevice, &render_pass_info, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Renderpass failed to initialize\n");
    }

    return renderPass;
}

std::pair<VkPipeline, VkPipelineLayout> NEDevice::createPipeline(VkRenderPass renderpass, std::vector<uint32_t> vertexShaderSpirv, std::vector<uint32_t> fragmentShaderSpirv, uint32_t flags) {
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
    pipeline = builder.build_pipeline(mDevice, renderpass);

    temp = {pipeline, layout};

    vkDestroyShaderModule(mDevice, vertexShader, nullptr);
    vkDestroyShaderModule(mDevice, fragmentShader, nullptr);

    return temp;
}

bool NEDevice::loadShaderModule(std::vector<uint32_t> spirv, VkShaderModule &outShaderModule) {
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


VkSampler NEDevice::createSampler(uint32_t mipLevels) {
    //create a sampler for the texture
    VkSamplerCreateInfo samplerInfo = init::samplerCreateInfo(VK_FILTER_LINEAR, mMaxAnisotropy, mipLevels);


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

float NEDevice::getMaxAnisotropy() {
    float counts = mGPUProperties.limits.maxSamplerAnisotropy;

    std::cout << "Max Anisotropy supported for device is: ";

    if(counts >= 16) {std::cout << "16x\n"; counts = 16;}
    else if (counts >= 8) {std::cout << "8x\n"; counts = 8;}
    else if (counts >= 4) {std::cout << "4x\n"; counts = 4;}
    else if (counts >= 2) {std::cout << "2x\n"; counts = 2;}
    else {std::cout << "Unsupported\n"; counts = 0;}

    return counts;
}

void NEDevice::transitionImageLayout(VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout, uint32_t mipLevel) {

    ///Transition image and copy to GPU buffer
    immediateSubmit([=](VkCommandBuffer cmd) {
        VkImageMemoryBarrier imageBarrier = {};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.oldLayout = srcLayout;
        imageBarrier.newLayout = dstLayout;
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier.image = image;

        imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;;
        imageBarrier.subresourceRange.baseArrayLayer = 0;
        imageBarrier.subresourceRange.levelCount = mipLevel;
        imageBarrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (srcLayout == VK_IMAGE_LAYOUT_UNDEFINED && dstLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            imageBarrier.srcAccessMask = 0;
            imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (srcLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && dstLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }


        //barrier the image into the transfer-receive layout
        vkCmdPipelineBarrier(cmd, sourceStage, destinationStage, 0, 0,
                             nullptr, 0, nullptr,
                             1, &imageBarrier);
    });
}

void NEDevice::copyBufferToImage(VkBuffer buffer, VkImage image, VkExtent2D extent) {

        immediateSubmit([=](VkCommandBuffer cmd) {
            VkBufferImageCopy copyRegion = {};
            copyRegion.bufferOffset = 0;
            copyRegion.bufferRowLength = 0;
            copyRegion.bufferImageHeight = 0;

            copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.imageSubresource.mipLevel = 0;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount = 1;
            copyRegion.imageExtent = {
                    extent.width,
                    extent.height,
                    1
            };

            //copy the buffer into the image
            vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   1, &copyRegion);
    });
}

void NEDevice::generateMipmaps(VkImage image, VkFormat imageFormat, VkExtent2D texSize, uint32_t mipLevels) {

    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(mGPU, imageFormat, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    int mipWidth = texSize.width;
    int mipHeight = texSize.height;

    immediateSubmit([&](VkCommandBuffer cmd) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;


        for(uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(cmd,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);


            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {static_cast<int32_t>(mipWidth), static_cast<int32_t>(mipHeight), 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(cmd,
                           image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &blit,
                           VK_FILTER_LINEAR);


            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmd,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

    });
}

///Getters
VkDevice NEDevice::device() {return mDevice;}
VkPhysicalDevice NEDevice::GPU() {return mGPU;}
VkQueue NEDevice::presentQueue() {return mPresentQueue;}
VkQueue NEDevice::graphicsQueue() {return mGraphicsQueue;}
uint32_t NEDevice::presentQueueFamily() {return mPresentQueueFamily;}
uint32_t NEDevice::graphicsQueueFamily() {return mGraphicsQueueFamily;}
VmaAllocator NEDevice::allocator() {return mAllocator;}
VkDescriptorPool NEDevice::descriptorPool() {return mDescriptorPool;}
VkDescriptorSetLayout NEDevice::globalSetLayout() {return mGlobalSetLayout;}
VkDescriptorSetLayout NEDevice::objectSetLayout() {return mObjectSetLayout;}
VkDescriptorSetLayout NEDevice::singleTextureSetLayout() {return mSingleTextureSetLayout;}
VkDescriptorSetLayout NEDevice::textureSetLayout() {return mTextureSetLayout;}
bool NEDevice::bindless() {return mBindless;}
VkSampleCountFlagBits NEDevice::sampleCount() {return mSampleCount;}