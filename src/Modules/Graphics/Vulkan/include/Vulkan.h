//
// Created by nineball on 7/6/21.
//

#ifndef NINEENGINE_VULKAN_H
#define NINEENGINE_VULKAN_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <array>
#include <optional>
#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/optimizer.hpp>

#include "Device.h"
#include "Display.h"
#include "Types.h"
#include "Mesh.h"

#include "chrono"
//Im sure this breaks a few design patterns lol
#include "ECS.h"
#include "Textures.h"
#include "../shaders/Shaders.h"


//using texture = uint32_t;


class Vulkan : Module {
public:
    Vulkan(ECS &ecs, Entity cameraEntity);
    ~Vulkan();

    void init();
    void tick();

    bool shouldExit();

    Material* createMaterial(uint32_t features);
    void deleteMaterial(uint32_t features);

    void makeRenderable(Entity entity, uint32_t material, const std::string& mesh, TextureID* Textures = {}, std::string* textureIndex = {});

    void createMesh(const std::string& filepath, const std::string& meshName);
    bool deleteMesh(const std::string& meshName);

    Texture* getTexture(TextureID);
    TextureID loadTexture(const std::string& filepath, const std::string& name = "");
    void deleteTexture(TextureID texID);

    GLFWwindow* getWindow(Display display);

    ImTextureID textureId;
    bool firstRun = true;
private:

    void draw();
    void drawEntity(VkCommandBuffer cmd, Entity entity);
    void init_vulkan();

    bool debug = true;
    std::string mTitle = "NineEngine";

    void uploadMesh(Mesh& mesh);

private:
    ///Context
    VkInstance mInstance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;

    ///Primary/output device
    std::shared_ptr<NEDevice> mDevice;

    ///Primary/Root window && swapchain
    std::optional<NEDisplay> mRootDisplay;

    ///Additional displays that may be opened from the Primary/Root window
    std::array<std::optional<Display>, 10> mAdditionalDisplays;

private:
    //Only used for legacy descriptor backend
    VkSampler mSampler;

private:
    std::unordered_map<uint32_t, Material> mMaterials;
    std::unordered_map<std::string, MeshGroup> mMeshGroups;


    std::array<Texture, MAX_TEXTURES> mTextures;
    std::array<uint32_t, MAX_TEXTURES> mTextureToPos;
    std::array<TextureID, MAX_TEXTURES> mPosToTexture;
    uint32_t mTextureCount {0};
    std::queue<TextureID> mOldTextureIDs{};

private:
    ECS *mECS;

    std::array<std::array<Entity, MAX_ENTITYS>, MAX_DISPLAYS> mLocalEntityList;
    uint32_t mEntityListSize = 0;
    std::array<std::pair<Display, Entity>, MAX_ENTITYS> mEntityToPos;

    bool mShouldExit = false;

    Entity mCameraEntity = 0;



    ///Engine deletion queue
    DeletionQueue mDeletionQueue;
};

#endif //NINEENGINE_VULKAN_H
