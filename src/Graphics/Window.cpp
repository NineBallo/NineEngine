//
// Created by nineball on 12/23/20.
//

#include "Window.h"

Window::Window() {
createWindow(800, 600, "Vulkan go brrrrrrrrrrrrrrrr");
}
Window::Window(const char *title) {
createWindow(800, 600, title);
}
Window::Window(int width, int height, const char *title) {
    createWindow(width, height, title);
}
Window::~Window() {
    glfwDestroyWindow(window);

    glfwTerminate();
}

void Window::createWindow(int width, int height, const char *title) {
    glfwInit();
    ///Dont create an opengl context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    ///No resizing for you, loser.
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
}

bool Window::shouldExit() {
    if(!glfwWindowShouldClose(window)){
        glfwPollEvents();
        return false;
    } else {
        return true;
    }

}