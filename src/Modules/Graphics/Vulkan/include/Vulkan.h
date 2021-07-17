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

//Im sure this breaks a few design patterns lol
#include "../../../../Managers/ECS/ECS.h"

struct Position {
    glm::vec3 Pos {};
    float scalar = 0;
};

struct Camera {
    //x (left to right), y (up down), z (forwards and back)
    glm::vec3 Pos {0, 0, 0};
    glm::vec3 Angle {0, 0,0};
    float degrees {70.f};
    float aspect {800.f / 600.f};
    float znear {0.1f};
    float zfar {200.0f};
};

struct VKRender {
    Mesh mesh;

    int pipeline = 0;
};



class Vulkan : Module {
public:
    Vulkan(ECS &ecs, Entity cameraEntity);
    ~Vulkan();

    void init();
    void tick();

    void loadMesh(std::string filepath, uint32_t entity);

    bool shouldExit();

    GLFWwindow* getWindow(Display display);
private:

    void draw();
    void init_vulkan();
    void init_commands(device& _device);
    void init_default_renderpass();
    void init_pipelines();


    bool debug = true;
    std::string mTitle = "NineEngine";

    void compileShader(std::string path);
    bool loadShaderModule(const char* filepath, VkShaderModule &outShaderModule);

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

    ///Pipeline
    VkPipelineLayout mTrianglePipelineLayout;
    VkPipeline mMeshPipeline = VK_NULL_HANDLE;


private:
    ECS *mECS;

    bool mShouldExit = false;

    Entity mCameraEntity = 0;

    ///Engine deletion queue
    DeletionQueue mMainDeletionQueue;
};

#endif //NINEENGINE_VULKAN_H
