//
// Created by nineball on 7/6/21.
//

#include "Vulkan.h"

#include <utility>
#include <VkBootstrap.h>
#include <iostream>
#include <fstream>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include "Initializers.h"
#include "Pipeline.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>


Vulkan::Vulkan(ECS &ecs, Entity cameraEntity) {
    mCameraEntity = cameraEntity;
    mECS = &ECS::Get();

    SubscribeData subscribeData {
            .localEntityList = &mLocalEntityList,
            .size = &mEntityListSize,
            .entityToPos = &mEntityToPos,
    };

    ECS::Get().registerComponent<Mesh>();
    ECS::Get().registerSystem<Vulkan>(subscribeData);

    Signature systemSig {};
    systemSig.set(ECS::Get().getComponentType<Mesh>());

    ECS::Get().setSystemSignature<Vulkan>(systemSig);

    init();
}


Vulkan::~Vulkan(){

    vmaDestroyAllocator(mDevice.allocator);

    vkDestroyRenderPass(mDevice.device, mDevice.renderpass, nullptr);
    vkDestroyPipeline(mDevice.device, mMeshPipeline, nullptr);
    vkDestroyPipelineLayout(mDevice.device, mTrianglePipelineLayout, nullptr);

    vkDestroyCommandPool(mDevice.device, mDevice.commandPool, nullptr);
    vkDestroyDevice(mDevice.device, nullptr);

    vkb::destroy_debug_utils_messenger(mInstance, mDebugMessenger);
    vkDestroyInstance(mInstance, nullptr);
}

void Vulkan::init() {
    ///Create context, display, and primary device
    init_vulkan();

    ///Create Swapchain, Finalize root display
    mRootDisplay->createSwapchain(mDevice.device, mDevice.GPU, mDevice.presentQueue, &mDevice.allocator);

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


    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = mDevice.GPU;
    allocatorInfo.device = mDevice.device;
    allocatorInfo.instance = mInstance;
    vmaCreateAllocator(&allocatorInfo, &mDevice.allocator);
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

    builder.color_attachment.format = mRootDisplay->format();


//Subpasses
    builder.subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    builder.subpass.colorAttachmentCount = 1;
    builder.subpass.pColorAttachments = &builder.color_attachment_ref;
    //hook the depth attachment into the subpass
    builder.subpass.pDepthStencilAttachment = &depth_attachment_ref;



    VkAttachmentDescription attachments[2] = { builder.color_attachment, depth_attachment };

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    render_pass_info.attachmentCount = 2;
    render_pass_info.pAttachments = &attachments[0];

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

    VkShaderModule meshVertexShader;
    if (!loadShaderModule("./shaders/mesh.vert.spv", meshVertexShader))
    {
        std::cout << "Error when building the vertex shader module" << std::endl;

    }
    else {
        std::cout << "Vertex shader successfully loaded" << std::endl;
    }

    //Pipeline creation
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipeline_layout_create_info();

    VkPushConstantRange push_constant;
    push_constant.offset = 0;
    push_constant.size = sizeof(MeshPushConstants);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    pipelineLayoutInfo.pPushConstantRanges = &push_constant;
    pipelineLayoutInfo.pushConstantRangeCount = 1;


    if (vkCreatePipelineLayout(mDevice.device, &pipelineLayoutInfo, nullptr, &mTrianglePipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Pipeline layout creation failed\n");
    }

    PipelineBuilder builder;
    builder.mShaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, meshVertexShader));
    builder.mShaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));



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
    builder.mViewport.width = mRootDisplay->extent().width;
    builder.mViewport.height = mRootDisplay->extent().height;
    builder.mViewport.minDepth = 0.f;
    builder.mViewport.maxDepth = 1.f;

    builder.mScissor.offset = {0, 0 };
    builder.mScissor.extent = mRootDisplay->extent();

    //Wireframe, points, or filled I think...
    builder.mRasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    builder.mMultisampling = vkinit::multisampling_state_create_info();

    builder.mColorBlendAttachment = vkinit::color_blend_attachment_state();

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT};

    builder.mDynamicState = vkinit::dynamic_state_create_info(dynamicStates);

    builder.mDepthStencil = init::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    builder.mPipelineLayout = mTrianglePipelineLayout;

    mMeshPipeline = builder.build_pipeline(mDevice.device, mDevice.renderpass);


    vkDestroyShaderModule(mDevice.device, meshVertexShader, nullptr);
    vkDestroyShaderModule(mDevice.device, triangleFragShader, nullptr);
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

