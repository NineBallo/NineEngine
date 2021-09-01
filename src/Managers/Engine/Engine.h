//
// Created by nineball on 6/19/21.
//

#ifndef NINEENGINE_ENGINE_H
#define NINEENGINE_ENGINE_H

#include <optional>
#include <string>
#include "ECS.h"


using settingVal = uint32_t;

class Engine {
public:
    Engine(const Engine&) = delete;

    static Engine& Get(){
        static Engine engine;
        return engine;
    }


public:
    //std::shared_ptr<Graphics> getVKRenderer();

    uint32_t createEntity(const std::string& modelPath, std::string texturePath);
    uint32_t createEntity(std::string modelPath);
    uint32_t createEntity();

    bool destroyEntity();
    bool unloadEntity(uint32_t entity);
    bool loadEntity(uint32_t entity);

    bool updateEntity(std::string modelPath, std::string texturePath);
    bool updateEntityModel(std::string modelPath);
    bool updateEntityTexture(std::string texturePath);

    void startEngine();

    std::string getRendererType();

    uint32_t getSetting(uint32_t key);
    void setSetting(uint32_t key, uint32_t value);


    enum Renderers {
        VK,
    };

#define settingCount 5
    enum Settings {
        Renderer,
        DisplayCount,
        FrameCount,
        MSAA,
        Editor,
    };

private:

    Engine();
    //This holds every setting for the engine in a key-value pair, ideally it is persistent.
    ///TODO make persistant
    std::array<settingVal, 5> mSettingToPos;
    std::array<uint32_t, 5> mPosToSetting;

    std::array<settingVal, 5> mSettings;


    ECS& mECS {ECS::Get()};
};


#endif //NINEENGINE_ENGINE_H
