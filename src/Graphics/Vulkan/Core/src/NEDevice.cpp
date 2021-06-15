//
// Created by nineball on 5/29/21.
//

#include "../NEDevice.h"
#include <iostream>
#include <string>
#include <set>
#include <stb_image.h>
#include <cstring>
#include "../NEShared.h"
#include "../../Helper/Device.h"
#include "../../Helper/Display.h"

NEDevice::NEDevice(std::shared_ptr<NEInstance>& instance_) {
    display = &vkContext.displays[0];
    pickPhysicalDevice();
    createLogicalDevice();
}

NEDevice::~NEDevice() {
    vkDestroyDevice(device, nullptr);
}


void NEDevice::createBuffers() {
    createCommandPool();
    createDescriptorPool(display->getFramebufferSize());
    createUniformBuffers();

    createTextureImage(vkContext.defaultImage, vkContext.defaultImageMemory, "Textures/image0.jpg");
    createTextureImageView(vkContext.defaultImage, vkContext.defaultImageView);

    renderpass = std::make_shared<NERenderpass>(device, VK_FORMAT_B8G8R8A8_SRGB);
    pipeline = std::make_shared<NEPipeline>();
}

void NEDevice::pickPhysicalDevice() {
    ///Get all devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkContext.instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with vulkan support. LMAO, loser.");
    } else {
        std::cout << deviceCount << " Device(s) Found!\n";
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vkContext.instance, &deviceCount, devices.data());

    ///Check if device can be used
    for (const auto &candidate : devices) {
        std::cout << candidate << std::endl;
        if (isDeviceSuitable(candidate, display->getSurface())) {
            physicalDevice = candidate;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

void NEDevice::createLogicalDevice() {
    //Filling in vulkan info to create the Present and Graphics queues
    queueFamilys = findQueueFamilies(physicalDevice, display->getSurface());
    swapChainSupportDetails = querySwapChainSupport(physicalDevice, display->getSurface());

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {queueFamilys.graphicsFamily.value(), queueFamilys.presentFamily.value()};

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};

        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        //The most interesting option here, I wonder if multiple queues have a perf/latency impact.
        queueCreateInfo.queueCount = 1;

        //0.0f-1.0f range for queue priority
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;


    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    //Reads da vector for the queues to create
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    //Extra jazz im assuming, DLSS and the like.
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());;
    createInfo.ppEnabledExtensionNames = &deviceExtensions[0];

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = &validationLayers[0];
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
    else {
        vkGetDeviceQueue(device, queueFamilys.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, queueFamilys.presentFamily.value(), 0, &presentQueue);
    }
}

void NEDevice::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {

}

void NEDevice::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout,
                                     VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
    );

    endSingleTimeCommands(commandBuffer);
}

void NEDevice::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
            width,
            height,
            1
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer NEDevice::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void NEDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

VkBuffer NEDevice::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                                VkBuffer &buffer, VkDeviceMemory &bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}



void NEDevice::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilys.graphicsFamily.value();
    poolInfo.flags = 0; // Optional

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
}

void NEDevice::createDescriptorPool(short size) {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(size);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(size);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());;
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(size);

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}


void NEDevice::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                           VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image,
                           VkDeviceMemory &imageMemory) {
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

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

void NEDevice::createTextureImage(VkImage &textureImage, VkDeviceMemory &textureImageMemory, const std::string& texPath) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(texPath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("Failed to load texture image!");
    }
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);
    stbi_image_free(pixels);

    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                textureImage, textureImageMemory);
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth),
                                          static_cast<uint32_t>(texHeight));
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void NEDevice::createTextureImageView(VkImage &image, VkImageView &imageView) {
    imageView = createImageView(image, VK_FORMAT_R8G8B8A8_SRGB, device);
}

void NEDevice::createVertexBuffers(VkDeviceMemory &vertexBufferMemory, VkBuffer &vertexBuffer, const std::vector<Vertex>& vertices) {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void NEDevice::createIndexBuffers(VkDeviceMemory &indexBufferMemory, VkBuffer &indexBuffer, std::vector<uint16_t> indexes) {
    VkDeviceSize bufferSize = sizeof(indexes[0]) * indexes.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indexes.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer,indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void NEDevice::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    size_t size = display->getFramebufferSize();

    vkContext.uniformBuffers.resize(size);
    vkContext.uniformBuffersMemory.resize(size);

    for (size_t i = 0; i < size; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             vkContext.uniformBuffers[i], vkContext.uniformBuffersMemory[i]);
    }

}
void NEDevice::submitCommandBuffer(VkCommandBuffer commandBuffer) {
    //Buffers::updateUniformBuffer(imageIndex, deviceVars, swapchainVars);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {display->getImageAvailableSemaphores()[display->getFrame()]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkSemaphore signalSemaphores[] = {display->getRenderFinishedSemaphores()[display->getFrame()]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;



    ///Submit graphics queue and continue if successful
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, display->getInFlightFences()[display->getFrame()]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }
}

NERenderpass* NEDevice::getRenderpass() {return renderpass.get();}
NEPipeline * NEDevice::getPipeline() {return pipeline.get();}

VkPhysicalDevice NEDevice::getGPU() {return physicalDevice;}
VkQueue NEDevice::getPresentQueue(){return presentQueue;};
SwapChainSupportDetails NEDevice::getSupportDetails() {return swapChainSupportDetails;};
QueueFamilyIndices NEDevice::getQueueFamilys() {return queueFamilys;};
VkDescriptorPool NEDevice::getDescriptorPool(){ return descriptorPool;};
VkCommandPool NEDevice::getCommandPool() {return commandPool;}