//
// Created by nineball on 4/16/21.
//

#include "Vulkan.h"

#include "../Devices/Window.h"


using namespace VKBareAPI;

Vulkan::Vulkan() {

    windowHandle = Graphics::Window::createWindow(600, 700, "googa booga", false);

    ///initialize vulky
    instance = Instance::createInstance(true);
    debugMessenger = Instance::setupDebugMessenger(instance);

    ///Setup window with Vulkan
    surface = Surface::createSurface(instance, windowHandle);

    ///Setup Vulkan devices/queues
    physicalDevice = Device::pickPhysicalDevice(instance, surface);
    deviceQueues = Device::createLogicalDevice(physicalDevice, surface);




}

Vulkan::~Vulkan() {
    Device::destroy(deviceQueues.device);

    Surface::destroySurface(instance, surface);
    Graphics::Window::destroyWindow(windowHandle);

    Instance::destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    Instance::destroyInstance(instance);



}

