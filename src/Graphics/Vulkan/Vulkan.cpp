//
// Created by nineball on 4/16/21.
//

#include "Vulkan.h"
#include "LeStuff/Instance.h"
#include "../Devices/Window.h"

using namespace VKBareAPI;

Vulkan::Vulkan() {

    windowHandle = Graphics::Window::createWindow(600, 700, "googa booga", false);

    instance = Instance::createInstance(true);
    debugMessenger = Instance::setupDebugMessenger(instance);

    surface = Surface::createSurface(instance, windowHandle);
}

Vulkan::~Vulkan() {
    Surface::destroySurface(instance, surface);
    Graphics::Window::destroyWindow(windowHandle);

    Instance::destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    Instance::destroyInstance(instance);

}