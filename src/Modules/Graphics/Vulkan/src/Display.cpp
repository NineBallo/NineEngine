//
// Created by nineball on 7/6/21.
//

#include "../../Common/ImGuiHelpers.h"

#include <iostream>
#include <VkBootstrap.h>
#include <cmath>
#include <Initializers.h>


#include "Display.h"
#include "ECS.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
//Circular include issues..


NEDisplay::NEDisplay(const displayCreateInfo& createInfo) {

    SubscribeData subscribeData {
            .localEntityList = &mLocalEntityList,
            .size = &mEntityListSize,
            .entityToPos = &mEntityToPos,
    };

    ECS::Get().registerSystem<NEDisplay>(subscribeData);

    Signature systemSig {};
    systemSig.set(ECS::Get().getComponentType<RenderObject>());
    systemSig.set(ECS::Get().getComponentType<Position>());

    ECS::Get().setSystemSignature<NEDisplay>(systemSig);

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

///Init methods
void NEDisplay::startInit(const std::shared_ptr<NEDevice>& device) {
    mDevice = device;
    createSwapchain(VK_PRESENT_MODE_IMMEDIATE_KHR);
}

void NEDisplay::finishInit() {
    //Create simple 1 mip generic sampler
    mSimpleSampler = mDevice->createSampler();

    createDescriptors();
    initImGUI();

    //Create both RenderPasses
    createFramebuffers(mExtent, mFormat, NE_RENDERMODE_TOSWAPCHAIN_BIT, true);
    createFramebuffers(mExtent, VK_FORMAT_R8G8B8A8_SRGB, NE_RENDERMODE_TOTEXTURE_BIT, true);
    createFramebuffers(mExtent, VK_FORMAT_R8G8B8A8_SRGB, NE_RENDERMODE_TOSHADOWMAP_BIT, true);





    for (int i = 0; i < MAX_FRAMES; i++) {
        VkDescriptorImageInfo shadowImageInfo;
        shadowImageInfo.sampler = mDevice->getSampler(0);
        shadowImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        shadowImageInfo.imageView = mFrameBufferList[NE_RENDERMODE_TOSHADOWMAP_BIT].depthImageViews[i];

        VkWriteDescriptorSet shadowWrites = init::writeDescriptorImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                                       mFrames[i].mDirectionalShadowDescriptor,
                                                                       &shadowImageInfo, 0);
        vkUpdateDescriptorSets(mDevice->device(), 1, &shadowWrites, 0, nullptr);
    }

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

void NEDisplay::tick() {
    if(!shouldExit()) {
        drawframe();
    } else {
        std::cout << "You didnt code it to exit lmao\n";
    }
}


///Runtime external methods
void NEDisplay::startRender() {


}

VkCommandBuffer NEDisplay::startFrame(Flags renderType) {
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
        return startFrame(renderType);
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }


    //Wipe and prep command buffer to be handed to the renderer

    VkCommandBufferBeginInfo cmdBeginInfo = init::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    vkBeginCommandBuffer(frame.mCommandBuffer, &cmdBeginInfo);

    VkExtent2D texExtent = mGUI->getRenderWindowSize();
    //Bind the first renderpass that we will draw the entity's/objects with.
    setupBindRenderpass(frame.mCommandBuffer, renderType, texExtent);
    setPipelineDynamics(frame.mCommandBuffer, texExtent);

    char* sceneData;
    vmaMapMemory(mDevice->allocator(), mSceneParameterBuffer.mAllocation, (void**)&sceneData);
    sceneData += mDevice->padUniformBufferSize(sizeof(GPUSceneData)) * mCurrentFrame;
    memcpy(sceneData, &mSceneData, sizeof(GPUSceneData));
    vmaUnmapMemory(mDevice->allocator(), mSceneParameterBuffer.mAllocation);

    //Farewell command buffer o/; May your errors gentle.
    return frame.mCommandBuffer;
}

void NEDisplay::setupCameraPosition(Camera camera) {
    FrameData& frame = mFrames[mCurrentFrame];

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
}

