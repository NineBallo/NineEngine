//
// Created by nineball on 12/23/20.
//

#ifndef NINEENGINE_WINDOW_H
#define NINEENGINE_WINDOW_H

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>


class Window {
public:
    Window(int WIDTH, int HEIGHT, const char *title, bool resizable);

    Window();

    ~Window();

    GLFWwindow *GetWindowHandle();

    bool shouldExit();

private:
    void createWindow(int width, int height, const char *title, bool resizable);

private:
    GLFWwindow *window;
};


#endif //NINEENGINE_WINDOW_H