void Vulkan::loadMesh(std::string filepath, Entity entity) {

    Mesh mesh;
    mesh.load_from_obj(filepath);

    uploadMesh(mesh);

    mECS->addComponent<Mesh>(entity, mesh);
}

void Vulkan::uploadMesh(Mesh &mesh) {
    //allocate vertex buffer
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    //this is the total size, in bytes, of the buffer we are allocating
    bufferInfo.size = mesh.mVertices.size() * sizeof(Vertex);
    //this buffer is going to be used as a Vertex Buffer
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;


    //let the VMA library know that this data should be writeable by CPU, but also readable by GPU
    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    //allocate the buffer
    vmaCreateBuffer(mDevice.allocator, &bufferInfo, &vmaallocInfo,
                             &mesh.mVertexBuffer.mBuffer,
                             &mesh.mVertexBuffer.mAllocation,
                             nullptr);

    //add the destruction of triangle mesh buffer to the deletion queue
    ///TODO move deletion queue to (device and display) instead of (engine & display)
    //mMainDeletionQueue.push_function([=]() {
    //    vmaDestroyBuffer(mDevice.allocator, mesh.mVertexBuffer.mBuffer, mesh.mVertexBuffer.mAllocation);
    //});

    void* data;
    vmaMapMemory(mDevice.allocator, mesh.mVertexBuffer.mAllocation, &data);

    memcpy(data, mesh.mVertices.data(), mesh.mVertices.size() * sizeof(Vertex));

    vmaUnmapMemory(mDevice.allocator, mesh.mVertexBuffer.mAllocation);
}

void Vulkan::draw() {
    VkCommandBuffer cmd = mRootDisplay->startFrame();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mMeshPipeline);

    VkDeviceSize offset = 0;


    //Camera setup
    //glm::vec3 camPos = { 0.f,-3.f,-2.f };
    //glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
    //glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
    //projection[1][1] *= -1;

    auto& camera = ECS::Get().getComponent<Camera>(mCameraEntity);

    //x = pitch, y = yaw, z = roll
    glm::vec3 angles = camera.Angle;

    glm::vec3 direction;
    direction.x = cos(glm::radians(angles.y)) * cos(glm::radians(angles.x));
    direction.y = sin(glm::radians(angles.x));
    direction.z = sin(glm::radians(angles.y)) * cos(glm::radians(angles.x));
    direction = glm::normalize(direction);

    glm::mat4 view = glm::lookAt(camera.Pos, camera.Pos + direction, glm::vec3(0, 1.f, 0));

    glm::mat4 projection = glm::perspective(glm::radians(camera.degrees), camera.aspect, camera.znear, camera.zfar);
    projection[1][1] *= -1;

  //  for(Entity i = 0; i > mEntityListSize; i++) {
      //  Entity currentEntity = mEntityToPos

        auto& mesh = ECS::Get().getComponent<Mesh>(1);

        vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.mVertexBuffer.mBuffer, &offset);

        //Model rotation
        glm::mat4 model(1.0f); //= glm::rotate(glm::mat4{ 1.0f }, glm::radians(mRootDisplay->frameNumber() * 0.4f), glm::vec3(0, 1, 0));

        //Final mesh matrix
        glm::mat4 mesh_matrix = projection * view * model;

        MeshPushConstants constants;
        constants.render_matrix = mesh_matrix;
        vkCmdPushConstants(cmd, mTrianglePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

        vkCmdDraw(cmd, mesh.mVertices.size(), 1, 0, 0);
  //  }


    mRootDisplay->endFrame();
}


void Vulkan::tick() {
    if (mRootDisplay->shouldExit()) {
        mRootDisplay.reset();
        mShouldExit = true;
    } else {
        draw();
    }
}


GLFWwindow * Vulkan::getWindow(Display display) {
    if(display == 0) {
        return mRootDisplay->window();
    }
    else {
        std::cout << "Not Implemented\n";
    }
}

bool Vulkan::shouldExit() {return mShouldExit;}
