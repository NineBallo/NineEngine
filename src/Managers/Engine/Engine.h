//
// Created by nineball on 6/19/21.
//

#ifndef NINEENGINE_ENGINE_H
#define NINEENGINE_ENGINE_H

#include <optional>
#include <string>
#include "ECS.h"
#include "Vulkan.h"
class Vulkan;

class Engine {
public:
    Engine(const Engine&) = delete;

    static Engine& Get(){
        static Engine engine;
        return engine;
    }



public:

    std::shared_ptr<Vulkan> getVKRenderer();

    uint32_t createEntity(const std::string& modelPath, std::string texturePath);
    uint32_t createEntity(std::string modelPath);
    uint32_t createEntity();

    void startEngine();



    uint32_t getSetting(uint32_t key);
    void setSetting(uint32_t key, uint32_t value);

    enum Renderers {
        VK,
    };

#define SettingCount 5
    enum Settings {
        Renderer,
        DisplayCount,
        FrameCount,
        MSAA,
        Editor,
        FullScreen,
    };

private:
    Engine();
    //This holds every setting for the engine in a key-value pair, ideally it is persistent.
    std::array<uint32_t, SettingCount> mSettings;

    
    std::shared_ptr<Vulkan> VKRenderer;
    ECS& mECS {ECS::Get()};
};


#endif //NINEENGINE_ENGINE_H
