//
// Created by nineball on 4/16/21.
//

//
// #ifndef NINEENGINE_RENDERPASS_H
//#define NINEENGINE_RENDERPASS_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#include <iostream>

namespace VKBareAPI::Pipeline::Renderpass {
    VkRenderPass createRenderPass(VkDevice device, VkFormat imageFormat);
    void destroy(VkRenderPass renderPass, VkDevice device);
}


//#endif //NINEENGINE_RENDERPASS_H
