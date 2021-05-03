//
// Created by nineball on 5/3/21.
//

#ifndef NINEENGINE_SHAREDSTRUCTS_H
#define NINEENGINE_SHAREDSTRUCTS_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <array>
#include <set>


namespace Device {

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    const std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };



    struct DeviceQueues {
        VkQueue GraphicsQueue;
        VkQueue PresentQueue;
        VkDevice device;
        VkPhysicalDevice physicalDevice;
    };
}
#endif //NINEENGINE_SHAREDSTRUCTS_H
