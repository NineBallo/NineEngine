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
#include "unistd.h"

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
    mDevice->init_upload_context();

    mRootDisplay->createFramebuffers(mDevice->defaultRenderpass());
    mRootDisplay->createDescriptors();
    mRootDisplay->initImGUI();
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
    mRootDisplay.emplace(createInfo);
    mDevice = std::make_shared<NEDevice>();

    vkb::PhysicalDevice vkb_GPU = mDevice->init_PhysicalDevice(mRootDisplay->surface(), vkb_inst);
    mDevice->init_LogicalDevice(vkb_GPU);
    mDevice->init_Allocator(vkb_inst.instance);
}

Material* Vulkan::createMaterial(uint32_t features) {

    std::pair<std::string, std::string> shaders = assembleShaders(features);

    std::vector<uint32_t>  vertex, fragment;
    vertex = compileShader("vertex", shaderc_glsl_vertex_shader, shaders.first, true);
    fragment = compileShader("fragment", shaderc_glsl_fragment_shader, shaders.second, true);

    std::pair<VkPipeline, VkPipelineLayout> pipelines = mDevice->createPipeline(vertex, fragment, features);


    Material material{};
    material.mPipeline = std::get<0>(pipelines);
    material.mPipelineLayout = std::get<1>(pipelines);
    mMaterials[features] = material;

    mDeletionQueue.push_function([=, this]() {
        deleteMaterial(features);
    });

    return &mMaterials[features];
}

void Vulkan::deleteMaterial(uint32_t features) {
    Material& material = mMaterials[features];

    vkDestroyPipeline(mDevice->device(), material.mPipeline, nullptr);
    vkDestroyPipelineLayout(mDevice->device(), material.mPipelineLayout, nullptr);

    mMaterials.erase(features);
}

void Vulkan::createMesh(const std::string &filepath, const std::string &meshName) {
    MeshGroup meshGroup;
    meshGroup.load_objects_from_file(filepath);

    for(auto& mesh : meshGroup.mMeshes) {
        uploadMesh(mesh);
    }

    mMeshes[meshName] = meshGroup;
    mDeletionQueue.push_function([=, this]() {
        deleteMesh(meshName);
    });

}

bool Vulkan::deleteMesh(const std::string& meshName) {
    //Check if it was already deleted...
    if(!mMeshes.contains(meshName)) return false;

    MeshGroup& meshGroup = mMeshes[meshName];

    for(auto& mesh : meshGroup.mMeshes) {
        vmaDestroyBuffer(mDevice->allocator(), mesh.mVertexBuffer.mBuffer, mesh.mVertexBuffer.mAllocation);
        vmaDestroyBuffer(mDevice->allocator(), mesh.mIndexBuffer.mBuffer, mesh.mIndexBuffer.mAllocation);
        mMeshes.erase(meshName);
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

void Vulkan::makeRenderable(Entity entity, uint32_t material, const std::string &mesh, std::string* Textures, uint32_t* textureIndex) {
    RenderObject renderObject{};
    renderObject.material = &mMaterials[material];
    renderObject.meshGroup = &mMeshes[mesh];

    if((material & NE_SHADER_TEXTURE_BIT) == NE_SHADER_TEXTURE_BIT) {
        MeshGroup& meshGroup = mMeshes[mesh];

        for (int i = 0; i < meshGroup.mMeshes.size(); i++) {
            uint32_t indexOfEntityTex =  textureIndex[i];
            mMeshes[mesh].mMeshes[i].texture = Textures[indexOfEntityTex];
        }
    }

    mECS->addComponent<RenderObject>(entity, renderObject);
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

    auto* objectSSBO = (GPUObjectData*)objectData;

    for (int i = 0; i < mEntityListSize; i++)
    {
        Entity currentEntityID = mLocalEntityList[0][i];
        auto& position = ECS::Get().getComponent<Position>(currentEntityID);

        glm::mat4 model {1.f};
        model = glm::translate(model, position.coordinates);
        model = glm::scale(model, position.scalar);
        model = glm::rotate(model, position.rotations.x * (3.14f/180), {1.f, 0.f , 0.f});
        model = glm::rotate(model, position.rotations.y * (3.14f/180), {0.f, 1.f , 0.f});
        model = glm::rotate(model, position.rotations.z * (3.14f/180), {0.f, 0.f , 1.f});

        objectSSBO[currentEntityID].modelMatrix = model;
    }
    vmaUnmapMemory(mDevice->allocator(), mRootDisplay->currentFrame().mObjectBuffer.mAllocation);


    Material* mLastMaterial = nullptr;



    for(Entity i = 0; i < mEntityListSize; i++) {
        Entity currentEntityID = mLocalEntityList[0][i];
        auto& currentEntity = ECS::Get().getComponent<RenderObject>(currentEntityID);
        MeshGroup& meshGroup = *currentEntity.meshGroup;


        if(currentEntity.material != mLastMaterial) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, currentEntity.material->mPipeline);
            mLastMaterial = currentEntity.material;

            VkDescriptorSet globalDescriptorSet = mRootDisplay->currentFrame().mGlobalDescriptor;
            VkDescriptorSet objectDescriptorSet = mRootDisplay->currentFrame().mObjectDescriptor;
            VkDescriptorSet textureDescriptorSet = mRootDisplay->currentFrame().mTextureDescriptor;

            uint32_t uniform_offset = mDevice->padUniformBufferSize(sizeof(GPUSceneData));

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, currentEntity.material->mPipelineLayout,
                                    0, 1, &globalDescriptorSet, 1, &uniform_offset);

            //object data descriptor
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, currentEntity.material->mPipelineLayout,
                                    1, 1, &objectDescriptorSet, 0, nullptr);


            //texture descriptor
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    currentEntity.material->mPipelineLayout, 2, 1,
                                    &textureDescriptorSet, 0, nullptr);
        }


        for(auto & mesh : meshGroup.mMeshes) {
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.mVertexBuffer.mBuffer, &offset);
            vkCmdBindIndexBuffer(cmd, mesh.mIndexBuffer.mBuffer, 0 , VK_INDEX_TYPE_UINT32);

            PushData pushData{};
            pushData.textureIndex = mTextureToBinding[mesh.texture];
            pushData.entityID = currentEntityID;


            vkCmdPushConstants(cmd, currentEntity.material->mPipelineLayout,
                               VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,0, sizeof(PushData), &pushData);


            vkCmdDrawIndexed(cmd, mesh.mIndices.size(), 1, 0, 0, 0);
        }
    }

    mRootDisplay->endFrame();
}

