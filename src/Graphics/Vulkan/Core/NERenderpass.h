//
// Created by nineball on 5/30/21.
//

#ifndef NINEENGINE_NERENDERPASS_H
#define NINEENGINE_NERENDERPASS_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>

class NERenderpass {
public:
    NERenderpass(VkDevice device, VkFormat imageFormat);
    ~NERenderpass();

    NERenderpass(const NERenderpass&) = delete;
    NERenderpass& operator = (const NERenderpass&) = delete;

    operator VkRenderPass() const { return mRenderPass;}

private:
    VkRenderPass mRenderPass;
    VkDevice mDevice;
};




#endif //NINEENGINE_NERENDERPASS_H
