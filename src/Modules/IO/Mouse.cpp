//
// Created by nineball on 7/11/21.
//

#include "Mouse.h"
#include "iostream"

Mouse::Mouse(GLFWwindow *window) {
    mWindow = window;
}

void Mouse::tick() {
    if(!attached) {
        int state = glfwGetMouseButton(mWindow, GLFW_MOUSE_BUTTON_MIDDLE);
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

            //Get mouse movement
            double xpos, ypos;
            glfwGetCursorPos(mWindow, &xpos, &ypos);


            //Return if mouse has not moved
            if(xpos == mOldxpos && ypos == mOldypos) {
                return;
            }

            float pitch, yaw, xdiff, ydiff;
            xdiff = mOldxpos - xpos;
            ydiff = mOldypos - ypos;

            yaw = xdiff*mouseData.mScalar * -1;
            pitch = ydiff*mouseData.mScalar;

            if((mouseData.mAngles->x + pitch) > 89.f) {
                mouseData.mAngles->x = 89.f;
            }
            else if ((mouseData.mAngles->x + pitch) < -89.f) {
                mouseData.mAngles->x = -89.f;
            }
            else {
                mouseData.mAngles->x += pitch;
                mouseData.mAngles->y = glm::mod(mouseData.mAngles->y + yaw, 360.f);
            }

            mOldxpos = xpos;
            mOldypos = ypos;
        }
    }
}

void Mouse::attachMouseToScreen() {
    glfwSetInputMode(mWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    attached = true;
    std::cout << "Attached cursor \n";
}

void Mouse::detachMouseFromScreen() {
    glfwSetInputMode(mWindow, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
    glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    attached = false;
    std::cout << "Detached cursor \n";
}

void Mouse::registerMovement(float scalar, glm::vec3 *angles) {
    mouseData.mAngles = angles;
    mouseData.mScalar = scalar;
}