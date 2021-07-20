//
// Created by nineball on 7/6/21.
//

#include "Display.h"
#include <iostream>
#include <VkBootstrap.h>
#include <cmath>
#include <Initializers.h>
#include "ECS.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

NEDisplay::NEDisplay(const displayCreateInfo& createInfo) {
    createWindow(createInfo.extent, createInfo.title, createInfo.resizable);
    mTitle = createInfo.title;

    if(createInfo.instance != VK_NULL_HANDLE) {
        createSurface(createInfo.instance);
    }

    if(createInfo.device != nullptr && createInfo.presentMode) {
        createSwapchain(createInfo.device, createInfo.presentMode);
    }
    else if (createInfo.device != nullptr) {
        createSwapchain(createInfo.device, VK_PRESENT_MODE_IMMEDIATE_KHR);
    }//VK_PRESENT_MODE_FIFO_KHR

    mExtent = createInfo.extent;
    mInstance = createInfo.instance;
}

NEDisplay::~NEDisplay() {
    vkDeviceWaitIdle(mDevice->device());

    mDeletionQueue.flush();

    if(mSurface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
    }

    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

void NEDisplay::createWindow(VkExtent2D extent, const std::string& title, bool resizable) {
    glfwInit();

    ///Dont create an opengl context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (resizable) {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    } else {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    }

    mWindow = glfwCreateWindow(extent.width, extent.height, title.c_str(), nullptr, nullptr);

    //  glfwSetWindowUserPointer(window, vulkan);
    //  glfwSetFramebufferSizeCallback(window, VKBareAPI::Window::framebufferResizeCallback);
}

void NEDisplay::createSurface(VkInstance instance) {
    if (glfwCreateWindowSurface(instance, mWindow, nullptr, &mSurface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!\n");
    }
}

void NEDisplay::createSwapchain(std::shared_ptr<NEDevice> device, VkPresentModeKHR presentMode) {
    vkb::SwapchainBuilder swapchainBuilder{device->GPU(), device->device(), mSurface};
    vkb::Swapchain vkbSwapchain = swapchainBuilder
            .use_default_format_selection()
            .set_desired_present_mode(presentMode)
            .set_desired_extent(mExtent.width, mExtent.height)
            .build()
            .value();

    mSwapchain = vkbSwapchain.swapchain;
    mImages = vkbSwapchain.get_images().value();
    mImageViews = vkbSwapchain.get_image_views().value();
    mFormat = vkbSwapchain.image_format;

    mDevice = device;

    mDeletionQueue.push_function([=]() {
        vkDestroySwapchainKHR(mDevice->device(), mSwapchain, nullptr);
    });

    VkExtent3D depthImageExtent = {
            mExtent.width,
            mExtent.height,
            1
    };

    mDepthFormat = VK_FORMAT_D32_SFLOAT;
    VkImageCreateInfo dimg_info = init::image_create_info(mDepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

    VmaAllocationCreateInfo dimg_allocinfo = {};
    dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vmaCreateImage(mDevice->allocator(), &dimg_info, &dimg_allocinfo, &mDepthImage.mImage, &mDepthImage.mAllocation, nullptr);

    VkImageViewCreateInfo dview_info = init::imageview_create_info(mDepthFormat, mDepthImage.mImage, VK_IMAGE_ASPECT_DEPTH_BIT);

    vkCreateImageView(mDevice->device(), &dview_info, nullptr, &mDepthImageView);

    mDeletionQueue.push_function([=]() {
        vkDestroyImageView(mDevice->device(), mDepthImageView, nullptr);
        vmaDestroyImage(mDevice->allocator(), mDepthImage.mImage, mDepthImage.mAllocation);
    });

    //Gonna need this later...
    populateFrameData();
}

void NEDisplay::createDescriptors() {
    const size_t sceneParamBufferSize = MAX_FRAMES * mDevice->padUniformBufferSize(sizeof(GPUSceneData));

    mSceneParameterBuffer = mDevice->createBuffer(sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    for (int i = 0; i < MAX_FRAMES; i++)
    {

        mFrames[i].mObjectBuffer = mDevice->createBuffer(sizeof(GPUObjectData) * MAX_ENTITYS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        mFrames[i].mCameraBuffer = mDevice->createBuffer(sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        mFrames[i].mGlobalDescriptor = mDevice->createDescriptorSet(mDevice->globalSetLayout());
        mFrames[i].mObjectDescriptor = mDevice->createDescriptorSet(mDevice->objectSetLayout());

        VkDescriptorBufferInfo cameraInfo;
        cameraInfo.buffer = mFrames[i].mCameraBuffer.mBuffer;
        cameraInfo.offset = 0;
        cameraInfo.range = sizeof(GPUCameraData);

        VkDescriptorBufferInfo sceneInfo;
        sceneInfo.buffer = mSceneParameterBuffer.mBuffer;
        sceneInfo.offset = 0;
        sceneInfo.range = sizeof(GPUSceneData);

        VkDescriptorBufferInfo objectBufferInfo;
        objectBufferInfo.buffer = mFrames[i].mObjectBuffer.mBuffer;
        objectBufferInfo.offset = 0;
        objectBufferInfo.range = sizeof(GPUObjectData) * MAX_ENTITYS;


        VkWriteDescriptorSet cameraWrite = init::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, mFrames[i].mGlobalDescriptor, &cameraInfo, 0);
        VkWriteDescriptorSet sceneWrite = init::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, mFrames[i].mGlobalDescriptor, &sceneInfo, 1);

        VkWriteDescriptorSet objectWrite = init::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, mFrames[i].mObjectDescriptor, &objectBufferInfo, 0);

        VkWriteDescriptorSet setWrites[] = { cameraWrite, sceneWrite, objectWrite };

        vkUpdateDescriptorSets(mDevice->device(), 3, setWrites, 0, nullptr);

        mDeletionQueue.push_function([=]() {
            vmaDestroyBuffer(mDevice->allocator(), mFrames[i].mObjectBuffer.mBuffer, mFrames[i].mObjectBuffer.mAllocation);
            vmaDestroyBuffer(mDevice->allocator(), mFrames[i].mCameraBuffer.mBuffer, mFrames[i].mCameraBuffer.mAllocation);
        });
    }
    mDeletionQueue.push_function([=]() {
        vmaDestroyBuffer(mDevice->allocator(), mSceneParameterBuffer.mBuffer, mSceneParameterBuffer.mAllocation);
    });
}

void NEDisplay::initImGUI() {
    ////Create a *large* descriptor pool for imgui
    //VkDescriptorPoolSize pool_sizes[] =
    //        {
    //                { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
    //                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
    //                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
    //                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
    //                { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
    //                { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
    //                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
    //                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
    //                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
    //                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
    //                { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    //        };
//
    //VkDescriptorPoolCreateInfo pool_info = {};
    //pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    //pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    //pool_info.maxSets = 1000;
    //pool_info.poolSizeCount = std::size(pool_sizes);
    //pool_info.pPoolSizes = pool_sizes;
//
    //VkDescriptorPool imguiPool;
    //vkCreateDescriptorPool(mDevice->device(), &pool_info, nullptr, &imguiPool);
//
//
    //ImGui::CreateContext();
//
    //ImGui_ImplGlfw_InitForVulkan(mWindow, true);
//
    ////Init Data
    //ImGui_ImplVulkan_InitInfo initInfo{};
    //ImGui_ImplVulkan_InitInfo init_info = {};
    //init_info.Instance = mInstance;
    //init_info.PhysicalDevice = mDevice->GPU();
    //init_info.Device = mDevice->device();
    //init_info.Queue = mDevice->graphicsQueue();
    //init_info.DescriptorPool = imguiPool;
    //init_info.MinImageCount = 3;
    //init_info.ImageCount = 3;
    //init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
//
    //ImGui_ImplVulkan_Init(&init_info, mDevice->defaultRenderpass());
//
    //ImGui_ImplVulkan_DestroyFontUploadObjects();
    //mDeletionQueue.push_function([=]() {
    //    vkDestroyDescriptorPool(mDevice->device(), imguiPool, nullptr);
    //    ImGui_ImplVulkan_Shutdown();
    //});
}

void NEDisplay::createFramebuffers(VkRenderPass renderpass) {
    mRenderpass = renderpass;
    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = nullptr;

    fb_info.renderPass = mRenderpass;
    fb_info.attachmentCount = 1;
    fb_info.width = mExtent.width;
    fb_info.height = mExtent.height;
    fb_info.layers = 1;

    //Swapchain size
    const uint32_t swapchain_imagecount = mImages.size();
    mFramebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

    //Create a corresponding framebuffer for each image
    for (int i = 0; i < swapchain_imagecount; i++) {

        VkImageView attachments[2];
        attachments[0] = mImageViews[i];
        attachments[1] = mDepthImageView;

        fb_info.pAttachments = &attachments[0];
        fb_info.attachmentCount = 2;

        if(vkCreateFramebuffer(mDevice->device(), &fb_info, nullptr, &mFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer\n");
        };

        mDeletionQueue.push_function([=]() {
            vkDestroyFramebuffer(mDevice->device(), mFramebuffers[i], nullptr);
            vkDestroyImageView(mDevice->device(), mImageViews[i], nullptr);
        });
    }
}

void NEDisplay::createSyncStructures(FrameData &frame) {
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = nullptr;

    //Create it already signaled to streamline renderloop
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (vkCreateFence(mDevice->device(), &fenceCreateInfo, nullptr, &frame.mRenderFence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create synchronization fences\n");
    };

    //Queue fence for eventual deletion
    mDeletionQueue.push_function([=]() {
        vkDestroyFence(mDevice->device(), frame.mRenderFence, nullptr);
    });

    //For the semaphores we don't need any flags
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    if (vkCreateSemaphore(mDevice->device(), &semaphoreCreateInfo, nullptr, &frame.mPresentSemaphore) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create present semaphore\n");
    };

    if (vkCreateSemaphore(mDevice->device(), &semaphoreCreateInfo, nullptr, &frame.mRenderSemaphore) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render semaphore\n");
    };


    mDeletionQueue.push_function([=]() {
        vkDestroySemaphore(mDevice->device(), frame.mPresentSemaphore, nullptr);
        vkDestroySemaphore(mDevice->device(), frame.mRenderSemaphore, nullptr);
    });
}

VkCommandBuffer NEDisplay::createCommandBuffer(VkCommandPool commandPool) {
    VkCommandBuffer temp;
    //Create Command Buffer
    VkCommandBufferAllocateInfo cmdAllocInfo = init::command_buffer_allocate_info(commandPool, 1);
    if (vkAllocateCommandBuffers(mDevice->device(), &cmdAllocInfo, &temp) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create display primary command buffer\n");
    }
    return temp;
}

void NEDisplay::populateFrameData() {
    for(uint8_t i = 0; i < MAX_FRAMES; i++) {
        FrameData &frame = mFrames[i];
        frame.mCommandPool = mDevice->createCommandPool(mDevice->graphicsQueueFamily());
        frame.mCommandBuffer = createCommandBuffer(frame.mCommandPool);
        createSyncStructures(frame);

        mDeletionQueue.push_function([=]() {
            vkFreeCommandBuffers(mDevice->device(), mFrames[i].mCommandPool, 1, &mFrames[i].mCommandBuffer);
            vkDestroyCommandPool(mDevice->device(), mFrames[i].mCommandPool, nullptr);
        });
    }
}

VkCommandBuffer NEDisplay::startFrame() {
    ///Calculate FPS
    currentTick = std::chrono::steady_clock::now();
    std::chrono::duration<float> duration = std::chrono::duration_cast<std::chrono::duration<float>>(currentTick - lastTick);
    float seconds = (duration.count());
    std::string newTitle = mTitle + "  [FPS: " + std::to_string((uint16_t)(std::floor(1/seconds))) + "] ";
    glfwSetWindowTitle(mWindow, newTitle.c_str());
    lastTick = currentTick;

    FrameData &frame = mFrames[mCurrentFrame];

    //Wait for frame to be ready/(returned to "back")
    vkWaitForFences(mDevice->device(), 1, &frame.mRenderFence, true, 1000000000);
    vkResetFences(mDevice->device(), 1, &frame.mRenderFence);

    //Get current swapchain index
    vkAcquireNextImageKHR(mDevice->device(), mSwapchain, 1000000000, frame.mPresentSemaphore, nullptr, &mSwapchainImageIndex);

    //Wipe and prep command buffer to be handed to the renderer
    vkResetCommandBuffer(frame.mCommandBuffer, 0);

    VkCommandBufferBeginInfo cmdBeginInfo = {};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;
    cmdBeginInfo.pInheritanceInfo = nullptr;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(frame.mCommandBuffer, &cmdBeginInfo);

    //Clear framebuffer
    VkClearValue clearValue;
    clearValue.color = { { 0.01f, 0.01f, 0.01f, 1.f}};

    VkClearValue depthClear;
    depthClear.depthStencil.depth = 1.f;

    //Lets start painting
    VkRenderPassBeginInfo rpInfo = {};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.pNext = nullptr;

    rpInfo.renderPass = mRenderpass;
    rpInfo.renderArea.offset.x = 0;
    rpInfo.renderArea.offset.y = 0;
    rpInfo.renderArea.extent = mExtent;
    rpInfo.framebuffer = mFramebuffers[mSwapchainImageIndex];

    //Set the value to clear to in renderpass
    rpInfo.clearValueCount = 2;
    VkClearValue clearValues[] = { clearValue, depthClear };
    rpInfo.pClearValues = &clearValues[0];

    vkCmdBeginRenderPass(frame.mCommandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkRect2D scissor = {0,0, mExtent.width, mExtent.height};
    vkCmdSetScissor(frame.mCommandBuffer, 0, 1, &scissor);

    VkViewport viewport;
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = mExtent.width;
    viewport.height = mExtent.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(frame.mCommandBuffer, 0, 1, &viewport);


    float framed = (mFrameCount / 120.f);

    mSceneData.ambientColor = { sin(framed),0,cos(framed),1 };

    char* sceneData;
    vmaMapMemory(mDevice->allocator(), mSceneParameterBuffer.mAllocation, (void**)&sceneData);

    sceneData += mDevice->padUniformBufferSize(sizeof(GPUSceneData)) * mCurrentFrame;

    memcpy(sceneData, &mSceneData, sizeof(GPUSceneData));

    vmaUnmapMemory(mDevice->allocator(), mSceneParameterBuffer.mAllocation);

    //Farewell command buffer o/; May your errors gentle.
    return frame.mCommandBuffer;
}

void NEDisplay::endFrame() {
    //Though wise men at their end know dark is right,
    //Because their words had forked no lightning they
    //Do not go gentle into that good night.
    FrameData &frame = mFrames[mCurrentFrame];

    vkCmdEndRenderPass(frame.mCommandBuffer);
    vkEndCommandBuffer(frame.mCommandBuffer);

    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pNext = nullptr;

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    submit.pWaitDstStageMask = &waitStage;

    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &frame.mPresentSemaphore;

    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &frame.mRenderSemaphore;

    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &frame.mCommandBuffer;

    //Submit command buffer and execute it.
    vkQueueSubmit(mDevice->presentQueue(), 1, &submit, frame.mRenderFence);

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;

    presentInfo.pSwapchains = &mSwapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores = &frame.mRenderSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &mSwapchainImageIndex;


    vkQueuePresentKHR(mDevice->presentQueue(), &presentInfo);

    //Rage, rage against the dying of the light.
    //Should flippy flop between 0 and MAX_FRAMES - 1
   mCurrentFrame = (mCurrentFrame + 1) % (MAX_FRAMES);
   mFrameCount++;
}


bool NEDisplay::shouldExit() {
    if (!glfwWindowShouldClose(mWindow)) {
        glfwPollEvents();
        return false;
    } else {
        return true;
    }
}

FrameData NEDisplay::currentFrame() {
    return mFrames[mCurrentFrame];
}

VkSurfaceKHR NEDisplay::surface() {return mSurface;}
VkFormat NEDisplay::format() {return mFormat;}
VkExtent2D NEDisplay::extent() {return mExtent;}
GLFWwindow *NEDisplay::window() {return mWindow;}

uint32_t NEDisplay::frameIndex() {
    return mCurrentFrame;
}