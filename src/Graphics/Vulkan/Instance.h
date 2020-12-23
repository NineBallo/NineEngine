//
// Created by nineball on 12/23/20.
//

#ifndef NINEENGINE_INSTANCE_H
#define NINEENGINE_INSTANCE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <optional>
#include <any>

///TODO split logging and vulkan initialization

class Instance {
public:
    Instance();
    ~Instance();

private:
    std::vector<const char*> getRequiredExtensions();
    void createInstance();
    bool checkValidationLayerSupport();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void setupDebugMessenger();
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void* pUserData);
private:
    ///Graphics misc
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    ///Validation layer stuff
    const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };

    #ifdef NODEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif

};


#endif //NINEENGINE_INSTANCE_H