glm::mat4 NEDisplay::setupLightPosition(glm::vec3 lightPos) {


    FrameData& frame = mFrames[mCurrentFrame];//x = pitch, y = yaw, z = roll

    glm::mat4 lightProj = glm::ortho(-10.f, 10.f, -10.f, 10.f, 1.0f, 128.f);

    glm::mat4 lightView = glm::lookAt(glm::vec3(-2.0f, 4.0f, -1.0f),
                                      glm::vec3( 0.0f, 0.0f,  0.0f),
                                      glm::vec3( 0.0f, 1.0f,  0.0f));


    glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians((float)(mExtent.width/mExtent.height)), 1.0f, 1.f, 128.f);
    glm::mat4 depthViewMatrix = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));

   // lightProj[1][1] *= -1;
    GPUCameraData cameraData {};
    cameraData.proj = lightProj;
    cameraData.view = lightView;
    cameraData.viewproj = lightProj * lightView;

    void* data;
    vmaMapMemory(mDevice->allocator(), frame.mCameraBuffer.mAllocation, &data);
    memcpy(data, &cameraData, sizeof(GPUCameraData));
    vmaUnmapMemory(mDevice->allocator(), frame.mCameraBuffer.mAllocation);

    return lightProj;
}

void NEDisplay::drawframe() {
    FrameData& frame = mFrames[mCurrentFrame];

    ///Calculate all positions and send to gpu
    void* objectData;
    vmaMapMemory(mDevice->allocator(), frame.mObjectBuffer.mAllocation, &objectData);

    auto* objectSSBO = (GPUObjectData*)objectData;

    for (int i = 0; i < mEntityListSize; i++)
    {
        Entity currentEntityID = mLocalEntityList[0][i];
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


    //Start shadowmap
    VkCommandBuffer cmd = startFrame(NE_RENDERMODE_TOSHADOWMAP_BIT);

    Camera lightPOV;
    lightPOV.Pos = mSceneData.sunlightDirection;
    lightPOV.aspect = (mExtent.width / mExtent.height);
    lightPOV.zfar = 128.f;
    lightPOV.znear = 1.f;
    lightPOV.Angle.x = -90;

    //setupCameraPosition(lightPOV);
    glm::mat4 lightPov = setupLightPosition(mSceneData.sunlightDirection);
    //setupCameraPosition(ECS::Get().getComponent<Camera>(0));

    drawEntities(cmd, NE_RENDERMODE_TOSHADOWMAP_BIT, NE_FLAG_SHADOW_BIT);
//
    ////Finish shadowmap
    vkCmdEndRenderPass(cmd);

    //Start main render
    setupBindRenderpass(cmd, NE_RENDERMODE_TOTEXTURE_BIT, mGUI->getRenderWindowSize());

    setupCameraPosition(ECS::Get().getComponent<Camera>(0));

    drawEntities(cmd, NE_RENDERMODE_TOTEXTURE_BIT, NE_FLAG_TEXTURE_BIT | NE_FLAG_MSAA8x_BIT);

    endFrame();
}

void NEDisplay::drawEntities(VkCommandBuffer cmd, Flags rendermode, Flags features) {
    FrameData& frame = mFrames[mCurrentFrame];

    Flags mLastFlags = 0;
    TextureID mLastTexture = MAX_TEXTURES + 1;

    for(Entity i = 0; i < mEntityListSize; i++) {
        Entity currentEntityID = mLocalEntityList[0][i];
        auto& currentEntity = ECS::Get().getComponent<RenderObject>(currentEntityID);
        MeshGroup& meshGroup = *currentEntity.meshGroup;

        auto pipelineInfo = mDevice->getPipeline(rendermode, features);
        if(currentEntity.features != mLastFlags) {

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInfo.first);
            mLastFlags = currentEntity.features;


            VkDescriptorSet globalDescriptorSet = frame.mGlobalDescriptor;
            VkDescriptorSet objectDescriptorSet = frame.mObjectDescriptor;
            VkDescriptorSet textureDescriptorSet = frame.mTextureDescriptor;

            //object data descriptor
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInfo.second,
                                    1, 1, &objectDescriptorSet, 0, nullptr);

            uint32_t uniform_offset = mDevice->padUniformBufferSize(sizeof(GPUSceneData)) * frameIndex();
            uniform_offset = 0;
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInfo.second,
                                    0, 1, &globalDescriptorSet, 1, &uniform_offset);



            if(mDevice->bindless()) {
                //texture descriptor
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        pipelineInfo.second, 3, 1,
                                        &textureDescriptorSet, 0, nullptr);


            }
        }

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInfo.second,
                                2, 1, &frame.mDirectionalShadowDescriptor, 0, nullptr);


        for(auto & mesh : meshGroup.mMeshes) {
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.mVertexBuffer.mBuffer, &offset);
            vkCmdBindIndexBuffer(cmd, mesh.mIndexBuffer.mBuffer, 0 , VK_INDEX_TYPE_UINT32);

            PushData pushData{};
            pushData.entityID = currentEntityID;

            //Get texture for mesh
            uint32_t texIdx = meshGroup.mMatToIdx[mesh.mMaterial];
            TextureID texID = meshGroup.mTextures[texIdx];


            if(mDevice->bindless()) {
                pushData.textureIndex = getTextureBinding(texID);
            }
            else if(mLastTexture != texID) {
                mLastTexture = texID;
                VkDescriptorSet textureSet = mDevice->getTexture(texID).mTextureSet;
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        pipelineInfo.second, 3, 1,
                                        &textureSet, 0, nullptr);
            }


            vkCmdPushConstants(cmd, pipelineInfo.second,
                               VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,0, sizeof(PushData), &pushData);


            vkCmdDrawIndexed(cmd, mesh.mIndices.size(), 1, 0, 0, 0);
        }
    }
}

