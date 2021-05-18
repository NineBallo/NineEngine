//
// Created by nineball on 4/16/21.
//

#include "Renderpass.h"


namespace VKBareAPI::Pipeline::Renderpass{

    VkRenderPass createRenderPass(VkDevice device, VkFormat imageFormat) {
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;

        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;

        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


        ///The color attachment
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = imageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

        //Start of rendering -> _CLEAR clear contents, _LOAD keep existing content, _DONT_CARE undefined
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        //_STORE Rendered contents will stored in memory, _DONT_CARE will make the contents of framebuffer undefined
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        //Stencil stuff that I dont care about because im not touching the stencil buffer
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        //We just dont care about the previous frame
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        //Image is to be presented in the swapychain
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        ///Reference to color attachment
        VkAttachmentReference colorAttachmentRef{};
        //Only one VkAttachmentDescription so index is 0;
        colorAttachmentRef.attachment = 0;
        //Will just be the best performance when acting as a color buffer.
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        ///Subpass
        VkSubpassDescription subpass{};
        //Compute and raytracing subpasses are a thing...
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        //thing
        subpass.colorAttachmentCount = 1;
        //Just a ref to the color attachment
        subpass.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        VkRenderPass renderPass;
        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
        return renderPass;
    }

    void destroy(VkRenderPass renderPass, VkDevice device) {
        vkDestroyRenderPass(device, renderPass, nullptr);
    }
}