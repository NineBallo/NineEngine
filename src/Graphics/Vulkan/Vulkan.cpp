//
// Created by nineball on 4/16/21.
//

#include "Vulkan.h"
#include "../Devices/Window.h"



using namespace VKBareAPI;

Vulkan::Vulkan() {

    windowVars.window = Graphics::Window::createWindow(600, 700, "why do i hate myself", true, this);

    ///initialize vulky
    instance = Instance::createInstance(true);
    debugMessenger = Instance::setupDebugMessenger(instance);

    ///Setup window with Vulkan
    Window::createSurface(windowVars, instance);

    ///Setup Vulkan devices/queues
    Device::createDevices(deviceVars, instance, windowVars.surface, swapchainVars.swapChainSupportDetails);

    ///Setup swapchain
    Swapchain::createSwapChain(swapchainVars, deviceVars, windowVars);

    ///Setup pipeline
    Pipeline::create(pipelineVars, deviceVars, swapchainVars);

    ///Setup commandBuffer
    Buffers::create(deviceVars, swapchainVars, pipelineVars);

    VKExtraAPI::Texture::createTextureImage(deviceVars);
    VKExtraAPI::Texture::createTextureImageView(deviceVars.Buffers.textureImage, deviceVars.Buffers.textureImageView,  deviceVars.device);

    VKBareAPI::Pipeline::createTextureSampler(pipelineVars.textureSampler, deviceVars.device, deviceVars.physicalDevice);

    VKBareAPI::Buffers::createDescriptorSets(deviceVars, swapchainVars, pipelineVars);
    VKBareAPI::Buffers::createCommandBuffers(deviceVars, swapchainVars, pipelineVars);




    mainLoop();
}

Vulkan::~Vulkan() {

    Swapchain::destroy(swapchainVars, deviceVars, pipelineVars);

    vkDestroyImageView(deviceVars.device, deviceVars.Buffers.textureImageView, nullptr);
    vkDestroyImage(deviceVars.device, deviceVars.Buffers.textureImage, nullptr);
    vkFreeMemory(deviceVars.device, deviceVars.Buffers.textureImageMemory, nullptr);


    vkDestroyDescriptorSetLayout(deviceVars.device, pipelineVars.descriptorSetLayout, nullptr);

    Buffers::destroy(deviceVars);

    Device::destroy(deviceVars.device);

    Window::destroySurface(instance, windowVars.surface);

    Instance::destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    Instance::destroyInstance(instance);

    Graphics::Window::destroyWindow(windowVars.window);
}


void Vulkan::mainLoop() {
    while(!Graphics::Window::shouldExit(windowVars.window)){
        VKBareAPI::Swapchain::drawFrame(swapchainVars, deviceVars, pipelineVars, windowVars);
    }
    vkDeviceWaitIdle(deviceVars.device);
}



