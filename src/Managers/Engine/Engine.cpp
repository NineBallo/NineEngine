//
// Created by nineball on 6/19/21.
//
#include <iostream>
#include <utility>

#include "Engine.h"


Engine::Engine() : mECS {ECS::Get()} {
    mSettings[Renderer] = VK;
    mSettings[DisplayCount] = 0;
    mSettings[MSAA] = VK_SAMPLE_COUNT_1_BIT;
    mSettings[Editor] = true;
}


std::shared_ptr<Vulkan> Engine::getVKRenderer() {
    if (mSettings[Renderer] == VK)
    {
        return VKRenderer;
    }
    else {
        std::cout << "Graphics engine requested but none found \n";
        return nullptr;
    }
}

uint32_t Engine::createEntity() {
    Entity newEntity = mECS.createEntity(0);
    return newEntity;
}

uint32_t Engine::createEntity(std::string modelPath) {
    if (mSettings[Renderer] == VK)
    {
        Entity newEntity = mECS.createEntity(0);
     //   VKRenderer->createMaterial(NE_SHADER_COLOR_BIT);
        VKRenderer->createMesh(modelPath, modelPath);
       // VKRenderer->makeRenderable(newEntity, NE_SHADER_COLOR_BIT, modelPath);
        return newEntity;
    }
    else {
        std::cout << "Graphics engine requested but none found \n";
        return 0;
    }
}

uint32_t Engine::createEntity(const std::string& modelPath, std::string texturePath) {
    if (mSettings[Renderer] == VK)
    {
        Entity newEntity = mECS.createEntity(0);
     //   VKRenderer->createMaterial(NE_SHADER_TEXTURE_BIT);
        VKRenderer->createMesh(modelPath, modelPath);
        VKRenderer->loadTexture(texturePath, texturePath);
     //   VKRenderer->makeRenderable(newEntity, NE_SHADER_COLOR_BIT, modelPath);
        return newEntity;
    }
    else {
        std::cout << "Graphics engine requested but none found \n";
        return 0;
    }
}

uint32_t Engine::getSetting(uint32_t key) {
    return mSettings[key];
}

void Engine::setSetting(uint32_t key, uint32_t value) {
    mSettings[key] = value;
}

void Engine::startEngine() {

}