//
// Created by nineball on 5/19/21.
//

#include "Loader.h"
#include <iostream>
#include "../Core/Buffers.h"
#include "../Core/Swapchain.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


namespace VKExtraAPI::Texture {

    void createTextureImage(VKBareAPI::Device::NEDevice &deviceVars) {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load("Textures/image0.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("Failed to load texture image!");
        }
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        VKBareAPI::Buffers::createBuffer(imageSize, deviceVars, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
        void* data;
        vkMapMemory(deviceVars.device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(deviceVars.device, stagingBufferMemory);

        stbi_image_free(pixels);



        createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    deviceVars.Buffers.textureImage, deviceVars.Buffers.textureImageMemory, deviceVars);


        VKBareAPI::Buffers::transitionImageLayout(deviceVars.Buffers.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, deviceVars.commandPool,
                                                  deviceVars.device, deviceVars.graphicsQueue);
        VKBareAPI::Buffers::copyBufferToImage(stagingBuffer, deviceVars.Buffers.textureImage, static_cast<uint32_t>(texWidth),
                                              static_cast<uint32_t>(texHeight), deviceVars.commandPool, deviceVars.device,
                                              deviceVars.graphicsQueue);
        VKBareAPI::Buffers::transitionImageLayout(deviceVars.Buffers.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, deviceVars.commandPool,
                                                  deviceVars.device, deviceVars.graphicsQueue);

        vkDestroyBuffer(deviceVars.device, stagingBuffer, nullptr);
        vkFreeMemory(deviceVars.device, stagingBufferMemory, nullptr);
    }

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VKBareAPI::Device::NEDevice &deviceVars) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;

        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0; // Optional

        if (vkCreateImage(deviceVars.device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(deviceVars.device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = VKBareAPI::Buffers::findMemoryType(memRequirements.memoryTypeBits, deviceVars.physicalDevice, properties);

        if (vkAllocateMemory(deviceVars.device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(deviceVars.device, image, imageMemory, 0);

    }

    void createTextureImageView(VkImage &image, VkImageView &imageView, VkDevice device) {
        imageView = VKBareAPI::Swapchain::createImageView(image, VK_FORMAT_R8G8B8A8_SRGB, device);
    }

}

