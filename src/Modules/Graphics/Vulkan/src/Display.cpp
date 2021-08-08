//
// Created by nineball on 7/6/21.
//

#include "../../Common/ImGuiHelpers.h"
#include "Display.h"
#include <iostream>
#include <VkBootstrap.h>
#include <cmath>
#include <Initializers.h>
#include "ECS.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
//Circular include issues..


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


    mGUI = std::make_unique<NEGUI>(this);
}

NEDisplay::~NEDisplay() {
    vkDeviceWaitIdle(mDevice->device());

    mSwapchainQueue.flush();
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


}

void NEDisplay::createSurface(VkInstance instance) {
    if (glfwCreateWindowSurface(instance, mWindow, nullptr, &mSurface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!\n");
    }
}

void NEDisplay::createSwapchain(const std::shared_ptr<NEDevice>& device, VkPresentModeKHR presentMode) {
    int width, height;
    glfwGetWindowSize(mWindow, &width, &height);
    mExtent.width = width; mExtent.height = height;
    vkb::SwapchainBuilder swapchainBuilder{device->GPU(), device->device(), mSurface};
    vkb::Swapchain vkbSwapchain = swapchainBuilder
            .use_default_format_selection()
            .set_desired_present_mode(presentMode)
            .set_desired_extent(width, height)
            .build()
            .value();


    RenderPassInfo &RPInfo = mRenderPasses[NE_RENDERPASS_TOSWAPCHAIN_BIT];

    mSwapchain = vkbSwapchain.swapchain;
    std::vector<VkImage> images = vkbSwapchain.get_images().value();
    RPInfo.resolveImageViews = vkbSwapchain.get_image_views().value();
    mFormat = vkbSwapchain.image_format;
    mPresentMode = presentMode;
    mDevice = device;

    mSwapchainImageCount = images.size();
    for(uint32_t i = 0; i > mSwapchainImageCount; i++) {
        RPInfo.resolveImages[i].mImage = images[i];
    }

    mSwapchainQueue.push_function([=, this]() {
        vkDestroySwapchainKHR(mDevice->device(), mSwapchain, nullptr);
    });


    //Gonna need this later...
    populateFrameData();
}

void NEDisplay::createImage(VkExtent2D extent, uint32_t mipLevels, AllocatedImage &image, VkImageView &imageView, VkImageAspectFlagBits aspect, VkFormat format, VkImageUsageFlagBits usage, VkSampleCountFlagBits sampleCount) {

    VkImageCreateInfo img_info = init::image_create_info(format, usage, extent, mipLevels, sampleCount);

    VmaAllocationCreateInfo img_allocinfo = {};
    img_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    img_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    auto wawa = vmaCreateImage(mDevice->allocator(), &img_info, &img_allocinfo, &image.mImage, &image.mAllocation, nullptr);

    if(wawa != VK_SUCCESS) {
        std::cout << "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
    }
    VkImageViewCreateInfo view_info = init::imageview_create_info(format, image.mImage, mipLevels, aspect);
    vkCreateImageView(mDevice->device(), &view_info, nullptr, &imageView);
}

void NEDisplay::recreateSwapchain() {
    vkDeviceWaitIdle(mDevice->device());
    std::cout << "Recreating Swapchain\n";
    mSwapchainQueue.flush();

    createSwapchain(mDevice, mPresentMode);

    resizeFrameBuffer(mExtent, NE_RENDERPASS_TOSWAPCHAIN_BIT);
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

        VkWriteDescriptorSet setWrites[3] = { cameraWrite, sceneWrite, objectWrite};
        size_t writeSize = 3;


        if(mDevice->bindless()) mFrames[i].mTextureDescriptor = mDevice->createDescriptorSet(mDevice->textureSetLayout());


        vkUpdateDescriptorSets(mDevice->device(), writeSize, setWrites, 0, nullptr);

        mDeletionQueue.push_function([=, this]() {
            vmaDestroyBuffer(mDevice->allocator(), mFrames[i].mObjectBuffer.mBuffer, mFrames[i].mObjectBuffer.mAllocation);
            vmaDestroyBuffer(mDevice->allocator(), mFrames[i].mCameraBuffer.mBuffer, mFrames[i].mCameraBuffer.mAllocation);
        });
    }
    mDeletionQueue.push_function([=, this]() {
        vmaDestroyBuffer(mDevice->allocator(), mSceneParameterBuffer.mBuffer, mSceneParameterBuffer.mAllocation);
    });
}

