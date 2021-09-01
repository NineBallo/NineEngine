//
// Created by nineball on 7/6/21.
//

#include "../../Common/ImGuiHelpers.h"
#include "Display.h"
#include <iostream>
#include <VkBootstrap.h>
#include <cmath>
#include <utility>
#include <Initializers.h>
#include "ECS.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

//Circular include issues..
NEDisplay::NEDisplay(const displayCreateInfo& createInfo) : mECS{ECS::Get()} {
    createWindow(createInfo.extent, createInfo.title, createInfo.resizable);
    mTitle = createInfo.title;

    if(createInfo.instance != VK_NULL_HANDLE) {
        createSurface(createInfo.instance);
    }

    if(createInfo.device != nullptr && createInfo.presentMode) {
        createSwapchain(createInfo.presentMode);
    }
    else if (createInfo.device != nullptr) {
        createSwapchain(VK_PRESENT_MODE_IMMEDIATE_KHR);
    }//VK_PRESENT_MODE_FIFO_KHR

    mExtent = createInfo.extent;
    mInstance = createInfo.instance;

    mGUI = std::make_unique<NEGUI>(this);

    SData.display = 0;
    mECS.registerSystem(&SData);

    Signature systemSig {};
    systemSig.set(ECS::Get().getComponentType<RenderObject>());
    systemSig.set(ECS::Get().getComponentType<Position>());

    mECS.setSystemSignature(SData.systemRef, systemSig);
}

