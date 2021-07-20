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
#include "chrono"
#include "Initializers.h"
#include "Pipeline.h"
#include "memory"
#include <vk_mem_alloc.h>


Vulkan::Vulkan(ECS &ecs, Entity cameraEntity) {
    mCameraEntity = cameraEntity;
    mECS = &ECS::Get();

    SubscribeData subscribeData {
            .localEntityList = &mLocalEntityList,
            .size = &mEntityListSize,
            .entityToPos = &mEntityToPos,
    };

    ECS::Get().registerComponent<RenderObject>();
    ECS::Get().registerComponent<Position>();

    ECS::Get().registerSystem<Vulkan>(subscribeData);

    Signature systemSig {};
    systemSig.set(ECS::Get().getComponentType<RenderObject>());
    systemSig.set(ECS::Get().getComponentType<Position>());

    ECS::Get().setSystemSignature<Vulkan>(systemSig);

    init();
}


Vulkan::~Vulkan(){
    mRootDisplay.reset();
    mDeletionQueue.flush();

    mDevice.reset();

    vkb::destroy_debug_utils_messenger(mInstance, mDebugMessenger);
    vkDestroyInstance(mInstance, nullptr);
}

void Vulkan::init() {
    ///Create context, display, and primary device
    init_vulkan();

    ///Create Swapchain, Finalize root display
    mRootDisplay->createSwapchain(mDevice, VK_PRESENT_MODE_IMMEDIATE_KHR);
    //VK_PRESENT_MODE_FIFO_KHR
    ///Create a default renderpass/framebuffers (kinda self explanatory but whatever)
    mDevice->createDefaultRenderpass(mRootDisplay->format());
    mDevice->init_descriptors();
    mRootDisplay->createFramebuffers(mDevice->defaultRenderpass());
    mRootDisplay->createDescriptors();
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
    mDevice = std::make_shared<NEDevice>();

    vkb::PhysicalDevice vkb_GPU = mDevice->init_PhysicalDevice(mRootDisplay->surface(), vkb_inst);
    mDevice->init_LogicalDevice(vkb_GPU);
    mDevice->init_Allocator(vkb_inst.instance);
}

Material* Vulkan::createMaterial(const std::string& vertexShaderPath, const std::string& fragmentShaderPath, const std::string &matName) {

    std::pair<VkPipeline, VkPipelineLayout> pipelines = mDevice->createPipeline(vertexShaderPath, fragmentShaderPath);

    Material material{};
    material.mPipeline = std::get<0>(pipelines);
    material.mPipelineLayout = std::get<1>(pipelines);
    mMaterials[matName] = material;

    mDeletionQueue.push_function([=]() {
        deleteMaterial(matName);
    });

    return &mMaterials[matName];
}

void Vulkan::deleteMaterial(const std::string &name) {
    Material& material = mMaterials[name];

    vkDestroyPipeline(mDevice->device(), material.mPipeline, nullptr);
    vkDestroyPipelineLayout(mDevice->device(), material.mPipelineLayout, nullptr);

    mMaterials.erase(name);
}

Mesh* Vulkan::createMesh(const std::string &filepath, const std::string &meshName) {
    Mesh mesh;
    mesh.load_from_obj(filepath);

    uploadMesh(mesh);

    mMeshes[meshName] = mesh;

    mDeletionQueue.push_function([=]() {
        deleteMesh(meshName);
    });

    return &mMeshes[meshName];
}

bool Vulkan::deleteMesh(std::string meshName) {
    Mesh& mesh = mMeshes[meshName];

    vmaDestroyBuffer(mDevice->allocator(), mesh.mVertexBuffer.mBuffer, mesh.mVertexBuffer.mAllocation);

    mMeshes.erase(meshName);
}

