//
// Created by nineball on 7/21/21.
//

#include "Textures.h"


#include "Initializers.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>

bool init::loadTextureFromFile(std::shared_ptr<NEDevice> device, const char *file, Texture &outTex) {

    int texWidth = 0, texHeight = 0, texChannels = 0;

    ///Load Texture from file into cpu memory
    stbi_uc* pixels = stbi_load(file, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    outTex.mMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

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


    VkExtent2D imageExtent;
    imageExtent.width = static_cast<uint32_t>(texWidth);
    imageExtent.height = static_cast<uint32_t>(texHeight);

    VkImageCreateInfo dimg_info = init::image_create_info(image_format, VK_IMAGE_USAGE_SAMPLED_BIT |
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, imageExtent, outTex.mMipLevels);


    VmaAllocationCreateInfo dimg_allocinfo = {};
    dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    //allocate and create the image
    vmaCreateImage(device->allocator(), &dimg_info, &dimg_allocinfo, &outTex.mImage.mImage, &outTex.mImage.mAllocation, nullptr);

    //Get texture to the gpu in an optimal format
    device->transitionImageLayout(outTex.mImage.mImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, outTex.mMipLevels);
    device->copyBufferToImage(stagingBuffer.mBuffer, outTex.mImage.mImage, imageExtent);
    device->generateMipmaps(outTex.mImage.mImage, VK_FORMAT_R8G8B8A8_SRGB, imageExtent, outTex.mMipLevels);

    //Generate MipMaps


    vmaDestroyBuffer(device->allocator(), stagingBuffer.mBuffer, stagingBuffer.mAllocation);

    return true;
}