NEDisplay::~NEDisplay() {
    vkDeviceWaitIdle(mDevice->device());

    mSwapchainQueue.flush();
    vkDestroySwapchainKHR(mDevice->device(), mSwapchain, nullptr);
    cleanupFramebuffer(NE_RENDERMODE_TOTEXTURE_BIT);
    cleanupFramebuffer(NE_RENDERMODE_TOSWAPCHAIN_BIT);
    mDeletionQueue.flush();

    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

///Init methods
void NEDisplay::startInit(const std::shared_ptr<NEDevice>& device) {
    mDevice = device;
    createSwapchain(VK_PRESENT_MODE_IMMEDIATE_KHR);
}

void NEDisplay::finishInit() {
    //Create simple 1 mip generic sampler
    mSimpleSampler = mDevice->createSampler();

    mDeletionQueue.push_function([=, this]() {
        vkDestroySampler(mDevice->device(), mSimpleSampler, nullptr);
    });
    //Create both RenderPasses

    createDescriptors();
    initImGUI();

    createFramebuffers(mExtent, mFormat, NE_RENDERMODE_TOSWAPCHAIN_BIT, true);
    createFramebuffers(mExtent, VK_FORMAT_R8G8B8A8_SRGB, NE_RENDERMODE_TOTEXTURE_BIT, true);
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
    mDeletionQueue.push_function([=, this]() {
        vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
    });
}


///Swapchain/Framebuffer (re)creation
void NEDisplay::createSwapchain(VkPresentModeKHR presentMode) {

    int width, height;
    glfwGetWindowSize(mWindow, &width, &height);
    mExtent.width = width; mExtent.height = height;
    vkb::SwapchainBuilder swapchainBuilder{mDevice->GPU(), mDevice->device(), mSurface};
    vkb::Swapchain vkbSwapchain = swapchainBuilder
            .use_default_format_selection()
            .set_desired_present_mode(presentMode)
            .set_desired_extent(width, height)
            .set_old_swapchain(mOldSwapchain)
            .build()
            .value();


    FrameBufferInfo &FBInfo = mFrameBufferList[NE_RENDERMODE_TOSWAPCHAIN_BIT];

    mSwapchain = vkbSwapchain.swapchain;
    std::vector<VkImage> images = vkbSwapchain.get_images().value();
    FBInfo.resolveImageViews = vkbSwapchain.get_image_views().value();
    mFormat = vkbSwapchain.image_format;
    mPresentMode = presentMode;

    mSwapchainImageCount = images.size();
    for(uint32_t i = 0; i > mSwapchainImageCount; i++) {
        FBInfo.resolveImages[i].mImage = images[i];
    }

    mSwapchainQueue.push_function([=, this]() {
        vkDestroySwapchainKHR(mDevice->device(), mOldSwapchain, nullptr);
        mOldSwapchain = mSwapchain;
    });

    //Gonna need this later...
    populateFrameData();
}

void NEDisplay::createFramebuffers(VkExtent2D FBSize, VkFormat format, uint32_t flags, bool MSAA) {
    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = nullptr;

    FrameBufferInfo& fbInfo = mFrameBufferList[flags];
    fb_info.renderPass = mDevice->getRenderPass(flags, format);
    fb_info.attachmentCount = 1;
    fb_info.width = FBSize.width;
    fb_info.height = FBSize.height;
    fb_info.layers = 1;

    //Swapchain size
    fbInfo.frameBuffers = std::vector<VkFramebuffer>(mSwapchainImageCount);
    if(fbInfo.colorImages.size() != mSwapchainImageCount) {
        fbInfo.depthImages.resize(mSwapchainImageCount);
        fbInfo.depthImageViews.resize(mSwapchainImageCount);
        fbInfo.colorImages.resize(mSwapchainImageCount);
        fbInfo.colorImageViews.resize(mSwapchainImageCount);

        if((flags & NE_RENDERMODE_TOTEXTURE_BIT) == NE_RENDERMODE_TOTEXTURE_BIT) {
            fbInfo.resolveImageViews.resize(mSwapchainImageCount);
            fbInfo.resolveImages.resize(mSwapchainImageCount);
        }
    }


    //Create a corresponding framebuffer for each image
    for (int i = 0; i < mSwapchainImageCount; i++) {
        createImage(FBSize, 1, fbInfo.depthImages[i], fbInfo.depthImageViews[i], VK_IMAGE_ASPECT_DEPTH_BIT, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, mDevice->sampleCount());
        createImage(FBSize, 1, fbInfo.colorImages[i], fbInfo.colorImageViews[i], VK_IMAGE_ASPECT_COLOR_BIT, format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, mDevice->sampleCount());

        if((flags & NE_RENDERMODE_TOTEXTURE_BIT) == NE_RENDERMODE_TOTEXTURE_BIT) {
            createImage(FBSize, 1, fbInfo.resolveImages[i], fbInfo.resolveImageViews[i], VK_IMAGE_ASPECT_COLOR_BIT,
                        format, static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
                        VK_SAMPLE_COUNT_1_BIT);

            mGUI->addRenderSpace(fbInfo.resolveImageViews[i], mSimpleSampler);
        }

        VkImageView attachments[3];
        attachments[0] = fbInfo.colorImageViews[i];
        attachments[1] = fbInfo.depthImageViews[i];
        if(MSAA) {
            attachments[2] = fbInfo.resolveImageViews[i];
        }

        fb_info.pAttachments = attachments;
        fb_info.attachmentCount = MSAA ? 3 : 2;

        if(vkCreateFramebuffer(mDevice->device(), &fb_info, nullptr, &fbInfo.frameBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer\n");
        };
    }
}

void NEDisplay::resizeFrameBuffer(VkExtent2D extent, uint32_t flags) {
    vkDeviceWaitIdle(mDevice->device());

    VkFormat format;
    if((flags & NE_RENDERMODE_TOSWAPCHAIN_BIT) == NE_RENDERMODE_TOSWAPCHAIN_BIT) {
        format = mFormat;
    }
    else {
        format = VK_FORMAT_R8G8B8A8_SRGB;
        cleanupFramebuffer(flags);
    }

    createFramebuffers(extent, format, flags, true);
}

void NEDisplay::cleanupFramebuffer(uint32_t FBflags) {
    FrameBufferInfo& fbInfo = mFrameBufferList[FBflags];
    for (int i = 0; i < mSwapchainImageCount; i++) {

        vkDestroyFramebuffer(mDevice->device(), fbInfo.frameBuffers[i], nullptr);

        vkDestroyImageView(mDevice->device(), fbInfo.colorImageViews[i], nullptr);
        vmaDestroyImage(mDevice->allocator(), fbInfo.colorImages[i].mImage, fbInfo.colorImages[i].mAllocation);

        vkDestroyImageView(mDevice->device(), fbInfo.depthImageViews[i], nullptr);
        vmaDestroyImage(mDevice->allocator(), fbInfo.depthImages[i].mImage, fbInfo.depthImages[i].mAllocation);

        vkDestroyImageView(mDevice->device(), fbInfo.resolveImageViews[i], nullptr);

        if((FBflags & NE_RENDERMODE_TOTEXTURE_BIT) == NE_RENDERMODE_TOTEXTURE_BIT) {
            vmaDestroyImage(mDevice->allocator(), fbInfo.resolveImages[i].mImage, fbInfo.resolveImages[i].mAllocation);
        }
    }
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

void NEDisplay::drawFrame() {
    //TODO potential to add different pathways here so that branch complexity is marginally decreased per frame
   VkCommandBuffer cmd = startFrame();

   FrameData &frame = mFrames[mCurrentFrame];

   //Camera setup
   auto& camera = ECS::Get().getComponent<Camera>(mCameraEntity);
   //x = pitch, y = yaw, z = roll
   glm::vec3 angles = camera.Angle;

   glm::vec3 direction;
   direction.x = cos(glm::radians(angles.y)) * cos(glm::radians(angles.x));
   direction.y = sin(glm::radians(angles.x));
   direction.z = sin(glm::radians(angles.y)) * cos(glm::radians(angles.x));
   direction = glm::normalize(direction);

   glm::mat4 view = glm::lookAt(camera.Pos, camera.Pos + direction, glm::vec3(0, 1.f, 0));
   glm::mat4 projection = glm::perspective(glm::radians(camera.degrees), camera.aspect,
                                           camera.znear, camera.zfar);
   projection[1][1] *= -1;

   GPUCameraData cameraData {};
   cameraData.proj = projection;
   cameraData.view = view;
   cameraData.viewproj = projection * view;

   void* data;
   vmaMapMemory(mDevice->allocator(), frame.mCameraBuffer.mAllocation, &data);
   memcpy(data, &cameraData, sizeof(GPUCameraData));
   vmaUnmapMemory(mDevice->allocator(), frame.mCameraBuffer.mAllocation);

   ///Calculate all positions and send to gpu
   void* objectData;
   vmaMapMemory(mDevice->allocator(), frame.mObjectBuffer.mAllocation, &objectData);

   auto* objectSSBO = (GPUObjectData*)objectData;

   for (int i = 0; i < SData.size; i++)
   {
       Entity currentEntityID = SData.localEntityList[i];
       auto& position = ECS::Get().getComponent<Position>(currentEntityID);

       glm::mat4 model {1.0f};
       model = glm::translate(model, glm::vec3{position.coordinates});
       model = glm::rotate(model, position.rotations.x * (3.14f/180), {1.f, 0.f , 0.f});
       model = glm::rotate(model, position.rotations.y * (3.14f/180), {0.f, 1.f , 0.f});
       model = glm::rotate(model, position.rotations.z * (3.14f/180), {0.f, 0.f , 1.f});
       model = glm::scale(model, position.scalar);

       objectSSBO[currentEntityID].modelMatrix = model;
   }
   vmaUnmapMemory(mDevice->allocator(), frame.mObjectBuffer.mAllocation);

   Material* lastMaterial = nullptr;
   TextureID lastTexture;

   for(Entity i = 0; i < SData.size; i++) {
       Entity currentEntityID = SData.localEntityList[i];
       auto& currentEntity = ECS::Get().getComponent<RenderObject>(currentEntityID);
       MeshGroup& meshGroup = *currentEntity.meshGroup;

       auto pipelineInfo = mDevice->getPipeline(currentEntity.material->renderMode, currentEntity.material->features);
       if(currentEntity.material != lastMaterial) {

           vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInfo.first);
           lastMaterial = currentEntity.material;

           VkDescriptorSet globalDescriptorSet = frame.mGlobalDescriptor;
           VkDescriptorSet objectDescriptorSet = frame.mObjectDescriptor;
           VkDescriptorSet textureDescriptorSet = frame.mTextureDescriptor;

           //object data descriptor
           vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInfo.second,
                                   1, 1, &objectDescriptorSet, 0, nullptr);

           uint32_t uniform_offset = mDevice->padUniformBufferSize(sizeof(GPUSceneData)) * mCurrentFrame;
           uniform_offset = 0;
           vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInfo.second,
                                   0, 1, &globalDescriptorSet, 1, &uniform_offset);

           if(mDevice->bindless()) {
               //texture descriptor
               vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                       pipelineInfo.second, 2, 1,
                                       &textureDescriptorSet, 0, nullptr);
           }
       }

       for(auto & mesh : meshGroup.mMeshes) {
           VkDeviceSize offset = 0;
           vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.mVertexBuffer.mBuffer, &offset);
           vkCmdBindIndexBuffer(cmd, mesh.mIndexBuffer.mBuffer, 0 , VK_INDEX_TYPE_UINT32);

           TexPushData pushData{};
           pushData.entityID = currentEntityID;

           //Get texture for mesh
           TextureID texid = mesh.mMaterial.texture;

           if(mDevice->bindless()) {
               pushData.textureIndex = mTextureToBinding[texid];
           }
           else if(lastTexture != texid) {
               lastTexture = texid;

               Texture &texture = mDevice->getTexture(texid);
               vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                       pipelineInfo.second, 2, 1,
                                       &texture.mTextureSet, 0, nullptr);
           }


           vkCmdPushConstants(cmd, pipelineInfo.second,
                              VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,0, sizeof(TexPushData), &pushData);


           vkCmdDrawIndexed(cmd, mesh.mIndices.size(), 1, 0, 0, 0);
       }
   }
}

