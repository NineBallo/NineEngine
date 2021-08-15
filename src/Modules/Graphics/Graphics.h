//
// Created by nineball on 8/10/21.
//

#ifndef NINEENGINE_GRAPHICS_H
#define NINEENGINE_GRAPHICS_H
#include "Vulkan.h"

class Graphics {
public:

    void modifyMeshProperties();
    void modifyMeshTexture();

    void addDisplay();
    void removeDisplay();

private:
   // std::unique_ptr<Vulkan> vulkanRenderer;
};


#endif //NINEENGINE_GRAPHICS_H
