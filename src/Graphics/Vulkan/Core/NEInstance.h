//
// Created by nineball on 5/29/21.
//

#ifndef NINEENGINE_NEINSTANCE_H
#define NINEENGINE_NEINSTANCE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

class NEInstance {
public:
    NEInstance(bool enableValidationLayers);
    ~NEInstance();

    operator VkInstance() const { return mInstance;}

    NEInstance(const NEInstance&) = delete;
    NEInstance& operator = (const NEInstance&) = delete;

private:
    void createInstance(bool enableValidationLayers);
    void setupDebugMessenger();

    VkDebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfo();

    VkResult createDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                          const VkAllocationCallbacks *pAllocator,
                                          VkDebugUtilsMessengerEXT *pDebugMessenger);

    void destroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT debugMessenger,
                                       const VkAllocationCallbacks *pAllocator);

    ///Sub/Helper functions
    std::vector<const char *> getRequiredExtensions(bool enableValidationLayers);
    bool checkValidationLayerSupport();
private:
    VkDebugUtilsMessengerEXT mDebugMessenger;
    VkInstance mInstance;
};


#endif //NINEENGINE_NEINSTANCE_H
