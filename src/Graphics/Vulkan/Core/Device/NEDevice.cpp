//
// Created by nineball on 5/29/21.
//

#include "NEDevice.h"
#include <iostream>
#include <string>
#include <set>

namespace NEVK {
    NEDevice::NEDevice(std::shared_ptr<NEInstance>& instance_) : instance{instance_} {
        createWindow();
        pickPhysicalDevice();
        createLogicalDevice();
        window->createSwapchain(this);
        renderpass = std::make_shared<NERenderpass>(device, format);
        pipeline = std::make_shared<NEPipeline>(this, renderpass, window);

    }

    NEDevice::~NEDevice() {
        vkDestroyDevice(device, nullptr);
    }



    void NEDevice::createWindow() {
        window = std::make_shared<NEWindow>(900, 600, "gooba googa", false, *instance);
        window->createSurface();
    }


    void NEDevice::pickPhysicalDevice() {
        ///Get all devices
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(*instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("Failed to find GPUs with vulkan support. LMAO, loser.");
        } else {
            std::cout << deviceCount << " Device(s) Found!\n";
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(*instance, &deviceCount, devices.data());

        ///Check if device can be used
        for (const auto &candidate : devices) {
            std::cout << candidate << std::endl;
            if (isDeviceSuitable(candidate)) {
                physicalDevice = candidate;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }
    }

    bool NEDevice::isDeviceSuitable(VkPhysicalDevice candidate) {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(candidate);

        ///Can it even have video out, etc...
        bool extensionsSupported = checkDeviceExtensionSupport(candidate);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails supportDetails = querySwapChainSupport(candidate);
            swapChainAdequate = !supportDetails.formats.empty() && !supportDetails.presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(candidate, &supportedFeatures);

        ///Does it have a value?
        return queueFamilyIndices.graphicsFamily.has_value() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;;
    }

   SwapChainSupportDetails NEDevice::querySwapChainSupport(VkPhysicalDevice candidate) {
       SwapChainSupportDetails supportDetails;

       ///Thingy that gets surface capabilities
       vkGetPhysicalDeviceSurfaceCapabilitiesKHR(candidate, window->getSurface(), &supportDetails.capabilities);


       ///Get supported surface formats
       uint32_t formatCount = 0;
       vkGetPhysicalDeviceSurfaceFormatsKHR(candidate, window->getSurface(), &formatCount, nullptr);
       if (formatCount != 0) {
           supportDetails.formats.resize(formatCount);
           vkGetPhysicalDeviceSurfaceFormatsKHR(candidate, window->getSurface(), &formatCount, supportDetails.formats.data());
       }

       ///Get supported resent formats
       uint32_t presentModeCount = 0;
       vkGetPhysicalDeviceSurfacePresentModesKHR(candidate, window->getSurface(), &presentModeCount, nullptr);
       if (presentModeCount != 0) {
           supportDetails.presentModes.resize(presentModeCount);
           vkGetPhysicalDeviceSurfacePresentModesKHR(candidate, window->getSurface(), &presentModeCount, supportDetails.presentModes.data());
       }

       return supportDetails;
    }

    void NEDevice::createLogicalDevice() {
        //Filling in vulkan info to create the Present and Graphics queues
        indices = findQueueFamilies(physicalDevice);
        swapChainSupportDetails = querySwapChainSupport(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        for (uint32_t QueueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};

            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
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
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    }

    QueueFamilyIndices NEDevice::findQueueFamilies(VkPhysicalDevice candidate) {

        QueueFamilyIndices queryIndices;

        ///Get device queue properties
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueFamilyCount, queueFamilies.data());

        ///Find a queue family that supports VK_QUEUE_GRAPHICS_BIT.
        int i = 0;
        for (const auto &queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                queryIndices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, window->getSurface(), &presentSupport);

            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                queryIndices.presentFamily = i;
            }
            if (queryIndices.isComplete()) {
                break;
            }
            i++;
        }
        return queryIndices;
    }

    bool NEDevice::checkDeviceExtensionSupport(VkPhysicalDevice candidate) {
        //Get all extensions
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, availableExtensions.data());

        //Get length of extension list.
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        //If the extension does exist it will erase from the required extensions;
        for (const auto &extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        //Return true if empty/all extensions supported.
        return requiredExtensions.empty();
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
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    uint32_t NEDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    SwapChainSupportDetails NEDevice::getSwapChainDetails() {return swapChainSupportDetails;}
    NEWindow * NEDevice::getWindow() {return window.get();}
    VkPhysicalDevice NEDevice::getPhysicalDevice() {return physicalDevice;}
    QueueFamilyIndices NEDevice::getIndices() {return indices;}
    void NEDevice::setFormat(VkFormat format_) {format = format_;}
    NERenderpass* NEDevice::getRenderpass() {return renderpass.get();}
    NEPipeline * NEDevice::getPipeline() {return pipeline.get();}
}