void NEDisplay::initImGUI() {
    //Create a *large* descriptor pool for imgui
    VkDescriptorPoolSize pool_sizes[] =
            {
                    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
            };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;


    vkCreateDescriptorPool(mDevice->device(), &pool_info, nullptr, &mGuiDescriptorPool);


    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForVulkan(mWindow, true);

    //Init Data

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = mInstance;
    init_info.PhysicalDevice = mDevice->GPU();
    init_info.Device = mDevice->device();
    init_info.Queue = mDevice->graphicsQueue();
    init_info.DescriptorPool = mGuiDescriptorPool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = mDevice->sampleCount();

    ImGui_ImplVulkan_Init(&init_info, mRenderPasses[NE_RENDERPASS_TOSWAPCHAIN_BIT].renderPass);

    //execute a gpu command to upload imgui font textures
    mDevice->immediateSubmit([&](VkCommandBuffer cmd) {
        ImGui_ImplVulkan_CreateFontsTexture(cmd);
    });

    //clear font textures from cpu data
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    mDeletionQueue.push_function([=, this]() {
        vkDestroyDescriptorPool(mDevice->device(), mGuiDescriptorPool, nullptr);
        ImGui_ImplVulkan_Shutdown();
    });
}

void NEDisplay::createFramebuffers(VkExtent2D FBSize, VkFormat format, uint32_t flags, bool MSAA) {
    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = nullptr;

    RenderPassInfo& rpInfo = mRenderPasses[flags];
    fb_info.renderPass = rpInfo.renderPass;
    fb_info.attachmentCount = 1;
    fb_info.width = FBSize.width;
    fb_info.height = FBSize.height;
    fb_info.layers = 1;




    //Swapchain size
    rpInfo.frameBuffers = std::vector<VkFramebuffer>(mSwapchainImageCount);
    if(rpInfo.colorImages.size() != mSwapchainImageCount) {
        rpInfo.depthImages.resize(mSwapchainImageCount);
        rpInfo.depthImageViews.resize(mSwapchainImageCount);
        rpInfo.colorImages.resize(mSwapchainImageCount);
        rpInfo.colorImageViews.resize(mSwapchainImageCount);


        if((flags & NE_RENDERPASS_TOTEXTURE_BIT) == NE_RENDERPASS_TOTEXTURE_BIT) {
            rpInfo.resolveImageViews.resize(mSwapchainImageCount);
            rpInfo.resolveImages.resize(mSwapchainImageCount);
        }
    }


    //Create a corresponding framebuffer for each image
    for (int i = 0; i < mSwapchainImageCount; i++) {
        createImage(FBSize, 1, rpInfo.depthImages[i], rpInfo.depthImageViews[i], VK_IMAGE_ASPECT_DEPTH_BIT, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, mDevice->sampleCount());
        createImage(FBSize, 1, rpInfo.colorImages[i], rpInfo.colorImageViews[i], VK_IMAGE_ASPECT_COLOR_BIT, format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, mDevice->sampleCount());

        if((flags & NE_RENDERPASS_TOTEXTURE_BIT) == NE_RENDERPASS_TOTEXTURE_BIT) {
            createImage(FBSize, 1, rpInfo.resolveImages[i], rpInfo.resolveImageViews[i], VK_IMAGE_ASPECT_COLOR_BIT,
                        format, static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
                        VK_SAMPLE_COUNT_1_BIT);

            mGUI->addRenderSpace(rpInfo.resolveImageViews[i], mSimpleSampler);
        }

        VkImageView attachments[3];
        attachments[0] = rpInfo.colorImageViews[i];
        attachments[1] = rpInfo.depthImageViews[i];
        if(MSAA) {
            attachments[2] = rpInfo.resolveImageViews[i];
        }

        fb_info.pAttachments = attachments;
        fb_info.attachmentCount = MSAA ? 3 : 2;

        if(vkCreateFramebuffer(mDevice->device(), &fb_info, nullptr, &rpInfo.frameBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer\n");
        };
    }
}

void NEDisplay::resizeFrameBuffer(VkExtent2D extent, uint32_t flags) {
    vkDeviceWaitIdle(mDevice->device());

    VkFormat format;
    if((flags & NE_RENDERPASS_TOSWAPCHAIN_BIT) == NE_RENDERPASS_TOSWAPCHAIN_BIT) {
        format = mFormat;
    }
    else {
        mAspect = (extent.height / extent.width);
        format = VK_FORMAT_R8G8B8A8_SRGB;
    }
    std::cout << "Resizing Framebuffer\n";
    RenderPassInfo& rpInfo = mRenderPasses[flags];

    for (int i = 0; i < mSwapchainImageCount; i++) {
        vkDestroyFramebuffer(mDevice->device(), rpInfo.frameBuffers[i], nullptr);

        vkDestroyImageView(mDevice->device(), rpInfo.colorImageViews[i], nullptr);
        vmaDestroyImage(mDevice->allocator(), rpInfo.colorImages[i].mImage, rpInfo.colorImages[i].mAllocation);

        vkDestroyImageView(mDevice->device(), rpInfo.depthImageViews[i], nullptr);
        vmaDestroyImage(mDevice->allocator(), rpInfo.depthImages[i].mImage, rpInfo.depthImages[i].mAllocation);

        if((flags & NE_RENDERPASS_TOTEXTURE_BIT) == NE_RENDERPASS_TOTEXTURE_BIT) {
            vkDestroyImageView(mDevice->device(), rpInfo.resolveImageViews[i], nullptr);
            vmaDestroyImage(mDevice->allocator(), rpInfo.resolveImages[i].mImage, rpInfo.resolveImages[i].mAllocation);
        }
    }

    createFramebuffers(extent, format, flags, true);
}

void NEDisplay::createSyncStructures(FrameData &frame) {

    VkFenceCreateInfo fenceCreateInfo = init::fenceCreateInfo();
    //Create it already signaled
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateFence(mDevice->device(), &fenceCreateInfo, nullptr, &frame.mRenderFence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create synchronization fences\n");
    }

    //Queue fence for eventual deletion
    mDeletionQueue.push_function([=, this]() {
        vkDestroyFence(mDevice->device(), frame.mRenderFence, nullptr);
    });



    VkSemaphoreCreateInfo semaphoreCreateInfo = init::semaphoreCreateInfo();

    if (vkCreateSemaphore(mDevice->device(), &semaphoreCreateInfo, nullptr, &frame.mPresentSemaphore) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create present semaphore\n");
    }
    if (vkCreateSemaphore(mDevice->device(), &semaphoreCreateInfo, nullptr, &frame.mRenderSemaphore) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render semaphore\n");
    }

    mDeletionQueue.push_function([=, this]() {
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

        mDeletionQueue.push_function([=, this]() {
            vkDestroyCommandPool(mDevice->device(), mFrames[i].mCommandPool, nullptr);
        });
    }
}

void NEDisplay::addTexture(Texture& tex, uint32_t dstIdx) {
    VkWriteDescriptorSet writes[MAX_FRAMES];
    if(tex.mSampler == VK_NULL_HANDLE)
        tex.mSampler = mDevice->createSampler(tex.mMipLevels);
    for(uint32_t i = 0; i < MAX_FRAMES; i++) {
        VkDescriptorImageInfo descriptorImageInfo;
        descriptorImageInfo.sampler = tex.mSampler;
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfo.imageView = tex.mImageView;
        writes[i] = init::writeDescriptorImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mFrames[i].mTextureDescriptor, &descriptorImageInfo, 0);
        writes[i].dstArrayElement = dstIdx;
    }
    vkUpdateDescriptorSets(mDevice->device(), MAX_FRAMES, writes, 0, nullptr);
}

void NEDisplay::setupBindRenderpass(VkCommandBuffer cmd, uint32_t flags, VkExtent2D extent) {
    //Clear framebuffer
    VkClearValue colorClear;
    colorClear.color = { { 0.01f, 0.01f, 0.01f, 1.f}};

    VkClearValue depthClear;
    depthClear.depthStencil.depth = 1.f;

    //Lets start painting
    VkRenderPassBeginInfo rpInfo = {};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.pNext = nullptr;

    RenderPassInfo& RPDetails = mRenderPasses[flags];

   // std::cout << "Ex: W: " << extent.width << "Ex: H: " << extent.height << std::endl;

    rpInfo.renderPass = RPDetails.renderPass;
    rpInfo.renderArea.offset.x = 0;
    rpInfo.renderArea.offset.y = 0;
    rpInfo.renderArea.extent = extent;
    rpInfo.framebuffer = RPDetails.frameBuffers[mSwapchainImageIndex];

    //Set the value to clear to in renderpass
    rpInfo.clearValueCount = 3;
    VkClearValue clearValues[] = { colorClear, depthClear, colorClear};
    rpInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void NEDisplay::setPipelineDynamics(VkCommandBuffer cmd, VkExtent2D extent) {
    VkRect2D scissor = {0,0, extent.width, extent.height};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    VkViewport viewport;
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = extent.width;
    viewport.height = extent.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);
}

VkCommandBuffer NEDisplay::startFrame() {
    FrameData &frame = mFrames[mCurrentFrame];

    //Wait for frame to be ready/(returned to "back")
    vkWaitForFences(mDevice->device(), 1, &frame.mRenderFence, true, 1000000000);
    vkResetFences(mDevice->device(), 1, &frame.mRenderFence);

    //Get current swapchain index && result
    VkResult result = vkAcquireNextImageKHR(mDevice->device(), mSwapchain, 1000000000, frame.mPresentSemaphore, nullptr, &mSwapchainImageIndex);

    vkResetCommandBuffer(frame.mCommandBuffer, 0);

    mGUI->checkFrameBuffers(mDevice->device());
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return startFrame();
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }


    //Wipe and prep command buffer to be handed to the renderer

    VkCommandBufferBeginInfo cmdBeginInfo = init::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    vkBeginCommandBuffer(frame.mCommandBuffer, &cmdBeginInfo);

    VkExtent2D texExtent = mGUI->getRenderWindowSize();
    //Bind the first renderpass that we will draw the entity's/objects with.
    setupBindRenderpass(frame.mCommandBuffer, NE_RENDERPASS_TOTEXTURE_BIT, texExtent);
    setPipelineDynamics(frame.mCommandBuffer, texExtent);

    char* sceneData;
    vmaMapMemory(mDevice->allocator(), mSceneParameterBuffer.mAllocation, (void**)&sceneData);
    sceneData += mDevice->padUniformBufferSize(sizeof(GPUSceneData)) * mCurrentFrame;
    memcpy(sceneData, &mSceneData, sizeof(GPUSceneData));
    vmaUnmapMemory(mDevice->allocator(), mSceneParameterBuffer.mAllocation);

    //Farewell command buffer o/; May your errors gentle.
    return frame.mCommandBuffer;
}