void NEDisplay::presentFrame() {
    endFrame();
}


///Runtime external methods
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
    setupBindRenderpass(frame.mCommandBuffer, NE_RENDERMODE_TOTEXTURE_BIT, texExtent);
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

    setupBindRenderpass(frame.mCommandBuffer, NE_RENDERMODE_TOSWAPCHAIN_BIT, mExtent);

    ImGui::Begin("Environment");
    ImGui::Text("Environment Color");
    ImGui::SliderFloat("R", &mSceneData.ambientColor.x, -1.0f, 1.0f);
    ImGui::SliderFloat("G", &mSceneData.ambientColor.y, -1.0f, 1.0f);
    ImGui::SliderFloat("B", &mSceneData.ambientColor.z, -1.0f, 1.0f);
    ImGui::End();

    ImGui::Render();

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
    //Should flip-flop between 0 and MAX_FRAMES - 1
    mCurrentFrame = (mCurrentFrame + 1) % (MAX_FRAMES);
    mFrameCount++;
}


void NEDisplay::createImage(VkExtent2D extent, uint32_t mipLevels, AllocatedImage &image, VkImageView &imageView, VkImageAspectFlagBits aspect, VkFormat format, VkImageUsageFlagBits usage, VkSampleCountFlagBits sampleCount) {

    VkImageCreateInfo img_info = init::image_create_info(format, usage, extent, mipLevels, sampleCount);

    VmaAllocationCreateInfo img_allocinfo = {};
    img_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    img_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if(vmaCreateImage(mDevice->allocator(), &img_info, &img_allocinfo, &image.mImage, &image.mAllocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Could not create/allocate image");
    }

    VkImageViewCreateInfo view_info = init::imageview_create_info(format, image.mImage, mipLevels, aspect);
    vkCreateImageView(mDevice->device(), &view_info, nullptr, &imageView);
}

void NEDisplay::recreateSwapchain() {
    vkDeviceWaitIdle(mDevice->device());
    mSwapchainQueue.flush();

    cleanupFramebuffer(NE_RENDERMODE_TOSWAPCHAIN_BIT);
    createSwapchain(mPresentMode);
    resizeFrameBuffer(mExtent, NE_RENDERMODE_TOSWAPCHAIN_BIT);
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

    ImGui_ImplVulkan_Init(&init_info, mDevice->getRenderPass(NE_RENDERMODE_TOSWAPCHAIN_BIT, mFormat));

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

        mSwapchainQueue.push_function([=, this]() {
            vkDestroyCommandPool(mDevice->device(), mFrames[i].mCommandPool, nullptr);
        });
    }
}

