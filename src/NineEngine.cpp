//
// Created by nineball on 5/22/21.
//

#include "NineEngine.h"
#include "../src/Graphics/Vulkan/Vulkan.h"
#include "Managers/Coordinator.h"


int main(){
    Coordinator& coordinator = Coordinator::Get();
    coordinator.Init();
    coordinator.RegisterComponent<Transform>();
    coordinator.RegisterComponent<VkRenderable>();
    coordinator.RegisterComponent<Forces>();

    Vulkan vulkan;
    vulkan.mainLoop();

}