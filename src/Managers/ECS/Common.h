//
// Created by nineball on 7/17/21.
//

#ifndef NINEENGINE_ECSCOMMON_H
#define NINEENGINE_ECSCOMMON_H
#include "glm/glm.hpp"

#define MAX_ENTITYS 1000
#define MAX_DISPLAYS 5
#define MAX_COMPONENTS 10
#define MAX_MODULES 2

using Entity = uint32_t;
using Display = uint8_t;
using Signature = std::bitset<MAX_COMPONENTS>;
using Component = unsigned short;
using System = std::string_view;

struct Position {
    glm::vec3 coordinates {0, 0 , 0};
    glm::vec3 rotations {0, 0, 0};
    glm::vec3 scalar {1.f, 1.f, 1.f};
};


#endif //NINEENGINE_ECSCOMMON_H
