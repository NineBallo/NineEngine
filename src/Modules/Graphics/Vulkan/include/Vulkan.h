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

class Vulkan : Module {
public:
    Vulkan(Entity cameraEntity);
    ~Vulkan();

    void init();
    void tick();

    bool shouldExit();

    void makeRenderable(Entity entity, const std::string &mesh, std::vector<TextureID> Textures = {}, std::vector<std::string> textureIndex = {});

    void createMesh(const std::string& filepath, const std::string& meshName);
    bool deleteMesh(const std::string& meshName);

    TextureID loadTexture(const std::string& filepath, const std::string& name = "");
    void addTextureToDisplay(TextureID texID);

    GLFWwindow* getWindow(Display display);

    ImTextureID textureId;
    bool firstRun = true;
private:
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

    ///Displays may be opened from the Primary/Root window, root is at idx 0
    std::array<std::optional<NEDisplay>, 10> mDisplays;

private:
    //Only used for legacy descriptor backend
    VkSampler mSampler;

private:
    std::unordered_map<std::string, MeshGroup> mMeshGroups;

    ECS& mECS;
private:
    bool mShouldExit = false;

    ///Engine deletion queue
    DeletionQueue mDeletionQueue;
};

#endif //NINEENGINE_VULKAN_H
