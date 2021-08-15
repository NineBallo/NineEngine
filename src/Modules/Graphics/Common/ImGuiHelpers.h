//
// Created by nineball on 7/31/21.
//

#ifndef NINEENGINE_IMGUIHELPERS_H
#define NINEENGINE_IMGUIHELPERS_H
#include <string>
#include <glm/glm.hpp>
#include <imgui.h>
#include "ECS.h"
#include "Display.h"
#include "Engine.h"
class NEDisplay;
class Engine;

class NEGUI {
public:
    NEGUI(NEDisplay* display);
    ~NEGUI();

    void wipeRenderSpace();
    void addRenderSpace(VkImageView imageView, VkSampler sampler);

    void drawGui(uint16_t currentFrame);
    void checkFrameBuffers(VkDevice device);

   VkExtent2D getRenderWindowSize();

private:
    void drawMenuBar();
    void drawEntityListPanel();
    void drawEntityPanel();
    void setupDockSpace();
    void drawTimings();
    void drawRenderSpace(uint16_t currentFrame);


private:
    ECS& mECS;
    Engine& mEngine;

    Entity mFocusedEntity {0};
    bool mMenuBarEnabled {true};
    bool mEntityPanelEnabled {false};
    bool mEntityListPanelEnabled {true};
    bool mStatisticPanelEnabled {true};
    bool mDockSpaceEnabled {true};
    bool mTimingsEnabled {true};
    bool mRenderOutputEnabled {true};

private:
    bool mFrameBufferExpired {false};
    ImVec2 mLastRenderWindowSize;
    ImVec2 mNextRenderWindowSize;

private:
    NEDisplay* mDisplay;

    //Subscribe Data
    std::array<std::array<Entity, MAX_ENTITYS>, MAX_DISPLAYS> mLocalEntityList;
    uint32_t mEntityListSize = 0;
    std::array<std::pair<Display, Entity>, MAX_ENTITYS> mEntityToPos;

    enum mComponents {
        camera,
        position,
    };

    Signature mCameraSignature;
    Signature mPositionSignature;

    std::pair<Entity, uint32_t> mSelected;

    ImGuiContext* mContext;

private:
    std::vector<ImTextureID> mRenderTexture;

private:
    //Timings
    std::chrono::time_point<std::chrono::steady_clock> currentTick, lastTick;
    std::vector<float> mFrameTimes;
    uint32_t mFrameCount;
};



#endif //NINEENGINE_IMGUIHELPERS_H
