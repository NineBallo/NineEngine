//
// Created by nineball on 1/4/21.
//

#ifndef NINEENGINE_GRAPHICSPIPELINE_H
#define NINEENGINE_GRAPHICSPIPELINE_H

#define GLFW_INCLUDE_VULKAN
#include "../vkGlobalPool.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include "vector"
#include "fstream"
#include "../vkGlobalPool.h"
#include "RenderPass.h"

class GraphicsPipeline {

public:
    GraphicsPipeline();
    ~GraphicsPipeline();

private:
    void createGraphicsPipeline();
    void createFrameBuffers();

private:
    static std::vector<char> readFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);

private:
    VkPipeline graphicsPipeline;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    VkPipelineLayout pipelineLayout;

private:
    RenderPass* renderPass;

};


#endif //NINEENGINE_GRAPHICSPIPELINE_H
