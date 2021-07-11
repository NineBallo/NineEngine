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

#include "Display.h"
#include "Types.h"


class Vulkan {
public:
    Vulkan();
    ~Vulkan();

    void init();
    void init_vulkan();
    void init_commands(device& _device);
    void init_default_renderpass();
    void init_pipelines();

    void draw();
    void tick();
private:
    bool debug = true;
    std::string mTitle = "NineEngine";

    void compileShader(std::string path);
    bool loadShaderModule(const char* filepath, VkShaderModule &outShaderModule);

private:
    ///Context
    VkInstance mInstance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;

    ///Primary/output device
    device mDevice;

    ///Primary/Root window && swapchain
    std::optional<Display> mRootDisplay;

    ///Additional displays that may be opened from the Primary/Root window
    std::array<std::optional<Display>, 10> mAdditionalDisplays;

    ///Pipeline
    VkPipelineLayout mTrianglePipelineLayout;
    VkPipeline mTrianglePipeline;
};


#endif //NINEENGINE_VULKAN_H
