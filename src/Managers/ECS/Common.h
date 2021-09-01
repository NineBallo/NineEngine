//
// Created by nineball on 7/17/21.
//

#ifndef NINEENGINE_ECSCOMMON_H
#define NINEENGINE_ECSCOMMON_H
#include "glm/glm.hpp"

using TextureID = uint32_t;

struct Position {
    glm::vec3 coordinates {0, 0 , 0};
    glm::vec3 rotations {0, 0, 0};
    glm::vec3 scalar {1.f, 1.f, 1.f};
};


#endif //NINEENGINE_ECSCOMMON_H
