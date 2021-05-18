//
// Created by nineball on 4/26/21.
//

#include "app.h"
#include "../src/Graphics/Vulkan/Vulkan.h"
#include "../src/Graphics/Devices/Window.h"

int main(){
    Vulkan crab;

    while(!Graphics::Window::shouldExit(crab.windowVars.window)){
        VKBareAPI::Swapchain::drawFrame(crab.swapchainVars, crab.deviceVars);
    };

}