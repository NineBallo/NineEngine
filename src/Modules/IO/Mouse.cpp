//
// Created by nineball on 7/11/21.
//

#include "Mouse.h"
#include "iostream"

Mouse::Mouse(GLFWwindow *window) {
    mWindow = window;
    currentTick = std::chrono::steady_clock::now();
    lastTick = std::chrono::steady_clock::now();
}

void Mouse::tick() {
    if(!attached) {
        int state = glfwGetMouseButton(mWindow, GLFW_MOUSE_BUTTON_LEFT);
        if(state == GLFW_PRESS) {
            attachMouseToScreen();
            glfwGetCursorPos(mWindow, &mOldxpos, &mOldypos);
        }
    }
    else {
        int state = glfwGetKey(mWindow, GLFW_KEY_ESCAPE);
        if(state == GLFW_PRESS) {
            detachMouseFromScreen();
        } else {
            if(mouseData.mAngles == nullptr) return;

            currentTick = std::chrono::steady_clock::now();
            std::chrono::duration<float> duration = std::chrono::duration_cast<std::chrono::duration<float>>(lastTick - currentTick);
            float seconds = (duration.count());

            float adjScalar = mouseData.mScalar * seconds;

            double xpos, ypos, xdiff, ydiff;
            glfwGetCursorPos(mWindow, &xpos, &ypos);
           // std::cout << "X: " << xpos << " Y: " << ypos << std::endl;

            xdiff = xpos - mOldxpos;
            ydiff = ypos - mOldypos;

            float pitch, yaw;
            yaw = xdiff*adjScalar;
            pitch = ydiff*adjScalar;


            if(((*mouseData.mAngles).x + pitch) > 89.f) {
                (*mouseData.mAngles).x = 89.f;
            }
            else if (((*mouseData.mAngles).x + pitch) < -89.f) {
                (*mouseData.mAngles).x = -89.f;
            }else {
                mouseData.mAngles->x += pitch;
                mouseData.mAngles->y = glm::mod(mouseData.mAngles->y + (yaw * -1.f), 360.0f);
            }

            mOldxpos = xpos;
            mOldypos = ypos;
            lastTick = currentTick;
        }
    }




}

void Mouse::attachMouseToScreen() {
    glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    attached = true;
    std::cout << "Attached cursor \n";
}

void Mouse::detachMouseFromScreen() {
    glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    attached = false;
    std::cout << "Detached cursor \n";
}

void Mouse::registerMovement(float scalar, glm::vec3 *angles) {
    mouseData.mAngles = angles;
    mouseData.mScalar = scalar;
    attachMouseToScreen();
}