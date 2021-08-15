//
// Created by nineball on 7/7/21.
//
#include "Initializers.h"


VkCommandPoolCreateInfo init::command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext = nullptr;

    info.queueFamilyIndex = queueFamilyIndex;
    info.flags = flags;
    return info;
}

VkCommandBufferAllocateInfo init::command_buffer_allocate_info(VkCommandPool pool, uint32_t count,
                                                               VkCommandBufferLevel level) {
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext = nullptr;

    info.commandPool = pool;
    info.commandBufferCount = count;
    info.level = level;
    return info;
}

VkImageCreateInfo init::image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent2D extent, uint32_t mipLevels, VkSampleCountFlagBits sampleCount) {
    VkImageCreateInfo info = { };
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext = nullptr;

    info.imageType = VK_IMAGE_TYPE_2D;

    info.format = format;
    info.extent = {
            extent.width,
            extent.height,
            1
    };

    info.mipLevels = mipLevels;
    info.arrayLayers = 1;
    info.samples = sampleCount;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usageFlags;

    return info;
}

VkImageViewCreateInfo init::imageview_create_info(VkFormat format, VkImage image, uint32_t mipLevels, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;

    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.image = image;
    info.format = format;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = mipLevels;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.aspectMask = aspectFlags;

    return info;
}


VkPipelineDepthStencilStateCreateInfo init::depth_stencil_create_info(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp)
{
    VkPipelineDepthStencilStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    info.pNext = nullptr;

    info.depthTestEnable = bDepthTest ? VK_TRUE : VK_FALSE;
    info.depthWriteEnable = bDepthWrite ? VK_TRUE : VK_FALSE;
    info.depthCompareOp = bDepthTest ? compareOp : VK_COMPARE_OP_ALWAYS;
    info.depthBoundsTestEnable = VK_FALSE;
    info.minDepthBounds = 0.0f; // Optional
    info.maxDepthBounds = 1.0f; // Optional
    info.stencilTestEnable = VK_FALSE;

    return info;
}

VkWriteDescriptorSet init::writeDescriptorBuffer(VkDescriptorType type, VkDescriptorSet dstSet,
                                                 VkDescriptorBufferInfo *bufferInfo, uint32_t binding) {
    VkWriteDescriptorSet setWrite = {};
    setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    setWrite.pNext = nullptr;

    //We are going to write into binding number 0
    setWrite.dstBinding = binding;
    //of the global descriptor
    setWrite.dstSet = dstSet;

    setWrite.descriptorCount = 1;
    //and the type is uniform buffer
    setWrite.descriptorType = type;
    setWrite.pBufferInfo = bufferInfo;

    return setWrite;
}

VkFenceCreateInfo init::fenceCreateInfo() {
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = nullptr;
    return fenceCreateInfo;
}

VkSemaphoreCreateInfo init::semaphoreCreateInfo() {
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;
    return semaphoreCreateInfo;
}

VkCommandBufferBeginInfo init::commandBufferBeginInfo(VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo cmdBeginInfo = {};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;
    cmdBeginInfo.pInheritanceInfo = nullptr;
    cmdBeginInfo.flags = flags;
    return cmdBeginInfo;
}

VkSubmitInfo init::submitInfo(VkCommandBuffer* cmd, size_t size) {

   VkSubmitInfo info {};
   info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   info.pNext = nullptr;

   info.waitSemaphoreCount = 0;
   info.pWaitSemaphores = nullptr;
   info.pWaitDstStageMask = nullptr;
   info.commandBufferCount = size;
   info.pCommandBuffers = cmd;
   info.signalSemaphoreCount = 0;
   info.pSignalSemaphores = nullptr;

   return info;
}

VkSamplerCreateInfo init::samplerCreateInfo(VkFilter filters, float anisotropy, uint32_t mipLevels, VkSamplerAddressMode samplerAddressMode) {
    VkSamplerCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.pNext = nullptr;

    info.magFilter = filters;
    info.minFilter = filters;
    info.addressModeU = samplerAddressMode;
    info.addressModeV = samplerAddressMode;
    info.addressModeW = samplerAddressMode;

    if(mipLevels >= 1) {
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.maxLod = static_cast<float>(mipLevels);
        info.minLod = 0.0f; //op
        info.mipLodBias = 0.0f; //op
    }

    if(anisotropy > 1) {
        info.anisotropyEnable = VK_TRUE;
        info.maxAnisotropy = anisotropy;
    }

    return info;
}

VkWriteDescriptorSet init::writeDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet,
                                                VkDescriptorImageInfo *imageInfo, uint32_t binding) {
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;

    write.dstBinding = binding;
    write.dstSet = dstSet;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = imageInfo;

    return write;
}

VkDescriptorImageInfo init::descriptorImageInfo(VkSampler sampler, VkImageView imageView) {
    VkDescriptorImageInfo imageBufferInfo;
    imageBufferInfo.sampler = sampler;
    imageBufferInfo.imageView = imageView;
    imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return imageBufferInfo;
}

VkDescriptorSetLayoutBinding init::createDescriptorSetBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding) {

    //information about the binding.
    VkDescriptorSetLayoutBinding setBind = {};
    setBind.binding = binding;
    setBind.descriptorCount = 1;
    // it's a uniform buffer binding
    setBind.descriptorType = type;

    // we use it from the vertex shader
    setBind.stageFlags = stageFlags;

    return setBind;

}