void Vulkan::drawEntity(VkCommandBuffer cmd, Entity entity){
}

GLFWwindow* Vulkan::getWindow(Display display) {
    if(display == 0) {
        return mRootDisplay->window();
    }
    else {
        throw std::runtime_error("Not Implemented\n");
    }
}

Texture* Vulkan::loadTexture(const std::string &filepath, const std::string &name) {

    Texture texture {};
    init::loadImageFromFile(mDevice, filepath.c_str(), texture.mImage);

    VkImageViewCreateInfo imageInfo = init::imageview_create_info(VK_FORMAT_R8G8B8A8_SRGB, texture.mImage.mImage,
                                                                  VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(mDevice->device(), &imageInfo, nullptr, &texture.mImageView);

    mTextures[name] = texture;

    mRootDisplay->addTexture(texture.mImageView, mTextureCount);

    mTextureToBinding[name] = mTextureCount;

    mTextureCount++;

    mDeletionQueue.push_function([=, this](){
        deleteTexture(name);
    });


    return &mTextures[name];
}

bool Vulkan::deleteTexture(const std::string &name) {
    //If it was implicitly deleted it potentially no longer be in the auto deletion queue
    if(!mTextures.contains(name)) return false;

    Texture &tex = mTextures[name];
    vmaDestroyImage(mDevice->allocator(), tex.mImage.mImage, tex.mImage.mAllocation);
    vkDestroyImageView(mDevice->device(), tex.mImageView, nullptr);

    uint32_t oldBinding = mTextureToBinding[name];
    mTextures.erase(name);
    mTextureToBinding.erase(name);

    //New free space at binding x
    std::string lastTexture = mBindingToTexture[mTextureCount - 1];
    //Upload last item to binding x
    mRootDisplay->addTexture(mTextures[lastTexture].mImageView, oldBinding);
    //Remove reference to old binding
    mTextureToBinding[lastTexture] = oldBinding;
    //Profit, idk how to clean up a unused descriptor so yea....

    //Make count smaller to account for the deleted texture
    mTextureCount--;

    return true;
}

void Vulkan::tick() {
    if (mRootDisplay->shouldExit()) {
        mRootDisplay.reset();
        mShouldExit = true;
    } else {
        draw();
    }
}

bool Vulkan::shouldExit() {return mShouldExit;}