void NEDisplay::endFrame() {
    FrameData &frame = mFrames[mCurrentFrame];
    vkCmdEndRenderPass(frame.mCommandBuffer);

    setupBindRenderpass(frame.mCommandBuffer, NE_RENDERMODE_TOSWAPCHAIN_BIT, mExtent);

    mGUI->drawGui(mSwapchainImageIndex);

    ImGui::Begin("Environment");
    ImGui::Text("Environment Color");
    ImGui::SliderFloat("R", &mSceneData.ambientColor.x, 0.f, 0.3f);
    ImGui::SliderFloat("G", &mSceneData.ambientColor.y, 0.f, 0.3f);
    ImGui::SliderFloat("B", &mSceneData.ambientColor.z, 0.f, 0.3f);

    ImGui::Text("Sunlight Color");
    ImGui::SliderFloat("R###1", &mSceneData.sunlightColor.x, 0.f, 1.0f);
    ImGui::SliderFloat("G###2", &mSceneData.sunlightColor.y, 0.f, 1.0f);
    ImGui::SliderFloat("B###3", &mSceneData.sunlightColor.z, 0.f, 1.0f);
    ImGui::SliderFloat("Strength", &mSceneData.sunlightDirection.w, 0.f, 5.0f);

    ImGui::Text("Sunlight Direction");
    ImGui::SliderFloat("X", &mSceneData.sunlightDirection.x, -100.0f, 100.0f);
    ImGui::SliderFloat("Y", &mSceneData.sunlightDirection.y, -100.0f, 100.0f);
    ImGui::SliderFloat("Z", &mSceneData.sunlightDirection.z, -100.0f, 100.0f);

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
        vkDestroySwapchainKHR(mDevice->device(), mSwapchain, nullptr);
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


        if(MSAA) {
            fbInfo.resolveImageViews.resize(mSwapchainImageCount);
            fbInfo.resolveImages.resize(mSwapchainImageCount);
        }
    }

    bool ToTex = (flags & NE_RENDERMODE_TOTEXTURE_BIT) == NE_RENDERMODE_TOTEXTURE_BIT;
    bool ToShadow = (flags & NE_RENDERMODE_TOSHADOWMAP_BIT) == NE_RENDERMODE_TOSHADOWMAP_BIT;
    bool ToSC = (flags & NE_RENDERMODE_TOSWAPCHAIN_BIT) == NE_RENDERMODE_TOSWAPCHAIN_BIT;
    VkImageView attachments[3];
    uint8_t attachmentCount {};

    //Create a corresponding framebuffer for each image
    for (int i = 0; i < mSwapchainImageCount; i++) {




        createImage(FBSize, 1, fbInfo.depthImages[i], fbInfo.depthImageViews[i],
                    VK_IMAGE_ASPECT_DEPTH_BIT, VK_FORMAT_D32_SFLOAT,
                    static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
                    ToShadow? VK_SAMPLE_COUNT_1_BIT : mDevice->sampleCount());

        if(!ToShadow) {
            createImage(FBSize, 1, fbInfo.colorImages[i], fbInfo.colorImageViews[i], VK_IMAGE_ASPECT_COLOR_BIT, format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, mDevice->sampleCount());

            attachments[0] = fbInfo.colorImageViews[i];
            attachments[1] = fbInfo.depthImageViews[i];
            attachmentCount = 2;

            if(ToTex) {
                createImage(FBSize, 1, fbInfo.resolveImages[i], fbInfo.resolveImageViews[i], VK_IMAGE_ASPECT_COLOR_BIT,
                            format, static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
                            VK_SAMPLE_COUNT_1_BIT);

            }

            attachments[2] = fbInfo.resolveImageViews[i];
            attachmentCount++;


            if(ToTex) {
                mGUI->addRenderSpace(fbInfo.resolveImageViews[i], mSimpleSampler);
            }
        } else {
            attachments[0] = fbInfo.depthImageViews[i];
            attachmentCount = 1;
        }


        fb_info.pAttachments = attachments;
        fb_info.attachmentCount = attachmentCount;

        if(vkCreateFramebuffer(mDevice->device(), &fb_info, nullptr, &fbInfo.frameBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer\n");
        }
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
    }

    FrameBufferInfo& fbInfo = mFrameBufferList[flags];

    for (int i = 0; i < mSwapchainImageCount; i++) {
        vkDestroyFramebuffer(mDevice->device(), fbInfo.frameBuffers[i], nullptr);

        vkDestroyImageView(mDevice->device(), fbInfo.colorImageViews[i], nullptr);
        vmaDestroyImage(mDevice->allocator(), fbInfo.colorImages[i].mImage, fbInfo.colorImages[i].mAllocation);

        vkDestroyImageView(mDevice->device(), fbInfo.depthImageViews[i], nullptr);
        vmaDestroyImage(mDevice->allocator(), fbInfo.depthImages[i].mImage, fbInfo.depthImages[i].mAllocation);

        if((flags & NE_RENDERMODE_TOTEXTURE_BIT) == NE_RENDERMODE_TOTEXTURE_BIT) {
            vkDestroyImageView(mDevice->device(), fbInfo.resolveImageViews[i], nullptr);
            vmaDestroyImage(mDevice->allocator(), fbInfo.resolveImages[i].mImage, fbInfo.resolveImages[i].mAllocation);
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

        mFrames[i].mDirectionalShadowDescriptor = mDevice->createDescriptorSet(mDevice->shadowSetLayout());

        ///TODO finish shadow discriptor

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

        std::vector<VkWriteDescriptorSet> setWrites = { cameraWrite, sceneWrite, objectWrite};

        if(mDevice->bindless()) mFrames[i].mTextureDescriptor = mDevice->createDescriptorSet(mDevice->textureSetLayout());


        vkUpdateDescriptorSets(mDevice->device(), setWrites.size(), setWrites.data(), 0, nullptr);

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

    mSwapchainQueue.push_function([=, this]() {
       // vkFreeCommandBuffers(mDevice->device(), commandPool, 1, &temp);
    });

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

void NEDisplay::addTexture(TextureID texID) {
    uint32_t nextBinding;

    if(!mOldTextures.empty()) {
        nextBinding = mOldTextures.front();
    }
    else {
        nextBinding = mBindingCount;
    }

    mBindingCount++;

    if(mDevice->bindless()) {
      addTextureBinding(texID, nextBinding);
    }

    mBindingsToTex[nextBinding] = texID;
    mTexToBindings[texID] = nextBinding;
}

void NEDisplay::addTextureBinding(TextureID texID, uint32_t binding) {
    VkWriteDescriptorSet writes[MAX_FRAMES];
    Texture tex = mDevice->getTexture(texID);

    if(tex.mSampler == VK_NULL_HANDLE)
        tex.mSampler = mDevice->getSampler(tex.mMipLevels);
    for(uint32_t i = 0; i < MAX_FRAMES; i++) {
        VkDescriptorImageInfo descriptorImageInfo;
        descriptorImageInfo.sampler = tex.mSampler;
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfo.imageView = tex.mImageView;
        writes[i] = init::writeDescriptorImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mFrames[i].mTextureDescriptor, &descriptorImageInfo, 0);
        writes[i].dstArrayElement = binding;
    }
    vkUpdateDescriptorSets(mDevice->device(), MAX_FRAMES, writes, 0, nullptr);
}

uint32_t NEDisplay::getTextureBinding(TextureID tex) {
    return mTexToBindings[tex];
}


void NEDisplay::deleteTexture(TextureID texID) {

    uint32_t lastBinding = --mBindingCount;
    TextureID lastTexID = mBindingsToTex[lastBinding];
    uint32_t oldBinding = mTexToBindings[texID];

    if(texID != lastTexID) {

        //Move last binding into old/deleted bindings slot (CPU)
        mTexBindings[oldBinding] = mTexBindings[lastBinding];

        if(mDevice->bindless()) {
            //Move last binding into old/deleted bindings slot (GPU)
            addTextureBinding(lastTexID, oldBinding);
        }


        //Update Maps
        mTexToBindings[lastTexID] = oldBinding;
        mBindingsToTex[oldBinding] = lastTexID;

        mOldTextures.push(texID);
    }
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
    if((flags & NE_RENDERMODE_TOSHADOWMAP_BIT) == NE_RENDERMODE_TOSHADOWMAP_BIT) {
        rpInfo.clearValueCount = 1;
        VkClearValue clearValues[] = {depthClear};
        rpInfo.pClearValues = clearValues;
    }
    else {
        rpInfo.clearValueCount = 3;
        VkClearValue clearValues[] = { colorClear, depthClear, colorClear};
        rpInfo.pClearValues = clearValues;
    }


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
