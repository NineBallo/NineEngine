//
// Created by nineball on 6/19/21.
//

#ifndef NINEENGINE_ENGINE_H
#define NINEENGINE_ENGINE_H

#include <optional>
#include <string>
#include "../../Graphics/Vulkan/Renderer.h"



class Engine {
public:
    Engine(const Engine&) = delete;

    static Engine& Get(){
        static Engine engine;
        return engine;
    }



public:
    void setRenderer(const std::string& renderer);


    uint32_t createEntity(std::string modelPath, std::string texturePath);
    uint32_t createEntity(std::string modelPath);
    uint32_t createEntity();

    bool destroyEntity();
    bool unloadEntity(uint32_t entity);
    bool loadEntity(uint32_t entity);

    bool updateEntity(std::string modelPath, std::string texturePath);
    bool updateEntityModel(std::string modelPath);
    bool updateEntityTexture(std::string texturePath);


    std::string getRendererType();
    NERenderer* getVKRenderer();

private:
    Engine();

    std::optional<NERenderer> VKRenderer;
};


#endif //NINEENGINE_ENGINE_H
