//
// Created by nineball on 7/11/21.
//

#include "Keyboard.h"

#include <utility>
#include "iostream"

Keyboard::Keyboard(GLFWwindow* window) {
    lastTick = std::chrono::steady_clock::now();
    currentTick = std::chrono::steady_clock::now();
    mWindow = window;
}

void Keyboard::registerValue(Key key, float scalar, size_t index, glm::vec3 *value) {
    keyData data {
        .mScalar = scalar,
        .mIndex = index,
        .mValue = value,
    };

    keyToData[key] = data;
    packedKeys[arraySize] = key;
    arraySize++;
}


void Keyboard::registerMovement(Key key, glm::vec3 offset, float scalar, glm::vec3 *value, glm::vec3 *angles) {
   keyData data {
           .mScalar = scalar,
           .mOffset = offset,
           .mValue = value,
           .mAngle = angles,
   };
    keyToData[key] = data;
    packedKeys[arraySize] = key;
    arraySize++;
}

void Keyboard::tick() {
    currentTick = std::chrono::steady_clock::now();
    //Get elapsed time since last tick
    std::chrono::duration<float> duration = std::chrono::duration_cast<std::chrono::duration<float>>(currentTick - lastTick);
    float seconds = (duration.count());

    for(uint16_t i = 0; i < arraySize; i++) {
        Key key = packedKeys[i];
        int state = glfwGetKey(mWindow, key);

        if(state != GLFW_PRESS) {
            continue;
        }
        else {
            keyData data = keyToData[key];

            if(data.mAngle == nullptr) {
                glm::vec3 amount {0,0,0};
                amount[data.mIndex] += data.mScalar * seconds;
                *data.mValue += amount;
            }
            else {
                glm::vec3 angle = *data.mAngle;
                angle += data.mOffset;
                if(angle.y > 360.f) angle.y = glm::mod(angle.y, 360.f);
                float adjYaw = 0;
                float modifierX = 0, modifierZ = 0;

                //Split angle into "Quadrents" then make coordinates relative
                if(angle.y <= 90 && angle.y >= 0) {
                    adjYaw = angle.y;
                    modifierX = 1;
                    modifierZ = 1;
                }
                else if(angle.y <= 180 && angle.y >= 90) {
                    adjYaw = 90 - (angle.y - 90);
                    modifierX = 1;
                    modifierZ = -1;
                }
                else if(angle.y <= 270 && angle.y >= 180) {
                    adjYaw = angle.y - 180;
                    modifierX = -1;
                    modifierZ = -1;
                }
                else if(angle.y <= 360 && angle.y >= 270) {
                    adjYaw = 90 - (angle.y - 270);
                    modifierX = -1;
                    modifierZ = 1;
                }

                //Multiplyer's for direction.
                float xToHypotenuse, zToHypotenuse;
                adjYaw = adjYaw/90;
                xToHypotenuse = adjYaw * modifierX;
                zToHypotenuse = (1 - adjYaw) * modifierZ;

                float x, z;
                z = xToHypotenuse * seconds * data.mScalar;
                x = zToHypotenuse * seconds * data.mScalar;

                (*data.mValue).x += x;
                (*data.mValue).z += z;
            }
        }
    }

    lastTick = currentTick;
}