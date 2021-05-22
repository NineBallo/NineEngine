//
// Created by nineball on 4/16/21.
//
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <fstream>

#include "Renderpass.h"
#include "SharedStructs.h"

namespace VKBareAPI::Pipeline {
    void destroy(NEPipeline &pipelineVars, Device::NEDevice &deviceVars, VKBareAPI::Swapchain::NESwapchain& swapvars);
    VkPipeline create(NEPipeline &pipelineVars, Device::NEDevice &deviceVars, VKBareAPI::Swapchain::NESwapchain& swapvars);

    void createPipeline(NEPipeline &pipelineVars, VkDevice device, VKBareAPI::Swapchain::NESwapchain& swapvars);
    void createFrameBuffers(NEPipeline &pipelineVars, VkDevice device, VKBareAPI::Swapchain::NESwapchain& swapvars);
    static std::vector<char> readFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);

    void createDescriptorSetLayout(VkDevice device, NEPipeline &pipelineVars);

    void createTextureSampler(VkSampler &textureSampler, VkDevice device, VkPhysicalDevice physicalDevice);
}

