//
// Created by nineball on 5/30/21.
//

#include "EntityManager.h"

EntityManager & EntityManager::Get() {
    return instance;
}

void EntityManager::addEntity(Entity* entity) {
    entityList.push_back(entity);
}

void EntityManager::removeEntity(Entity* entity) {
    entityList.erase(std::remove(entityList.begin(), entityList.end(), entity), entityList.end());
}

void EntityManager::addRenderer(Vulkan *vulkan_) {
    vulkan = vulkan_;
}

Entity::Entity() {
    EntityManager::Get().addEntity(this);
}

Entity::~Entity() {
    EntityManager::Get().removeEntity(this);
}
