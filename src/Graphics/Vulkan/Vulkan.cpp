//
// Created by nineball on 4/16/21.
//

#include "Vulkan.h"



Vulkan::Vulkan() {
    renderer = std::make_shared<NEVK::NERenderer>();
}

void Vulkan::mainLoop() {
    renderer->renderFrame();
}


Vulkan::~Vulkan() {
}








