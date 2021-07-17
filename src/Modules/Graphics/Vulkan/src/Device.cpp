//
// Created by nineball on 7/16/21.
//

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <VkBootstrap.h>
#include "Device.h"
#include "Initializers.h"
#include "iostream"

NEDevice::NEDevice() {

}

NEDevice::~NEDevice() {
    mDeletionQueue.flush();
}

vkb::PhysicalDevice NEDevice::init_PhysicalDevice(VkSurfaceKHR surface, vkb::Instance &vkb_inst) {
    ///Create/Select rootDevice;
    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    vkb::PhysicalDevice physicalDevice = selector
            .set_minimum_version(1, 1)
            .set_surface(surface)
            .select()
            .value();

    mGPU = physicalDevice.physical_device;

    return physicalDevice;
}

bool NEDevice::init_LogicalDevice(vkb::PhysicalDevice &physicalDevice) {
    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();

    mDevice = vkbDevice.device;

    mGraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    mGraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    mPresentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();
    mPresentQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::present).value();
}
void NEDevice::init_Allocator(VkInstance instance) {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = mGPU;
    allocatorInfo.device = mDevice;
    allocatorInfo.instance = instance;
    vmaCreateAllocator(&allocatorInfo, &mAllocator);

    mDeletionQueue.push_function([=]() {
        vmaDestroyAllocator(mAllocator);
    });

}

VkRenderPass NEDevice::createDefaultRenderpass(VkFormat format) {
    if(mDefaultRenderpass != VK_NULL_HANDLE) return mDefaultRenderpass;

//Color
     VkAttachmentDescription color_attachment = {};
    ///TODO MSAA
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;

    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    color_attachment.format = format;

//Other stuff

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


    VkAttachmentDescription depth_attachment = {};
    depth_attachment.flags = 0;
    depth_attachment.format = VK_FORMAT_D32_SFLOAT;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref = {};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

//Subpasses
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    //hook the depth attachment into the subpass
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    //Tape em all together
    VkAttachmentDescription attachments[2] = { color_attachment, depth_attachment };

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    render_pass_info.attachmentCount = 2;
    render_pass_info.pAttachments = &attachments[0];

    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    VkRenderPass renderPass;

    if(vkCreateRenderPass(mDevice, &render_pass_info, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Renderpass failed to initialize\n");
    }

    mDeletionQueue.push_function([=]() {
        vkDestroyRenderPass(mDevice, renderPass, nullptr);
    });

    mDefaultRenderpass = renderPass;
    return renderPass;
}

///User of this function must handle destruction...
VkCommandPool NEDevice::createCommandPool(uint32_t queueFamily) {
    VkCommandPool cmdPool;
    VkCommandPoolCreateInfo commandPoolInfo = init::command_pool_create_info(queueFamily,VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    if (vkCreateCommandPool(mDevice, &commandPoolInfo, nullptr, &cmdPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create commandPool\n");
    }
    mDeletionQueue.push_function([=]() {
        vkDestroyCommandPool(mDevice, cmdPool, nullptr);
    });
    return cmdPool;
}

///Getters
VkDevice NEDevice::device() {
    return mDevice;
}
VkPhysicalDevice NEDevice::GPU() {
    return mGPU;
}
VkQueue NEDevice::presentQueue() {
    return mPresentQueue;
}
VkQueue NEDevice::graphicsQueue() {
    return mGraphicsQueue;
}

uint32_t NEDevice::presentQueueFamily() {
    return mPresentQueueFamily;
}

uint32_t NEDevice::graphicsQueueFamily() {
    return mGraphicsQueueFamily;
}

VkRenderPass NEDevice::defaultRenderpass() {
    return mDefaultRenderpass;
}
VmaAllocator NEDevice::allocator() {
    return mAllocator;
}