//
// Created by nineball on 7/6/21.
//

#include "Vulkan.h"

#include <utility>
#include <VkBootstrap.h>
#include <iostream>
#include <fstream>

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include "chrono"
#include "Initializers.h"
#include "Pipeline.h"
#include "memory"
#include <vk_mem_alloc.h>
#include "unistd.h"
#include "backends/imgui_impl_vulkan.h"

Vulkan::Vulkan(ECS &ecs, Entity cameraEntity) : mECS{ECS::Get()} {

    init();
}

void createDisplay(Display display) {
    mDisplays[mDisplayCount].emplace()
}


void Vulkan::tick() {
    if (mRootDisplay->shouldExit()) {
        mRootDisplay.reset();
        mShouldExit = true;
    } else {
        draw();
    }
}

bool Vulkan::shouldExit() {return mShouldExit;}
