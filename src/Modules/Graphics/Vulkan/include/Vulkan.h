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

    void tick();

    bool shouldExit();

    void addDisplay(Display display);
    void removeDisplay(Display display);


    GLFWwindow* getWindow(Display display);

    ImTextureID textureId;
    bool firstRun = true;


private:
    bool debug = true;
    std::string mTitle = "NineEngine";


private:
    ///Context
    VkInstance mInstance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;


    ///Primary device
    std::shared_ptr<NEDevice> mDevice;


    ///Primary/Root window is at index [0]
    ///All displays that may be opened including the Primary/Root window
    std::array<std::optional<Display>, MAX_DISPLAYS> mDisplays;
    std::array<Display, MAX_DISPLAYS> mPosToDisplay;
    std::array<uint8_t, MAX_DISPLAYS> mDisplayToPos;
    uint8_t mDisplayCount;


private:
    //Only used for legacy descriptor backend
    VkSampler mSampler;


private:
    bool mShouldExit = false;
    ECS& mECS;
    ///Vulkan deletion queue
    DeletionQueue mDeletionQueue;
};

#endif //NINEENGINE_VULKAN_H
