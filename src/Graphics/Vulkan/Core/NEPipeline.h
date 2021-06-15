//
// Created by nineball on 5/30/21.
//

#ifndef NINEENGINE_NEPIPELINE_H
#define NINEENGINE_NEPIPELINE_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>
#include "../../../Managers/Coordinator.h"

class NEPipeline {
public:
    NEPipeline();
    operator VkPipeline() const { return pipeline; }

    VkPipelineLayout getPipelineLayout() {return pipelineLayout;}
    void recreateFrameBuffers();
    std::vector<VkDescriptorSet> getDescriptorSets();

private:
    void createPipeline();
    void createTextureSampler();
    void createDescriptorSetLayout();
    void createDescriptorSets(short size);

    static std::vector<char> readFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);
private:
    uint32_t deviceIndex = 0;
    uint32_t displayIndex = 0;
    VkDevice device = VK_NULL_HANDLE;

    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkSampler textureSampler;
};




#endif //NINEENGINE_NEPIPELINE_H
