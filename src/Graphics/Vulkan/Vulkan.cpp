//
// Created by nineball on 4/16/21.
//

#include "Vulkan.h"
#include "LeStuff/Instance.h"

Vulkan::Vulkan() {
    debugMessenger = Instance::setupDebugMessenger();
    instance = Instance::createInstance(true);

}

Vulkan::~Vulkan() {
    Instance::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    Instance::destroyInstance(instance);


}