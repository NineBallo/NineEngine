//
// Created by nineball on 5/29/21.
//

#include "../NEInstance.h"
#include <iostream>
#include <cstring>


const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

NEInstance::NEInstance(bool enableValidationLayers) {
    createInstance(enableValidationLayers);
    setupDebugMessenger();
}
NEInstance::~NEInstance() {
    vkDestroyInstance(mInstance, nullptr);
    destroyDebugUtilsMessengerEXT(mDebugMessenger, nullptr);
}

void NEInstance::createInstance(bool enableValidationLayers) {
    ///Da layers, gotta have em u kno || fail if validation layers are defined but not supported
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    ///Filling in a struct with some information that might make the final application run better cause driver optimizations
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "No Name";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    ///Passing the information to another struct
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    ///Getting the available extensions through glfw to pass to the Vkinstance
    auto extensions = getRequiredExtensions(enableValidationLayers);
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = &extensions[0];

    ///logging for pre-initialization
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = &validationLayers[0];

        debugCreateInfo = populateDebugMessengerCreateInfo();
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
    } else {

        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    ///haha error check go brrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr
    if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance");
    } else {
        std::cout << "Vulkan instance probably successfully created.\n";
    }
}

void NEInstance::setupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    createInfo = populateDebugMessengerCreateInfo();

    ///do the thing (create the extension)
    if (createDebugUtilsMessengerEXT(&createInfo, nullptr, &mDebugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    } else {
        std::cout << "Debug Messenger started\n";
    }
}

VkResult NEInstance::createDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                  const VkAllocationCallbacks *pAllocator,
                                                  VkDebugUtilsMessengerEXT *pDebugMessenger) {

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(mInstance,
                                                                           "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(mInstance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

std::vector<const char *> NEInstance::getRequiredExtensions(bool enableValidationLayers) {
    glfwInit();

    //Create test window to probe needed extensions.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(0, 0, "", nullptr, nullptr);

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    glfwDestroyWindow(window);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    for (auto a : extensions) std::cout << a << std::endl;

    return extensions;
}

bool NEInstance::checkValidationLayerSupport() {
    ///get number of layers
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    ///get layer count and populate vec with available VkLayerProperties(.data?)
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    ///Iterate over each (handle?) in the validationLayers array assigning layername to the value
    for (const char *layerName : validationLayers) {
        bool layerFound = false;


        ///Iterate over each struct in the availableLayers array assigning "layerProperties" to the value then compare to validation layers.
        ///eli5: if validation layers available, pog. If not and we need em, commit seppuku cause something is wrong...
        for (const auto &layerProperties : availableLayers) {
            std::cout << layerProperties.layerName << std::endl;
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

void NEInstance::destroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT debugMessenger,
                                               const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(mInstance,"vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(mInstance, debugMessenger, pAllocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                       VkDebugUtilsMessageTypeFlagsEXT messageType,
                       const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                       void *pUserData) {

    ///if (bad) we should probably show it
   if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
     std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
   }
   return VK_FALSE;
};

VkDebugUtilsMessengerCreateInfoEXT NEInstance::populateDebugMessengerCreateInfo() {
    ///set all the jazz if desired
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    return createInfo;
};