void NEDisplay::endFrame() {
    FrameData &frame = mFrames[mCurrentFrame];
    mGUI->drawGui(mSwapchainImageIndex);

    vkCmdEndRenderPass(frame.mCommandBuffer);

    setupBindRenderpass(frame.mCommandBuffer, NE_RENDERPASS_TOSWAPCHAIN_BIT, mExtent);

    ImGui::Begin("Environment");
    ImGui::Text("Environment Color");
    ImGui::SliderFloat("R", &mSceneData.ambientColor.x, -1.0f, 1.0f);
    ImGui::SliderFloat("G", &mSceneData.ambientColor.y, -1.0f, 1.0f);
    ImGui::SliderFloat("B", &mSceneData.ambientColor.z, -1.0f, 1.0f);
    ImGui::End();

    ////Prepare image to show to SwapChain

    ImGui::Render();

    //Though wise men at their end know dark is right,
    //Because their words had forked no lightning they
    //Do not go gentle into that good night.

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frame.mCommandBuffer);
    vkCmdEndRenderPass(frame.mCommandBuffer);
    vkEndCommandBuffer(frame.mCommandBuffer);

    VkSubmitInfo submit = init::submitInfo(&frame.mCommandBuffer, 1);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit.pWaitDstStageMask = &waitStage;

    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &frame.mPresentSemaphore;

    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &frame.mRenderSemaphore;


    //Submit command buffer and execute it.
    vkQueueSubmit(mDevice->graphicsQueue(), 1, &submit, frame.mRenderFence);

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;

    presentInfo.pSwapchains = &mSwapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores = &frame.mRenderSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &mSwapchainImageIndex;


   VkResult result = vkQueuePresentKHR(mDevice->graphicsQueue(), &presentInfo);

   if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
       recreateSwapchain();
   } else if (result != VK_SUCCESS) {
       throw std::runtime_error("failed to present swap chain image!");
   }

    //Rage, rage against the dying of the light.
    //Should flippy flop between 0 and MAX_FRAMES - 1
   mCurrentFrame = (mCurrentFrame + 1) % (MAX_FRAMES);
   mFrameCount++;
}

