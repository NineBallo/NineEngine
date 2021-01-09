//
// Created by nineball on 12/24/20.
//

#ifndef NINEENGINE_SURFACE_H
#define NINEENGINE_SURFACE_H
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <iostream>
#include "../vkGlobalPool.h"

class Surface {
public:
    Surface();

    ~Surface();



private:
    VkSurfaceKHR surface;
};


#endif //NINEENGINE_SURFACE_H