void Vulkan::makeRenderable(Entity entity, const std::string &material, const std::string &mesh) {
    RenderObject renderObject{};
    renderObject.material = &mMaterials[material];
    renderObject.mesh = &mMeshes[mesh];
    mECS->addComponent<RenderObject>(entity, renderObject);
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
    vmaCreateBuffer(mDevice->allocator(), &bufferInfo, &vmaallocInfo,
                             &mesh.mVertexBuffer.mBuffer,
                             &mesh.mVertexBuffer.mAllocation,
                             nullptr);

    //add the destruction of triangle mesh buffer to the deletion queue
    ///TODO move deletion queue to (device and display) instead of (engine & display)
    //mMainDeletionQueue.push_function([=]() {
    //    vmaDestroyBuffer(mDevice.allocator, mesh.mVertexBuffer.mBuffer, mesh.mVertexBuffer.mAllocation);
    //});

    void* data;
    vmaMapMemory(mDevice->allocator(), mesh.mVertexBuffer.mAllocation, &data);

    memcpy(data, mesh.mVertices.data(), mesh.mVertices.size() * sizeof(Vertex));

    vmaUnmapMemory(mDevice->allocator(), mesh.mVertexBuffer.mAllocation);
}


void Vulkan::draw() {
    VkCommandBuffer cmd = mRootDisplay->startFrame();

    //Camera setup
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

    GPUCameraData cameraData {};
    cameraData.proj = projection;
    cameraData.view = view;
    cameraData.viewproj = projection * view;

    void* data;
    vmaMapMemory(mDevice->allocator(), mRootDisplay->currentFrame().mCameraBuffer.mAllocation, &data);
    memcpy(data, &cameraData, sizeof(GPUCameraData));
    vmaUnmapMemory(mDevice->allocator(), mRootDisplay->currentFrame().mCameraBuffer.mAllocation);



    ///Calculate all positions and send to gpu
    void* objectData;
    vmaMapMemory(mDevice->allocator(), mRootDisplay->currentFrame().mObjectBuffer.mAllocation, &objectData);

    GPUObjectData* objectSSBO = (GPUObjectData*)objectData;

    for (int i = 0; i < mEntityListSize; i++)
    {
        Entity currentEntityID = mLocalEntityList[0][i];
        auto& position = ECS::Get().getComponent<Position>(currentEntityID);

        glm::mat4 model = glm::translate(glm::mat4 {1.f}, position.coordinates) * glm::translate(glm::mat4 {1.f}, position.coordinates) * glm::translate(glm::mat4 {1.f}, position.scalar); //= glm::rotate(glm::mat4{ 1.0f }, glm::radians(mRootDisplay->frameNumber() * 0.4f), glm::vec3(0, 1, 0));

        objectSSBO[i].modelMatrix = model;
    }
    vmaUnmapMemory(mDevice->allocator(), mRootDisplay->currentFrame().mObjectBuffer.mAllocation);

    Material* mLastMaterial = nullptr;

    for(Entity i = 0; i < mEntityListSize; i++) {
        Entity currentEntityID = mLocalEntityList[0][i];
        auto& currentEntity = ECS::Get().getComponent<RenderObject>(currentEntityID);

        if(currentEntity.material != mLastMaterial) {

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, currentEntity.material->mPipeline);
            mLastMaterial = currentEntity.material;

            VkDescriptorSet globalDescriptorSet = mRootDisplay->currentFrame().mGlobalDescriptor;
            VkDescriptorSet objectDescriptorSet = mRootDisplay->currentFrame().mObjectDescriptor;

            uint32_t uniform_offset = mDevice->padUniformBufferSize(sizeof(GPUSceneData));

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, currentEntity.material->mPipelineLayout,
                                    0, 1, &globalDescriptorSet, 1, &uniform_offset);

            //object data descriptor
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, currentEntity.material->mPipelineLayout,
                                1, 1, &objectDescriptorSet, 0, nullptr);

           }

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &currentEntity.mesh->mVertexBuffer.mBuffer, &offset);

        //Model matrix
        auto& position = ECS::Get().getComponent<Position>(currentEntityID);


     //  MeshPushConstants constants {};
     //  constants.render_matrix = model;
     //  vkCmdPushConstants(cmd, currentEntity.material->mPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

        vkCmdDraw(cmd, currentEntity.mesh->mVertices.size(), 1, 0, i);
    }

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
