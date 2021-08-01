//
// Created by nineball on 7/31/21.
//
#include "ImGuiHelpers.h"

#include "imgui.h"

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

    mContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(mContext);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ////TODO get multisampling working, and rendering to vkimage so it can be displayed by imgui
}

NEGUI::~NEGUI() {

}


void NEGUI::tick() {
    ImGui::SetCurrentContext(mContext);
    if(mMenuBarEnabled) {
        drawMenuBar();
    }
    if(mEntityListPanelEnabled) {
        drawEntityListPanel();
    }
    if(mEntityPanelEnabled) {
        drawEntityPanel();
    }
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

    }
}

void NEGUI::drawEntityPanel() {
    if(mEntityPanelEnabled) {

    }
}