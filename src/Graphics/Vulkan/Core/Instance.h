//
// Created by nineball on 4/16/21.
//

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

#ifndef NINEENGINE_INSTANCE_H
#define NINEENGINE_INSTANCE_H

namespace VKBareAPI::Instance {
    VkInstance createInstance(bool enableValidationLayers);
    void destroyInstance(VkInstance instance);
    ///Validation layers
    VkDebugUtilsMessengerEXT setupDebugMessenger(VkInstance instance);
    VkDebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfo();
    bool checkValidationLayerSupport();
    VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                          const VkAllocationCallbacks *pAllocator,
                                          VkDebugUtilsMessengerEXT *pDebugMessenger);
    void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                       const VkAllocationCallbacks *pAllocator);
    ///Sub/Helper functions
    std::vector<const char *> getRequiredExtensions(bool enableValidationLayers);
    VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                           VkDebugUtilsMessageTypeFlagsEXT messageType,
                           const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);
    }





#endif //NINEENGINE_INSTANCE_H
