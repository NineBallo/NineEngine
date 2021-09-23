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
#include "memory"
#include <vk_mem_alloc.h>
#include "unistd.h"
#include "backends/imgui_impl_vulkan.h"

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

TextureID Vulkan::loadTexture(const std::string &filepath, const std::string &name) {
    TextureID tex = mDevice->loadTexture(filepath, name);
    mDisplays[0]->addTexture(tex);
    return tex;
}

void Vulkan::addTextureToDisplay(TextureID texID) {

}

Vulkan::~Vulkan(){
    vkDeviceWaitIdle(mDevice->device());

    mDisplays[0].reset();
    mDeletionQueue.flush();
    mDevice.reset();

    vkb::destroy_debug_utils_messenger(mInstance, mDebugMessenger);
    vkDestroyInstance(mInstance, nullptr);
}

void Vulkan::init() {
    ///Create context, display, and primary device
    init_vulkan();

    ///Create Swapchain, Finalize root display
    mDisplays[0]->startInit(mDevice);

    //VK_PRESENT_MODE_FIFO_KHR
    ///Create a default renderpass/framebuffers (kinda self explanatory but whatever)
    mDevice->init_descriptors();
    mDevice->init_upload_context();

    mDisplays[0]->finishInit();


    if(!mDevice->bindless()) {
        mSampler = mDevice->createSampler();

        mDeletionQueue.push_function([=, this]() {
            vkDestroySampler(mDevice->device(), mSampler, nullptr);
        });
    }
}

void Vulkan::init_vulkan() {
    ///CreateContext
    vkb::InstanceBuilder builder;

    auto inst_ret = builder.set_app_name("NineEngine")
            .request_validation_layers(debug)
            .require_api_version(1, 2, 0)
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
    createInfo.resizable = true;
    mDisplays[0].emplace(createInfo);
    mDevice = std::make_shared<NEDevice>();

    vkb::PhysicalDevice vkb_GPU = mDevice->init_PhysicalDevice(mDisplays[0]->surface(), vkb_inst);
    mDevice->init_LogicalDevice(vkb_GPU);
    mDevice->init_Allocator(vkb_inst.instance);
}

void Vulkan::createMesh(const std::string &filepath, const std::string &meshName) {
    MeshGroup meshGroup;
    meshGroup.load_objects_from_file(filepath);

    for(auto& mesh : meshGroup.mMeshes) {
        uploadMesh(mesh);
    }

    mMeshGroups[meshName] = meshGroup;
    mDeletionQueue.push_function([=, this]() {
        deleteMesh(meshName);
    });
}

bool Vulkan::deleteMesh(const std::string& meshName) {
    //Check if it was already deleted...
    if(!mMeshGroups.contains(meshName)) return false;

    MeshGroup& meshGroup = mMeshGroups[meshName];

    for(auto& mesh : meshGroup.mMeshes) {
        vmaDestroyBuffer(mDevice->allocator(), mesh.mVertexBuffer.mBuffer, mesh.mVertexBuffer.mAllocation);
        vmaDestroyBuffer(mDevice->allocator(), mesh.mIndexBuffer.mBuffer, mesh.mIndexBuffer.mAllocation);
        mMeshGroups.erase(meshName);
    }

    return true;
}

void Vulkan::uploadMesh(Mesh &mesh) {

    size_t indexBufferSize = mesh.mIndices.size() * sizeof(uint32_t);
    size_t vertexBufferSize = mesh.mVertices.size() * sizeof(Vertex);

    mesh.mVertexBuffer = mDevice->createBuffer(vertexBufferSize,
                                               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                               VMA_MEMORY_USAGE_GPU_ONLY);

    mesh.mIndexBuffer = mDevice->createBuffer(indexBufferSize,
                                               VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                               VMA_MEMORY_USAGE_GPU_ONLY);



    mDevice->uploadToBuffer(mesh.mVertices, mesh.mVertexBuffer, vertexBufferSize);
    mDevice->uploadToBuffer(mesh.mIndices, mesh.mIndexBuffer, indexBufferSize);
}

void Vulkan::makeRenderable(Entity entity, const std::string &mesh, std::vector<TextureID> Textures, std::vector<std::string> textureIndex) {
   // Flags features = 0;
   //
   // //Set hidden bits/flags
   // if(!mDevice->bindless()) {
   //     features += NE_FLAG_BINDING_BIT;
   // }

    RenderObject renderObject{};

    renderObject.renderMode = NE_RENDERMODE_TOTEXTURE_BIT;

    renderObject.meshGroup = &mMeshGroups[mesh];
    MeshGroup& meshGroup = mMeshGroups[mesh];


    if(!Textures.empty() && !textureIndex.empty()) {
        renderObject.features += NE_SHADER_TEXTURE_BIT;
        //For each material, get the respective texture
        for (int i = 0; i < meshGroup.mMatToIdx.size(); i++) {

            uint32_t texIdx = meshGroup.mMatToIdx[textureIndex[i]];
            meshGroup.mTextures[texIdx] = Textures[i];
        }
    }

    mECS->addComponent<RenderObject>(entity, renderObject);
}

