///this lets glfw load the Graphics library for us.

#include "../src/Application.h"
#include "main.h"
#include "iostream"
vkGlobalPool vkGlobalPool::s_Instance;


int main() {

    sandbox app(800, 600, "Vulkan go brrrrrrrrt", false, false);


}


sandbox::sandbox(int width, int height, const char *title, bool resizableWindow, bool fullscreen) : Application(width,
                                                                                                                height,
                                                                                                                title,
                                                                                                                resizableWindow,
                                                                                                                fullscreen) {


}