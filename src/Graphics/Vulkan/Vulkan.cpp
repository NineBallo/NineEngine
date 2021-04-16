//
// Created by nineball on 4/16/21.
//

#include "Vulkan.h"
#include "LeStuff/Instance.h"

Vulkan::Vulkan() {
    instance = createInstance(true);
    debugMessenger = setupDebugMessenger(instance);
}

Vulkan::~Vulkan() {
    destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    destroyInstance(instance);


}