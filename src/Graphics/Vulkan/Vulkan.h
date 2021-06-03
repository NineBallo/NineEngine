//
// Created by nineball on 4/16/21.
//

#ifndef NINEENGINE_VULKAN_H
#define NINEENGINE_VULKAN_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Core/Renderer.h"
#include "memory"



class Vulkan {
public:
    Vulkan();
    ~Vulkan();


    void mainLoop();

private:
    std::shared_ptr<NEVK::NERenderer> renderer;
};




#endif //NINEENGINE_VULKAN_H
