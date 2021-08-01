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
class NEDisplay;

class NEGUI {
public:
    NEGUI(NEDisplay* display);
    ~NEGUI();

   void tick();

   //void drawVec3(glm::vec3 vec3, std::string name, std::string x, std::string y, std::string z);
private:
    void drawMenuBar();
    void drawEntityListPanel();
    void drawEntityPanel();

private:
    ECS& mECS;

    Entity mFocusedEntity {0};
    bool mMenuBarEnabled {true};
    bool mEntityPanelEnabled {true};
    bool mEntityListPanelEnabled {true};
    bool mStatisticPanelEnabled {true};

private:
    Signature mPositionSignature;
    Signature mRenderObjectSignature;


private:
    NEDisplay* mDisplay;

    //Subscribe Data
    std::array<std::array<Entity, MAX_ENTITYS>, MAX_DISPLAYS> mLocalEntityList;
    uint32_t mEntityListSize = 0;
    std::array<std::pair<Display, Entity>, MAX_ENTITYS> mEntityToPos;

    ImGuiContext* mContext;
};



#endif //NINEENGINE_IMGUIHELPERS_H
