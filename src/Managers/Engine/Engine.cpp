//
// Created by nineball on 6/19/21.
//
#include <iostream>
#include <utility>


#include "Engine.h"

//void Engine::setRenderer(const std::string& renderer) {
//    if (renderer == "VK")
//    {
//        //VKRenderer.emplace();
//    }
//    else {
//        std::cout << "Bad Renderer create \n";
//    }
//}
//
//std::string Engine::getRendererType() {
//    if(VKRenderer.has_value())
//    {
//        return "VK";
//    }
//    else {
//        return "NONE";
//    }
//}
//
//NERenderer* Engine::getVKRenderer() {
//    if (VKRenderer.has_value())
//    {
//        return &VKRenderer.value();
//    }
//    else {
//        std::cout << "Graphics engine requested but none found \n";
//        return nullptr;
//    }
//}
//
//uint32_t Engine::createEntity() {
//    std::string rendererType = Engine::Get().getRendererType();
//    Coordinator& coordinator = Coordinator::Get();
//
//    if(rendererType == "VK") {
//        uint32_t entity = Coordinator::Get().CreateEntity(0);
//        Coordinator::Get().AddComponent(entity, VKRenderer->loadEntityObjects("Textures/image0.jpg", ""));
//        Coordinator::Get().AddComponent(entity, Transform {
//            .position = {0, 0, 0},
//            .rotation = {0, 0, 0},
//            .scale = 1
//        });
//        return entity;
//    }
//}
//
//uint32_t Engine::createEntity(std::string modelPath) {
//    std::string rendererType = Engine::Get().getRendererType();
//    Coordinator& coordinator = Coordinator::Get();
//
//    if(rendererType == "VK") {
//        uint32_t entity = Coordinator::Get().CreateEntity(0);
//        Coordinator::Get().AddComponent(entity, VKRenderer->loadEntityObjects("Textures/image0.jpg", std::move(modelPath)));
//        Coordinator::Get().AddComponent(entity, Transform {
//                .position = {0, 0, 0},
//                .rotation = {0, 0, 0},
//                .scale = 1
//        });
//        return entity;
//    }
//}