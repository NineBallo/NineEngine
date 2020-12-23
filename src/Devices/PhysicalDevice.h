//
// Created by nineball on 12/23/20.
//

#ifndef VULKANATTEMPT_PHYSICALDEVICES_H
#define VULKANATTEMPT_PHYSICALDEVICES_H

///this lets glfw load the Graphics library for us.
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <cstdlib>
#include <vector>
#include <optional>
#include <any>




struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;

    bool isComplete() {
        return graphicsFamily.has_value();
    }

};


class PhysicalDevice {
public:
    PhysicalDevice(VkInstance *instance);
    ~PhysicalDevice();

private:
    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    //int rateDeviceSuitability(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

private:
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkInstance *instance;
};



#endif //VULKANATTEMPT_PHYSICALDEVICES_H
