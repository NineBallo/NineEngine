//
// Created by nineball on 5/29/21.
//

#ifndef NINEENGINE_NEINSTANCE_H
#define NINEENGINE_NEINSTANCE_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>


namespace NEVK {
    class NEInstance {
    public:
        NEInstance(bool enableValidationLayers);
        ~NEInstance();
        VkInstance getInstance();


        operator VkInstance() const { return instance; }

    private:
        void createInstance(bool enableValidationLayers);
        void setupDebugMessenger();
        VkDebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfo();
        bool checkValidationLayerSupport();
        VkResult createDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                              const VkAllocationCallbacks *pAllocator,
                                              VkDebugUtilsMessengerEXT *pDebugMessenger);
        void destroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT debugMessenger,
                                           const VkAllocationCallbacks *pAllocator);
        ///Sub/Helper functions
        std::vector<const char *> getRequiredExtensions(bool enableValidationLayers);


    private:
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
    };
}



#endif //NINEENGINE_NEINSTANCE_H
