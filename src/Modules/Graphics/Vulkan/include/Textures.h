//
// Created by nineball on 7/21/21.
//

#ifndef NINEENGINE_TEXTURES_H
#define NINEENGINE_TEXTURES_H

#include "Device.h"
#include "Types.h"
#include <memory>

namespace init {
    bool loadImageFromFile(std::shared_ptr<NEDevice> device, const char* file, AllocatedImage& outImage);
}







#endif //NINEENGINE_TEXTURES_H