TextureID NEDisplay::loadTexture(std::string filePath, std::string name) {
    //Load texture into gpu memory
    TextureID texid = mDevice->loadTexture(std::move(filePath));
    Texture &texture = mDevice->getTexture(texid);
    texture.name = std::move(name);

    //Update descriptor sets
    VkWriteDescriptorSet writes[MAX_FRAMES];
    for(uint32_t i = 0; i < MAX_FRAMES; i++) {
        VkDescriptorImageInfo descriptorImageInfo;
        descriptorImageInfo.sampler = texture.mSampler;
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfo.imageView = texture.mImageView;
        writes[i] = init::writeDescriptorImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mFrames[i].mTextureDescriptor, &descriptorImageInfo, 0);
        writes[i].dstArrayElement = mTextureCount++;
    }
    vkUpdateDescriptorSets(mDevice->device(), MAX_FRAMES, writes, 0, nullptr);

    //Give the user their referenceID;
    return texid;
}

void NEDisplay::setupBindRenderpass(VkCommandBuffer cmd, uint32_t flags, VkExtent2D extent) {
    //Clear framebuffer
    VkClearValue colorClear;
    colorClear.color = { { 0.01f, 0.01f, 0.01f, 1.f}};

    VkClearValue depthClear;
    depthClear.depthStencil.depth = 1.f;

    //Let's start painting
    VkRenderPassBeginInfo rpInfo = {};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.pNext = nullptr;

    FrameBufferInfo& FBDetails = mFrameBufferList[flags];

    rpInfo.renderPass = mDevice->getRenderPass(flags);
    rpInfo.renderArea.offset.x = 0;
    rpInfo.renderArea.offset.y = 0;
    rpInfo.renderArea.extent = extent;
    rpInfo.framebuffer = FBDetails.frameBuffers[mSwapchainImageIndex];

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

VkRenderPass NEDisplay::texturePass() {return mDevice->getRenderPass(NE_RENDERMODE_TOTEXTURE_BIT);}
FrameData NEDisplay::currentFrame() {return mFrames[mCurrentFrame];}
VkSurfaceKHR NEDisplay::surface() {return mSurface;}
VkFormat NEDisplay::format() {return mFormat;}
VkExtent2D NEDisplay::extent() {return mExtent;}
GLFWwindow *NEDisplay::window() {return mWindow;}
uint32_t NEDisplay::frameIndex() {return mCurrentFrame;}
std::shared_ptr<NEDevice> NEDisplay::device() {return mDevice;}
VkDescriptorPool NEDisplay::guiDescriptorPool() {return mGuiDescriptorPool;}
