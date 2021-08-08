//
// Created by nineball on 7/31/21.
//
#include "ImGuiHelpers.h"

#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_glfw.h"

NEGUI::NEGUI(NEDisplay* display) : mECS {ECS::Get()} {
    mDisplay = display;

    SubscribeData sd {
        .localEntityList = &mLocalEntityList,
        .size = &mEntityListSize,
        .entityToPos = &mEntityToPos,
    };

    mECS.registerSystem<NEGUI>(sd);

    //We want every entity
    Signature emptySig;
    mECS.setSystemSignature<NEGUI>(emptySig);

    mPositionSignature.set(mECS.getComponentType<Position>(), true);
    mCameraSignature.set(mECS.getComponentType<Camera>(), true);

    mContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(mContext);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    mFrameTimes.reserve(300);
}

NEGUI::~NEGUI() {

}

void NEGUI::addRenderSpace(VkImageView imageView, VkSampler sampler) {


    ImTextureID texture = ImGui_ImplVulkan_AddTexture(sampler, imageView,
                                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    mRenderTexture.push_back(texture);
    mRenderOutputEnabled = true;
}

void NEGUI::wipeRenderSpace() {
    for(auto tex : mRenderTexture) {
        auto set = (VkDescriptorSet)(tex);
        vkFreeDescriptorSets(mDisplay->device()->device(), mDisplay->guiDescriptorPool(), 1, &set);
    }
    mRenderTexture.clear();
    mRenderOutputEnabled = false;
}


void NEGUI::drawGui(uint16_t currentFrame) {
    ImGui::SetCurrentContext(mContext);
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    ImGui::NewFrame();
    ImGui::StyleColorsDark();
    if(mDockSpaceEnabled) {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
    }
    if(mRenderOutputEnabled) {
        drawRenderSpace(currentFrame);
    }
    if(mMenuBarEnabled) {
        drawMenuBar();
    }
    if(mEntityListPanelEnabled) {
        drawEntityListPanel();
    }
    if(mEntityPanelEnabled) {
        drawEntityPanel();
    }
    if(mTimingsEnabled) {
        drawTimings();
    }
}

void NEGUI::checkFrameBuffers(VkDevice device) {
    if(mFrameBufferExpired) {
        vkDeviceWaitIdle(device);
        wipeRenderSpace();

        VkExtent2D extent {
            .width = static_cast<uint32_t>(mNextRenderWindowSize.x),
            .height = static_cast<uint32_t>(mNextRenderWindowSize.y),
            };

        mDisplay->resizeFrameBuffer(extent, NE_RENDERPASS_TOTEXTURE_BIT);

        mFrameBufferExpired = false;

        mLastRenderWindowSize = mNextRenderWindowSize;

        auto& camera = mECS.getComponent<Camera>(0);
        camera.aspect = mNextRenderWindowSize.x / mNextRenderWindowSize.y;
    }
}

void NEGUI::drawRenderSpace(uint16_t currentFrame) {
    ImGui::Begin("Texture Test", nullptr , ImGuiWindowFlags_NoScrollbar);
    ImVec2 image = ImGui::GetWindowSize();

    if(mLastRenderWindowSize.x != image.x || mLastRenderWindowSize.y != image.y) {
        std::cout << "X: " << image.x << "Y: " << image.y << std::endl;
        mFrameBufferExpired = true;

        mNextRenderWindowSize = image;
    }

    image.y -= 35;
    ImGui::Image((void*)mRenderTexture[currentFrame], image);
    ImGui::End();
}

void NEGUI::drawMenuBar() {
    if(ImGui::BeginMainMenuBar()) {
        if(ImGui::BeginMenu("Options")) {
            if(ImGui::MenuItem("FullScreen")) {
                mDisplay->toggleFullscreen();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void NEGUI::drawEntityListPanel() {
    if(mEntityListPanelEnabled) {
        if(ImGui::Begin("Entity List")) {
            for(Entity i = 0; i < mEntityListSize; i++) {
                Entity entity = mLocalEntityList[0][i];

                if(ImGui::TreeNode(std::to_string(entity).c_str())) {
                    Signature entitySig = mECS.getEntitySignature(entity);

                    if((entitySig & mCameraSignature) == mCameraSignature) {
                        if(ImGui::Selectable("Camera")) {
                            mSelected = {entity, camera};
                            mEntityPanelEnabled = true;
                        }
                    }
                    if((entitySig & mPositionSignature) == mPositionSignature) {
                        if(ImGui::Selectable("Position")) {
                            mSelected = {entity, position};
                            mEntityPanelEnabled = true;
                        }
                    }


                    ImGui::TreePop();
                }
            }

        } ImGui::End();
    }
}


void NEGUI::drawEntityPanel() {
    if(mEntityPanelEnabled) {
        if(ImGui::Begin("Entity Editor Panel")) {
            std::string entityName = "Entity:" + std::to_string(mSelected.first);
            ImGui::Text("%s", entityName.c_str());

            if(mSelected.second == position) {
                auto& component = mECS.getComponent<Position>(mSelected.first);

                ImGui::Text("Position Component");
                ImGui::Text("Translation");
                ImGui::SliderFloat("Coordinate X", &component.coordinates.x, 0.f, 1000.f);
                ImGui::SliderFloat("Coordinate Y", &component.coordinates.y, 0.f, 1000.f);
                ImGui::SliderFloat("Coordinate Z", &component.coordinates.z, 0.f, 1000.f);

                ImGui::Text("Rotation");
                ImGui::SliderFloat("Pitch", &component.rotations.x, -90.f, 90.f);
                ImGui::SliderFloat("Yaw", &component.rotations.y, 0.f, 359.f);
                ImGui::SliderFloat("Roll", &component.rotations.z, 0.f, 359.f);

                ImGui::Text("Scalar");
                ImGui::SliderFloat("Scalar X", &component.scalar.x, 0.f, 1.f);
                ImGui::SliderFloat("Scalar Y", &component.scalar.y, 0.f, 1.f);
                ImGui::SliderFloat("Scalar Z", &component.scalar.z, 0.f, 1.f);
            }
            else if(mSelected.second == camera) {
                auto& component = mECS.getComponent<Camera>(mSelected.first);

                ImGui::Text("Camera Component");

                ImGui::Text("Postitions");
                ImGui::SliderFloat("Pitch", &component.Angle.x, -90.f, 90.f);
                ImGui::SliderFloat("Yaw", &component.Angle.y, 0.f, 359.f);
                ImGui::SliderFloat("Roll", &component.Angle.z, 0.f, 359.f);

                ImGui::Text("Camera Settings");
                ImGui::SliderFloat("Aspect", &component.aspect, 0.1f, 3.f);
                ImGui::SliderFloat("Degrees", &component.degrees, 0.1f, 90.f);
                ImGui::SliderFloat("ZFar", &component.zfar, 0.1f, 1000.f);
                ImGui::SliderFloat("ZNear", &component.znear, 0.0f, 1000.f);
            }

        } ImGui::End();
    }
}

void NEGUI::drawTimings() {
    currentTick = std::chrono::steady_clock::now();
    std::chrono::duration<float> duration = std::chrono::duration_cast<std::chrono::duration<float>>(currentTick - lastTick);
    float seconds = duration.count();
    lastTick = currentTick;

    float min = 1, max = 0;
    if(ImGui::Begin("Timings")) {

        float roundedFrametime = 0;
        uint16_t sampleFrames = 10;

        if(mFrameTimes.size() > 300) {
            for(uint32_t i = 0; i < mFrameTimes.size(); i++) {
                //Shift every element down one.
                mFrameTimes[i] = mFrameTimes[i+1];

                //Get max value
                if(mFrameTimes[i] > max) {
                    max = mFrameTimes[i];
                }
                //Get min value
                if(mFrameTimes[i] < min && mFrameTimes[i] > 0.00001f) {
                    min = mFrameTimes[i];
                }

            }
            mFrameTimes[mFrameTimes.size() - 1] = seconds;

            for(uint8_t i = 0; i < sampleFrames; ++i) {
                roundedFrametime += mFrameTimes[mFrameTimes.size() - i];
            }

        }
        else {
            mFrameTimes.push_back(seconds);

        }

        roundedFrametime /= sampleFrames;

        std::string FPS = "Average FPS: " + std::to_string((uint32_t)(std::floor(1/roundedFrametime)));
        ImGui::Text("%s", FPS.c_str());
        ImGui::Text("FrameTimes:");
        std::string timings = "Max: " + std::to_string(max) + " Min: " + std::to_string(min);
        ImGui::Text("%s", timings.c_str());


        ImGui::PlotLines("", &mFrameTimes[0], mFrameTimes.size(),
                             0, NULL, 0, max, {300, 70});


    } ImGui::End();
}


VkExtent2D NEGUI::getRenderWindowSize() {
    VkExtent2D extent {
        .width = static_cast<uint32_t>(mLastRenderWindowSize.x),
        .height = static_cast<uint32_t>(mLastRenderWindowSize.y),
        };

    return extent;
}