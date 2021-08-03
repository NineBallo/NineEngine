//
// Created by nineball on 7/21/21.
//

#include "Textures.h"


#include "Initializers.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>

bool init::loadImageFromFile(std::shared_ptr<NEDevice> device, const char *file, AllocatedImage &outImage) {

    int texWidth = 0, texHeight = 0, texChannels = 0;

    ///Load Texture from file into cpu memory
    stbi_uc* pixels = stbi_load(file, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if(!pixels) {
        std::cout << "Failed to load texture file" << file << std::endl;
        return false;
    }

    ///Load texture into cpu buffer
    void* pixel_ptr = pixels;
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    //the format R8G8B8A8 matches exactly with the pixels loaded from stb_image lib
    VkFormat image_format = VK_FORMAT_R8G8B8A8_SRGB;

    //allocate temporary buffer for holding texture data to upload
    AllocatedBuffer stagingBuffer = device->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    //copy data to buffer
    void* data;
    vmaMapMemory(device->allocator(), stagingBuffer.mAllocation, &data);

    memcpy(data, pixel_ptr, static_cast<size_t>(imageSize));

    vmaUnmapMemory(device->allocator(), stagingBuffer.mAllocation);

    ///Delete copy now that we already have it in the cpu buffer
    stbi_image_free(pixels);


    VkExtent3D imageExtent;
    imageExtent.width = static_cast<uint32_t>(texWidth);
    imageExtent.height = static_cast<uint32_t>(texHeight);
    imageExtent.depth = 1;

    VkImageCreateInfo dimg_info = init::image_create_info(image_format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);

    AllocatedImage newImage {};

    VmaAllocationCreateInfo dimg_allocinfo = {};
    dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    //allocate and create the image
    vmaCreateImage(device->allocator(), &dimg_info, &dimg_allocinfo, &newImage.mImage, &newImage.mAllocation, nullptr);

    ///Transition image and copy to GPU buffer
    device->immediateSubmit([=](VkCommandBuffer cmd) {
        //Transition
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        VkImageMemoryBarrier imageBarrier_toTransfer = {};
        imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

        imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toTransfer.image = newImage.mImage;
        imageBarrier_toTransfer.subresourceRange = range;

        imageBarrier_toTransfer.srcAccessMask = 0;
        imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        //barrier the image into the transfer-receive layout
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);


        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = imageExtent;

        //copy the buffer into the image
        vkCmdCopyBufferToImage(cmd, stagingBuffer.mBuffer, newImage.mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &copyRegion);



        VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;

        imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        //barrier the image into the shader readable layout
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0,
                             nullptr, 1, &imageBarrier_toReadable);
    });

    vmaDestroyBuffer(device->allocator(), stagingBuffer.mBuffer, stagingBuffer.mAllocation);

    outImage = newImage;
    return true;
}