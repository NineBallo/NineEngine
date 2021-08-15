//
// Created by nineball on 7/11/21.
//

#include "Pipeline.h"
#include <iostream>

VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass) {
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;

    viewportState.viewportCount = 1;
    viewportState.pViewports = &mViewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &mScissor;

    //setup dummy color blending. We aren't using transparent objects yet
    //the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;

    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &mColorBlendAttachment;

    //Pipeline makin time
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;

    pipelineInfo.pDepthStencilState = &mDepthStencil;
    pipelineInfo.stageCount = mShaderStages.size();
    pipelineInfo.pStages = mShaderStages.data();
    pipelineInfo.pVertexInputState = &mVertexInputInfo;
    pipelineInfo.pInputAssemblyState = &mInputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &mRasterizer;
    pipelineInfo.pMultisampleState = &mMultisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &mDynamicState;
    pipelineInfo.layout = mPipelineLayout;
    pipelineInfo.renderPass = pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;


    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(
            device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
        std::cout << "Failed to create pipeline\n";
        return VK_NULL_HANDLE; // failed to create graphics pipeline
    }
    else
    {
        return newPipeline;
    }
}

VkPipelineShaderStageCreateInfo vkinit::pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule) {
    VkPipelineShaderStageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext = nullptr;

    //shader stage
    info.stage = stage;
    //module containing the code for this shader stage
    info.module = shaderModule;
    //the entry point of the shader
    info.pName = "main";
    return info;
}

VkPipelineVertexInputStateCreateInfo vkinit::vertex_input_state_create_info() {
    VkPipelineVertexInputStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info.pNext = nullptr;

    //no vertex bindings or attributes
    info.vertexBindingDescriptionCount = 0;
    info.vertexAttributeDescriptionCount = 0;
    return info;
}

VkPipelineInputAssemblyStateCreateInfo vkinit::input_assembly_create_info(VkPrimitiveTopology topology) {
    VkPipelineInputAssemblyStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info.pNext = nullptr;

    info.topology = topology;
    //we are not going to use primitive restart on the entire tutorial so leave it on false
    info.primitiveRestartEnable = VK_FALSE;
    return info;
}

VkPipelineRasterizationStateCreateInfo vkinit::rasterization_state_create_info(VkPolygonMode polygonMode) {
    VkPipelineRasterizationStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info.pNext = nullptr;

    info.depthClampEnable = VK_FALSE;
    //discards all primitives before the rasterization stage if enabled which we don't want
    info.rasterizerDiscardEnable = VK_FALSE;

    info.polygonMode = polygonMode;
    info.lineWidth = 1.0f;
    //no backface cull
    info.cullMode = VK_CULL_MODE_BACK_BIT;
    info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    //no depth bias
    info.depthBiasEnable = VK_FALSE;
    info.depthBiasConstantFactor = 0.0f;
    info.depthBiasClamp = 0.0f;
    info.depthBiasSlopeFactor = 0.0f;

    return info;
}

VkPipelineMultisampleStateCreateInfo vkinit::multisampling_state_create_info(VkSampleCountFlagBits samples, VkBool32 sampleShading) {
    VkPipelineMultisampleStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info.pNext = nullptr;

    info.sampleShadingEnable = sampleShading;
    //multisampling defaulted to no multisampling (1 sample per pixel)
    info.rasterizationSamples = samples;
    info.minSampleShading = 1.0f;
    info.pSampleMask = nullptr;
    info.alphaToCoverageEnable = VK_FALSE;
    info.alphaToOneEnable = VK_FALSE;
    return info;
}

VkPipelineColorBlendAttachmentState vkinit::color_blend_attachment_state() {
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    return colorBlendAttachment;
}

VkPipelineLayoutCreateInfo vkinit::pipeline_layout_create_info() {
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext = nullptr;

    //empty defaults
    info.flags = 0;
    info.setLayoutCount = 0;
    info.pSetLayouts = nullptr;
    info.pushConstantRangeCount = 0;
    info.pPushConstantRanges = nullptr;
    return info;
}

VkPipelineDynamicStateCreateInfo vkinit::dynamic_state_create_info(VkDynamicState dynamicStates[]) {
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = 2;
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;
    return dynamicStateCreateInfo;
}