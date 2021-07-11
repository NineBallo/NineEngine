//
// Created by nineball on 7/6/21.
//

#include "Vulkan.h"

#include <utility>
#include <VkBootstrap.h>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "Initializers.h"
#include "Pipeline.h"

Vulkan::Vulkan() {
    std::string path = std::filesystem::current_path();
    std::cout << path << std::endl;
    init();
}

Vulkan::~Vulkan(){
    vkDestroyCommandPool(mDevice.device, mDevice.commandPool, nullptr);
    vkDestroyDevice(mDevice.device, nullptr);
    vkb::destroy_debug_utils_messenger(mInstance, mDebugMessenger);
    vkDestroyInstance(mInstance, nullptr);
}

void Vulkan::init() {
    ///Create context, display, and primary device
    init_vulkan();

    ///Create Swapchain, Finalize root display
    mRootDisplay->createSwapchain(mDevice.device, mDevice.GPU, mDevice.presentQueue);

    ///Create primary command buffers
    init_commands(mDevice);

    ///Create a default renderpass/framebuffers (kinda self explanatory but whatever)
    init_default_renderpass();
    mRootDisplay->createFramebuffers(mDevice.renderpass);

    ///Setup sync structures
    mRootDisplay->createSyncStructures();

    init_pipelines();
}

void Vulkan::init_vulkan() {
    ///CreateContext
    vkb::InstanceBuilder builder;

    auto inst_ret = builder.set_app_name("NineEngine")
            .request_validation_layers(debug)
            .require_api_version(1, 1, 0)
            .use_default_debug_messenger()
            .build();

    vkb::Instance vkb_inst = inst_ret.value();

    mInstance = vkb_inst.instance;
    if(debug) {
        mDebugMessenger = vkb_inst.debug_messenger;
    }

    ///Create rootDisplay
    displayCreateInfo createInfo{};
    createInfo.instance = mInstance;
    createInfo.title = std::move(mTitle);
    createInfo.extent = { 800, 600 };
    mRootDisplay.emplace(createInfo);

    ///Create/Select rootDevice;
    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    vkb::PhysicalDevice physicalDevice = selector
            .set_minimum_version(1, 1)
            .set_surface(mRootDisplay->surface())
            .select()
            .value();

    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();

    mDevice.device = vkbDevice.device;
    mDevice.GPU = physicalDevice.physical_device;

    mDevice.graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    mDevice.graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    mDevice.presentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();
    mDevice.presentQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::present).value();
}

void Vulkan::init_commands(device& _device) {
    ///Create Command pool
    VkCommandPoolCreateInfo commandPoolInfo = init::command_pool_create_info(_device.graphicsQueueFamily,VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    if (vkCreateCommandPool(_device.device, &commandPoolInfo, nullptr, &_device.commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create commandPool\n");
    }

    mRootDisplay->createCommandBuffer(_device.commandPool);
}

void Vulkan::init_default_renderpass() {
    renderpassBuilder builder;

//Color
    ///TODO MSAA
    builder.color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;

    builder.color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    builder.color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    builder.color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    builder.color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    builder.color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    builder.color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

//Other color stuff
    builder.color_attachment_ref.attachment = 0;
    builder.color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

//Subpasses
    builder.subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    builder.subpass.colorAttachmentCount = 1;
    builder.subpass.pColorAttachments = &builder.color_attachment_ref;


    builder.color_attachment.format = mRootDisplay->format();

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &builder.color_attachment;

    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &builder.subpass;

    if(vkCreateRenderPass(mDevice.device, &render_pass_info, nullptr, &mDevice.renderpass) != VK_SUCCESS) {
        throw std::runtime_error("Renderpass failed to initialize\n");
    }
}

void Vulkan::init_pipelines() {
    VkShaderModule triangleFragShader;
    if (!loadShaderModule("./shaders/triangle.frag.spv", triangleFragShader))
    {
        std::cout << "Error when building the triangle fragment shader module" << std::endl;
    }
    else {
        std::cout << "Triangle fragment shader successfully loaded" << std::endl;
    }

    VkShaderModule triangleVertexShader;
    if (!loadShaderModule("./shaders/triangle.vert.spv", triangleVertexShader))
    {
        std::cout << "Error when building the triangle vertex shader module" << std::endl;

    }
    else {
        std::cout << "Triangle vertex shader successfully loaded" << std::endl;
    }

    //Pipeline creation
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipeline_layout_create_info();
    if (vkCreatePipelineLayout(mDevice.device, &pipelineLayoutInfo, nullptr, &mTrianglePipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Pipeline layout creation failed\n");
    }

    PipelineBuilder builder;
    builder.mShaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, triangleVertexShader));
    builder.mShaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));

    //Not using yet
    builder.mVertexInputInfo = vkinit::vertex_input_state_create_info();

    //How to group shapes.
    builder.mInputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    builder.mViewport.x = 0.f;
    builder.mViewport.y = 0.f;
    builder.mViewport.width = mRootDisplay->extent().width;
    builder.mViewport.height = mRootDisplay->extent().height;
    builder.mViewport.minDepth = 0.f;
    builder.mViewport.maxDepth = 1.f;

    builder.mScissor.offset = {0, 0 };

    //Wireframe, points, or filled I think...
    builder.mRasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    builder.mMultisampling = vkinit::multisampling_state_create_info();

    builder.mColorBlendAttachment = vkinit::color_blend_attachment_state();

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT};

    builder.mDynamicState = vkinit::dynamic_state_create_info(dynamicStates);
    builder.mPipelineLayout = mTrianglePipelineLayout;
    mTrianglePipeline = builder.build_pipeline(mDevice.device, mDevice.renderpass);
}


bool Vulkan::loadShaderModule(const char *filepath, VkShaderModule &outShaderModule) {
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    //Create buffer for asm
    size_t fileSize = (size_t)file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    //Read asm into buffer
    file.seekg(0);
    file.read((char*)buffer.data(), fileSize);

    //close
    file.close();

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;

    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(mDevice.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return false;
    }
    outShaderModule = shaderModule;
    return true;
}




void Vulkan::draw() {
    VkCommandBuffer cmd = mRootDisplay->startFrame();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mTrianglePipeline);
    vkCmdDraw(cmd, 3, 1, 0, 0);

    mRootDisplay->endFrame();
}


void Vulkan::tick() {
    while (!mRootDisplay->shouldExit()) {
        draw();
    }
    mRootDisplay.reset();
}