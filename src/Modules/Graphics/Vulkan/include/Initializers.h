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

    VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
    VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

    VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp);

    VkWriteDescriptorSet  writeDescriptorBuffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding);
}



#endif //NINEENGINE_INITIALIZERS_H
