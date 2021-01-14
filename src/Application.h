//
// Created by nineball on 12/23/20.
//

#ifndef NINEENGINE_APPLICATION_H
#define NINEENGINE_APPLICATION_H

#include "Graphics/Vulkan/Vulkan.h"

class Application {
public:
    Application(int width, int height, const char *title, bool resizableWindow, bool fullscreen);

    ~Application();

//  void initWindow(int width, int height, const char *title, bool resizableWindow, bool fullscreen);
//  void initVulkan();
//  void mainloop();

private:
    Vulkan *vulkan;
};


#endif //NINEENGINE_APPLICATION_H
