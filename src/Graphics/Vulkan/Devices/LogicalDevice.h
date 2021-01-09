//
// Created by nineball on 12/24/20.
//

#ifndef NINEENGINE_LOGICALDEVICE_H
#define NINEENGINE_LOGICALDEVICE_H

#include "PhysicalDevice.h"
#include "../vkGlobalPool.h"

class LogicalDevice : public PhysicalDevice {
public:
    LogicalDevice(bool _enableValidationLayers);
    ~LogicalDevice();

private:
    void createLogicalDevice();
    void populateVkDeviceQueueCreateInfo(VkDeviceQueueCreateInfo &CreateInfo, uint32_t family);


private:
    //Device stuff
    VkDevice logicalDevice;
    VkQueue graphicsQueue;
    QueueFamilyIndices indices;
    VkQueue presentQueue;

    ////TODO Condense all validation layer jazz into one file to reduce duplicate code.
    //Validation layer stuff
    bool enableValidationLayers = false;
    std::vector<const char *> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };
};


#endif //NINEENGINE_LOGICALDEVICE_H
