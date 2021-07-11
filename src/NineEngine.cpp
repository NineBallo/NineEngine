//
// Created by nineball on 5/22/21.
//

#include "NineEngine.h"
#include "Managers/ECS/Coordinator.h"
#include "Modules/Graphics/Vulkan/include/Vulkan.h"

int main(){
    Coordinator& coordinator = Coordinator::Get();
    coordinator.Init();
    coordinator.RegisterComponent<Transform>();
    coordinator.RegisterComponent<VkRenderable>();
    coordinator.RegisterComponent<Forces>();

    Vulkan renderer{};

    renderer.tick();
}