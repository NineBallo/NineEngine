//
// Created by nineball on 7/11/21.
//

#ifndef NINEENGINE_MOUSE_H
#define NINEENGINE_MOUSE_H
#include "GLFW/glfw3.h"
#include "array"
#include "chrono"
#include "glm/ext.hpp"
#include "glm/glm.hpp"

#define MAX_MOUSE_REGISTERS 1

struct MouseData {
    float mScalar = 0.f;
    glm::vec3* mAngles = nullptr;
};

class Mouse {
public:
    Mouse(GLFWwindow* window);

    void attachMouseToScreen();
    void detachMouseFromScreen();

    void registerMovement(float scalar, glm::vec3* angles);

    void tick();

private:
    GLFWwindow* mWindow;

private:
    bool attached = false;

    MouseData mouseData {};
    double mOldxpos, mOldypos;
};


#endif //NINEENGINE_MOUSE_H
