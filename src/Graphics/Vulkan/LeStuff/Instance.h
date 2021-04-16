//
// Created by nineball on 4/16/21.
//

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

#ifndef NINEENGINE_INSTANCE_H
#define NINEENGINE_INSTANCE_H

namespace Instance{


    VkInstance createInstance(int debugMessanger);
    bool destroyInstance(VkInstance instance);

    ///Validation layers
    VkDebugUtilsMessengerEXT setupDebugMessenger();
    VkDebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfo();
    bool checkValidationLayerSupport();
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                          const VkAllocationCallbacks *pAllocator,
                                          VkDebugUtilsMessengerEXT *pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                    const VkAllocationCallbacks *pAllocator);


    ///Sub/Helper functions
    std::vector<const char *> getRequiredExtensions();
    VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                           VkDebugUtilsMessageTypeFlagsEXT messageType,
                           const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);



    ///Variables and structs i guess
    std::vector<const char *> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };

}




#endif //NINEENGINE_INSTANCE_H
