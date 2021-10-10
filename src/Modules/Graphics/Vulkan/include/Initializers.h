//
// Created by nineball on 7/7/21.
//

#ifndef NINEENGINE_INITIALIZERS_H
#define NINEENGINE_INITIALIZERS_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace init {
    VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
    VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent2D extent, uint32_t mipLevels, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);
    VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, uint32_t mipLevels, VkImageAspectFlags aspectFlags);

    VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp);

    VkWriteDescriptorSet  writeDescriptorBuffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding);

    VkFenceCreateInfo fenceCreateInfo();
    VkSemaphoreCreateInfo semaphoreCreateInfo();

    VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags);
    VkSubmitInfo submitInfo(VkCommandBuffer* cmd, size_t size);


    VkSamplerCreateInfo samplerCreateInfo(VkFilter filters, float anisotropy, uint32_t mipLevels, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
    VkWriteDescriptorSet writeDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding);

    VkDescriptorImageInfo descriptorImageInfo(VkSampler sampler, VkImageView imageView);

    VkAttachmentDescription attachmentDescription(VkFormat format, VkSampleCountFlagBits samples,
                                                  VkImageLayout initialLayout, VkImageLayout finalLayout,
                                                  VkAttachmentStoreOp storeOp, VkAttachmentStoreOp stenStoreOp,
                                                  VkAttachmentLoadOp loadOp, VkAttachmentLoadOp stenLoadOp, VkFlags flags = 0);
}



#endif //NINEENGINE_INITIALIZERS_H
