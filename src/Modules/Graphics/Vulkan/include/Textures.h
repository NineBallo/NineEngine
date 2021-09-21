//
// Created by nineball on 7/21/21.
//

#ifndef NINEENGINE_TEXTURES_H
#define NINEENGINE_TEXTURES_H

#include "Device.h"
#include "Types.h"
#include <memory>

namespace init {
    bool loadTextureFromFile(NEDevice* device, const char* file, Texture& outTex);
}







#endif //NINEENGINE_TEXTURES_H
