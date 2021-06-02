//
// Created by nineball on 5/30/21.
//

#ifndef NINEENGINE_NERENDERPASS_H
#define NINEENGINE_NERENDERPASS_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>
namespace NEVK {
    class NERenderpass {
    public:
        NERenderpass(VkDevice device, VkFormat imageFormat);
        ~NERenderpass();

        VkRenderPass getRenderpass();
        operator VkRenderPass() const { return renderpass; }
    private:
        VkDevice device;
        VkRenderPass renderpass;
    };
}



#endif //NINEENGINE_NERENDERPASS_H
