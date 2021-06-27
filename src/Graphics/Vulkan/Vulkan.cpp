//
// Created by nineball on 4/16/21.
//

#include "Vulkan.h"



Vulkan::Vulkan() {
    Coordinator& coordinator = Coordinator::Get();
    renderer = coordinator.RegisterSystem<NERenderer>();
    {
        Signature signature;
        signature.set(coordinator.GetComponentType<Transform>());
        signature.set(coordinator.GetComponentType<VkRenderable>());
        coordinator.SetSystemSignature<NERenderer>(signature);
    }
}

void Vulkan::mainLoop() {
    while (renderer->renderFrame()) {

    };

}


Vulkan::~Vulkan() {
}








