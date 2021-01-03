//
// Created by nineball on 12/24/20.
//

#ifndef NINEENGINE_SURFACE_H
#define NINEENGINE_SURFACE_H
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <iostream>


class Surface {
public:
    Surface(VkInstance *instance, GLFWwindow *window);

    ~Surface();

    VkSurfaceKHR *getVkSurfaceKHRPTR();

private:
    GLFWwindow *window;
    VkInstance *instance;
    VkSurfaceKHR surface;
};


#endif //NINEENGINE_SURFACE_H