bool NEDisplay::shouldExit() {
    if (!glfwWindowShouldClose(mWindow)) {
        return false;
    } else {
        return true;
    }
}

void NEDisplay::toggleFullscreen() {
    if(!mFullScreen) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* vidMode = glfwGetVideoMode(monitor);

        glfwSetWindowMonitor(mWindow, monitor, 0, 0, vidMode->width, vidMode->height, vidMode->refreshRate);
        mFullScreen = true;
    }
    else {
        glfwSetWindowMonitor(mWindow, NULL, 0, 0, 800, 600, 0);
        mFullScreen = false;
    }
}

void NEDisplay::finishInit() {
    //Create simple 1 mip generic sampler
    mSimpleSampler = mDevice->createSampler();

    //Create both RenderPasses
    mRenderPasses[NE_RENDERPASS_TOSWAPCHAIN_BIT].renderPass = mDevice->createRenderpass(mFormat, NE_RENDERPASS_TOSWAPCHAIN_BIT | NE_RENDERPASS_MSAA8x_BIT);
    mRenderPasses[NE_RENDERPASS_TOTEXTURE_BIT].renderPass = mDevice->createRenderpass(VK_FORMAT_R8G8B8A8_SRGB, NE_RENDERPASS_TOTEXTURE_BIT | NE_RENDERPASS_MSAA8x_BIT);
    createDescriptors();
    initImGUI();

    createFramebuffers(mExtent, mFormat, NE_RENDERPASS_TOSWAPCHAIN_BIT, true);
    createFramebuffers(mExtent, VK_FORMAT_R8G8B8A8_SRGB, NE_RENDERPASS_TOTEXTURE_BIT, true);
}

VkRenderPass NEDisplay::swapchainPass() {return mRenderPasses[NE_RENDERPASS_TOSWAPCHAIN_BIT].renderPass;}
VkRenderPass NEDisplay::texturePass() {return mRenderPasses[NE_RENDERPASS_TOTEXTURE_BIT].renderPass;}
FrameData NEDisplay::currentFrame() {return mFrames[mCurrentFrame];}
VkSurfaceKHR NEDisplay::surface() {return mSurface;}
VkFormat NEDisplay::format() {return mFormat;}
VkExtent2D NEDisplay::extent() {return mExtent;}
GLFWwindow *NEDisplay::window() {return mWindow;}
uint32_t NEDisplay::frameIndex() {return mCurrentFrame;}
float NEDisplay::aspect() {return mAspect;}
std::shared_ptr<NEDevice> NEDisplay::device() {return mDevice;}
VkDescriptorPool NEDisplay::guiDescriptorPool() {return mGuiDescriptorPool;}
