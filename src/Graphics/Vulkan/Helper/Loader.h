//
// Created by nineball on 6/14/21.
//

#ifndef NINEENGINE_LOADER_H
#define NINEENGINE_LOADER_H

void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                 VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
void createTextureImage(VkImage& textureImage, VkDeviceMemory& textureImageMemory, const std::string& texPath);
void createTextureImageView(VkImage &image, VkImageView &imageView);


#endif //NINEENGINE_LOADER_H
