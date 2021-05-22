//
// Created by nineball on 5/19/21.
//



#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../Core/SharedStructs.h"

namespace VKExtraAPI {
    namespace Texture{
        void createTextureImage(VKBareAPI::Device::NEDevice &deviceVars);
        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                         VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VKBareAPI::Device::NEDevice &deviceVars);
        void createTextureImageView(VkImage &image, VkImageView &imageView, VkDevice device);
    }



}


