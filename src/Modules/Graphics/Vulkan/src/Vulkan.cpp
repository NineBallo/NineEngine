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

Vulkan::Vulkan(Entity cameraEntity) : mECS{ECS::Get()} {

    ECS::Get().registerComponent<RenderObject>();
    ECS::Get().registerComponent<Position>();

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
    mDisplays[0] = std::make_shared<NEDisplay>(createInfo);
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

    mECS.addComponent<RenderObject>(entity, renderObject);
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
        mDisplays[0]->drawframe();
    }
}

bool Vulkan::shouldExit() {return mShouldExit;}
    void draw();
    void drawEntity(VkCommandBuffer cmd, Entity entity);