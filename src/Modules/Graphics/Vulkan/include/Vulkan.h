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
#include <chrono>

#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/optimizer.hpp>

#include "Device.h"
#include "Display.h"
#include "Types.h"
#include "Mesh.h"
#include "ECS.h"
#include "Textures.h"
#include "../shaders/Shaders.h"

class Vulkan {
public:
    Vulkan(ECS &ecs, Entity cameraEntity);
    ~Vulkan();

    void init();
    void tick();

    bool shouldExit();

    Material* createMaterial(const std::string& name, uint32_t features, glm::vec3 color = {});
    void deleteMaterial(const std::string& name);

    void makeRenderable(Entity entity, std::string material, const std::string& mesh, std::string* Textures = {}, std::string* textureIndex = {});

    void createMesh(const std::string& filepath, const std::string& meshName);
    bool deleteMesh(const std::string& meshName);

    Texture* loadTexture(const std::string& filepath, const std::string& name);
    auto deleteTexture(const std::string& name);

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
    bool mShouldExit = false;

    ///Vulkan deletion queue
    DeletionQueue mDeletionQueue;
};

#endif //NINEENGINE_VULKAN_H
