//
// Created by nineball on 12/23/20.
//

#include "Application.h"

///TODO fullscreen implementation.
Application::Application(int width, int height, const char *title, bool resizableWindow, bool fullscreen) {
    vulkan = new Vulkan(width, height, title, resizableWindow, fullscreen);
}

Application::~Application() {
    delete vulkan;
    std::cout << "Cleaned up! Closing...\n";
}


