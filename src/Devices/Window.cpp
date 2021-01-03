//
// Created by nineball on 12/23/20.
//

#include "Window.h"

Window::Window() {
    createWindow(800, 600, "Vulkan go brrrrrrrrrrrrrrrr", false);
}

Window::Window(int width, int height, const char *title, bool resizable) {
    createWindow(width, height, title, resizable);
}

Window::~Window() {
    glfwDestroyWindow(window);

    glfwTerminate();
}

GLFWwindow *Window::GetWindowHandle() {
    return window;
}

void Window::createWindow(int width, int height, const char *title, bool resizable) {
    glfwInit();
    ///Dont create an opengl context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (resizable) {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    } else {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    }

    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
}

bool Window::shouldExit() {
    if (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        return false;
    } else {
        return true;
    }

}