void Vulkan::draw() {
    VkCommandBuffer cmd = mDisplays[0]->startFrame();

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
    glm::mat4 projection = glm::perspective(glm::radians(camera.degrees), camera.aspect,
                                            camera.znear, camera.zfar);
    projection[1][1] *= -1;

    GPUCameraData cameraData {};
    cameraData.proj = projection;
    cameraData.view = view;
    cameraData.viewproj = projection * view;

    void* data;
    vmaMapMemory(mDevice->allocator(), mDisplays[0]->currentFrame().mCameraBuffer.mAllocation, &data);
    memcpy(data, &cameraData, sizeof(GPUCameraData));
    vmaUnmapMemory(mDevice->allocator(), mDisplays[0]->currentFrame().mCameraBuffer.mAllocation);


    ///Calculate all positions and send to gpu
    void* objectData;
    vmaMapMemory(mDevice->allocator(), mDisplays[0]->currentFrame().mObjectBuffer.mAllocation, &objectData);

    auto* objectSSBO = (GPUObjectData*)objectData;

    for (int i = 0; i < mEntityListSize; i++)
    {
        Entity currentEntityID = mLocalEntityList[0][i];
        auto& position = ECS::Get().getComponent<Position>(currentEntityID);

        glm::mat4 model {1.0f};
        model = glm::translate(model, glm::vec3{position.coordinates});
        model = glm::rotate(model, position.rotations.x * (3.14f/180), {1.f, 0.f , 0.f});
        model = glm::rotate(model, position.rotations.y * (3.14f/180), {0.f, 1.f , 0.f});
        model = glm::rotate(model, position.rotations.z * (3.14f/180), {0.f, 0.f , 1.f});
        model = glm::scale(model, position.scalar);

        objectSSBO[currentEntityID].modelMatrix = model;
    }
    vmaUnmapMemory(mDevice->allocator(), mDisplays[0]->currentFrame().mObjectBuffer.mAllocation);

    Flags mLastFlags = 0;
    TextureID mLastTexture = MAX_TEXTURES + 1;

    for(Entity i = 0; i < mEntityListSize; i++) {
        Entity currentEntityID = mLocalEntityList[0][i];
        auto& currentEntity = ECS::Get().getComponent<RenderObject>(currentEntityID);
        MeshGroup& meshGroup = *currentEntity.meshGroup;

        auto pipelineInfo = mDevice->getPipeline(currentEntity.renderMode, currentEntity.features);
        if(currentEntity.features != mLastFlags) {

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInfo.first);
            mLastFlags = currentEntity.features;

            VkDescriptorSet globalDescriptorSet = mDisplays[0]->currentFrame().mGlobalDescriptor;
            VkDescriptorSet objectDescriptorSet = mDisplays[0]->currentFrame().mObjectDescriptor;
            VkDescriptorSet textureDescriptorSet = mDisplays[0]->currentFrame().mTextureDescriptor;

            //object data descriptor
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInfo.second,
                                    1, 1, &objectDescriptorSet, 0, nullptr);

            uint32_t uniform_offset = mDevice->padUniformBufferSize(sizeof(GPUSceneData)) * mDisplays[0]->frameIndex();
            uniform_offset = 0;
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInfo.second,
                                    0, 1, &globalDescriptorSet, 1, &uniform_offset);

            if(mDevice->bindless()) {
                //texture descriptor
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        pipelineInfo.second, 2, 1,
                                        &textureDescriptorSet, 0, nullptr);
            }
        }

        for(auto & mesh : meshGroup.mMeshes) {
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.mVertexBuffer.mBuffer, &offset);
            vkCmdBindIndexBuffer(cmd, mesh.mIndexBuffer.mBuffer, 0 , VK_INDEX_TYPE_UINT32);

            PushData pushData{};
            pushData.entityID = currentEntityID;

            //Get texture for mesh
            uint32_t texIdx = meshGroup.mMatToIdx[mesh.mMaterial];
            TextureID texID = meshGroup.mTextures[texIdx];


            if(mDevice->bindless()) {
                pushData.textureIndex = mDisplays[0]->getTextureBinding(texID);
            }
            else if(mLastTexture != texID) {
                mLastTexture = texID;
                VkDescriptorSet textureSet = mDevice->getTexture(texID).mTextureSet;
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        pipelineInfo.second, 2, 1,
                                        &textureSet, 0, nullptr);
            }


            vkCmdPushConstants(cmd, pipelineInfo.second,
                               VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,0, sizeof(PushData), &pushData);


            vkCmdDrawIndexed(cmd, mesh.mIndices.size(), 1, 0, 0, 0);
        }
    }

    mDisplays[0]->endFrame();
}

void Vulkan::drawEntity(VkCommandBuffer cmd, Entity entity){
}

GLFWwindow* Vulkan::getWindow(Display display) {
    if(display == 0) {
        return mDisplays[0]->window();
    }
    else {
        throw std::runtime_error("Not Implemented\n");
    }
}

void Vulkan::tick() {
    if (mDisplays[0]->shouldExit()) {
        mDisplays[0].reset();
        mShouldExit = true;
    } else {
        draw();
    }
}

bool Vulkan::shouldExit() {return mShouldExit;}
