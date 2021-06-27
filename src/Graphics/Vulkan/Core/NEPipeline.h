//
// Created by nineball on 5/30/21.
//

#ifndef NINEENGINE_NEPIPELINE_H
#define NINEENGINE_NEPIPELINE_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>
#include "../../../Managers/ECS/Coordinator.h"
#include "NEDevice.h"

class NEDevice;

class NEPipeline {
public:
    NEPipeline(NEDevice *device, VkExtent2D extent, const std::string& vertShader, const std::string& fragShader);
    operator VkPipeline() const { return mPipeline; }

    NEPipeline(const NEPipeline&) = delete;
    NEPipeline& operator = (const NEPipeline&) = delete;


    VkPipelineLayout pipelineLayout();
    std::vector<VkDescriptorSet> descriptorSets();

private:
    void createPipeline(std::string vertShaderPath, std::string fragShaderPath);
    void createTextureSampler();
    void createDescriptorSetLayout();
    void createDescriptorSets(short size);

    static std::vector<char> readFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);
private:

    VkExtent2D mExtent;
    VkDevice mDevice = VK_NULL_HANDLE;
    VkRenderPass mRenderPass = VK_NULL_HANDLE;

    VkDescriptorPool mDescriptorPool;
    VkPhysicalDevice mPhysicalDevice;

    VkDescriptorSetLayout mDescriptorSetLayout;
    std::vector<VkDescriptorSet> mDescriptorSets;

    VkPipeline mPipeline;
    VkPipelineLayout mPipelineLayout;
    VkSampler mTextureSampler;
};




#endif //NINEENGINE_NEPIPELINE_H
