//
// Created by nineball on 7/11/21.
//

#ifndef NINEENGINE_KEYBOARD_H
#define NINEENGINE_KEYBOARD_H
#include "GLFW/glfw3.h"
#include "array"
#include "memory"
#include "chrono"
#include "glm/ext.hpp"
#include "glm/glm.hpp"

#define MAX_BINDINGS 255
using Key = uint16_t;

struct keyData {
    float mScalar {};
    size_t mIndex = 0;
    glm::vec3 mOffset {0, 0, 0};
    glm::vec3 *mValue = nullptr;
    glm::vec3 *mAngle = nullptr;
};


class Keyboard {
public:
    Keyboard(GLFWwindow* window);
  //  ~Keyboard();

    void registerValue(Key key, float scalar, size_t index, glm::vec3 *value);
    void registerMovement(Key key, glm::vec3 offset, float scalar, glm::vec3 *value, glm::vec3 *angles);

  //  void registerFunction(Key key, void (*function)(float scalar));

    void tick();

private:
    std::chrono::time_point<std::chrono::steady_clock> currentTick;
    std::chrono::time_point<std::chrono::steady_clock> lastTick;

private:
    std::array<Key, MAX_BINDINGS> packedKeys;
    std::array<keyData, MAX_BINDINGS> keyToData;
    uint16_t arraySize = 0;

private:
    GLFWwindow* mWindow;
};



#endif //NINEENGINE_KEYBOARD_H
