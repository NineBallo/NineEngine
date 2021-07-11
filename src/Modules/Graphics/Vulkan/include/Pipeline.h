//
// Created by nineball on 7/11/21.
//

#ifndef NINEENGINE_PIPELINE_H
#define NINEENGINE_PIPELINE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

class PipelineBuilder {
public:

    std::vector<VkPipelineShaderStageCreateInfo> mShaderStages;
    VkPipelineVertexInputStateCreateInfo mVertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo mInputAssembly;
    VkViewport mViewport;
    VkRect2D mScissor;
    VkPipelineRasterizationStateCreateInfo mRasterizer;
    VkPipelineColorBlendAttachmentState mColorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo mMultisampling;
    VkPipelineLayout mPipelineLayout;
    VkPipelineDynamicStateCreateInfo mDynamicState;

    VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);
};

namespace vkinit {
    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();
    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info(VkPrimitiveTopology topology);
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(VkPolygonMode polygonMode);
    VkPipelineMultisampleStateCreateInfo multisampling_state_create_info();
    VkPipelineColorBlendAttachmentState color_blend_attachment_state();
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info(VkDynamicState dynamicStates[]);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info();
}



#endif //NINEENGINE_PIPELINE_H
