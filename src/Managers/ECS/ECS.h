//
// Created by nineball on 7/12/21.
//

#ifndef NINEENGINE_ECS_H
#define NINEENGINE_ECS_H

#include <array>
#include <queue>
#include <tuple>
#include <memory>
#include <unordered_map>
#include <bitset>
#include <iostream>
#include "Common.h"


template <typename T>
constexpr auto type_name() noexcept {
    std::string_view name = "Error: unsupported compiler", prefix, suffix;

    name = __PRETTY_FUNCTION__;
    prefix = "auto type_name() [T = ";
    suffix = "]";

    name.remove_prefix(prefix.size());
    name.remove_suffix(suffix.size());
    return name;
}

///All subscriber modules will extend this as they all require these variables
class Module {
protected:
    Module() = default;

    //Packed array for fast iteration
    std::array<std::array<Entity, MAX_ENTITYS>, MAX_DISPLAYS> mLocalEntityList{};

    //Current size / index of last entity + 1
    uint32_t mEntityListSize = 0;

    //Map for packed array, may be unnecessary
    std::array<std::pair<Display, Entity>, MAX_ENTITYS> mEntityToPos{};
};
///Subscribers will submit this
///Redundant but creates a faster interface as I dont have to lookup the vtable
struct SubscribeData {
    //Packed array for fast iteration
    std::array<std::array<Entity, MAX_ENTITYS>, MAX_DISPLAYS>* localEntityList = nullptr;

    //Current size/index of last entity + 1
    uint32_t * size = nullptr;

    //Map for packed array, may be unnecessary
    std::array<std::pair<Display, Entity>, MAX_ENTITYS>* entityToPos = nullptr;
};

class ComponentBase {
public:
    virtual void entityDestroyed(Entity entity) = 0;
    virtual ~ComponentBase() = default;
};

template<typename T>
class ComponentArray : public ComponentBase {
public:
    T& getComponent(Entity entityID) {
        uint32_t index = entityToIndex[entityID];
        return componentArray[index];
    }

    void setComponent(Entity entityID, T component) {

        //Update maps for packed array
        indexToEntity[arraySize] = entityID;
        entityToIndex[entityID] = arraySize;

        //Actually copy in the new component
        componentArray[arraySize] = component;

        //Account for new size
        arraySize++;
    }

    void removeComponent(Entity entityID) {

        //Index of deleted component in packed array
        uint32_t index = entityToIndex[entityID];
        uint32_t lastIndex = arraySize - 1;

        //Copy last element into the deleted elements place to retain packed array
        componentArray[index] = componentArray[lastIndex];

        //Get entityID for last component
        Entity lastEntity = indexToEntity[lastIndex];

        //Update map for moved entity to account for the moved component
        entityToIndex[lastEntity] = index;
    }

    void entityDestroyed(Entity entityID) override {
        //Remove component

        if(entityToIndex[entityID]) {
            removeComponent(entityID);
        }
    }

private:
    //Required to insure packed array as componentArray's index's are not necessarily 1:1 to the represented entity
    std::array<uint32_t, MAX_ENTITYS> entityToIndex;
    std::array<uint32_t, MAX_ENTITYS> indexToEntity;
    std::array<T, MAX_ENTITYS> componentArray;

    uint32_t arraySize;
};

class ECS {
public:
    static ECS& Get() {
        static ECS ecs;
        return ecs;
    };

    ///Entity handler
    Entity createEntity(Display display) {
        Entity newEntity;
        u_int32_t index = displaySize[display];

        if(!oldEntitys.empty()) {
            //Grab next available entity
            newEntity = oldEntitys.front();


            //Reserve entity
            oldEntitys.pop();
        }
        else {
            newEntity = index;
        }

        //Get display, then add newEntity to end of packed array
        displays[display][index] = newEntity;

        //Log index&display that entity is residing on
        entityToPos[newEntity] = {display, index};

        //Update new size of packed array
        displaySize[display]++;

        return newEntity;
    };

    bool destroyEntity(Entity entity) {

        ///Step 1: Remove entityID from allocated list and repack array
        //Get Display and Index
        std::tuple<Display, u_int32_t> location = entityToPos[entity];
        Display display = std::get<0>(location);
        u_int32_t index = std::get<1>(location);

        //Safety stuff
        if(displays[display][index] != entity) {
            std::runtime_error("Entity called for deletion does not exist");
            return false;
        }

        u_int32_t size = displaySize[display];

        //Overwrite entity if not at back to insure packed array remains
        displays[display][entity] = displays[display][size];

        //Reduce display size as there is one less entity
        displaySize[display]--;

        //Add it back to the list of available entity's
        oldEntitys.push(entity);

        ///Step 2: deallocate all components

        for (auto& componentPair : componentArrays) {
            auto& component = componentPair.second;

            component->entityDestroyed(entity);
        }
        return true;
    };

    ///Component Handler
    template<typename T>
    T& getComponent(Entity entityID) {
        return getComponentArray<T>()->getComponent(entityID);
    };

    template<typename T>
    Component getComponentType() {
        std::string_view typeName = type_name<T>();

        return componentTypes[typeName];
    };

    Signature getEntitySignature(Entity entity) {
        return entitySignatures[entity];
    };

    template<typename T>
    void addComponent(Entity entityID, T component) {

        getComponentArray<T>()->setComponent(entityID, component);

        std::string_view typeName = type_name<T>();

        Signature signature = entitySignatures[entityID];
        signature.set(componentTypes[typeName], true);
        updateEntitySignature(entityID, signature);
    };

    template<typename T>
    bool removeComponent(Entity entityID) {
        getComponentArray<T>()->removeComponent(entityID);

        std::string_view typeName = type_name<T>();

        Signature signature = entitySignatures[entityID];
        signature.set(componentTypes[typeName], false);
        updateEntitySignature(entityID, signature);
        return true;
    };

    template<typename T>
    std::shared_ptr<ComponentArray<T>> getComponentArray() {
        std::string_view typeName = type_name<T>();

        return std::static_pointer_cast<ComponentArray<T>>(componentArrays[typeName]);
    };

    template<typename T>
    void registerComponent() {
        //Get name of type
        std::string_view typeName = type_name<T>();

        componentTypes.insert({typeName, nextComponentID});
        componentArrays.insert({typeName, std::make_shared<ComponentArray<T>>()});

        nextComponentID++;
    };

    ///System Handler
    template<typename T>
    void registerSystem(SubscribeData subscriber) {

        //Convert type to name
        System typeName = type_name<T>();
        systems.insert({typeName, subscriber});
    };

    template<typename T>
    void setSystemSignature(Signature signature) {

        System typeName = type_name<T>();
        systemSignatures[typeName] = signature;

        updateSystemSignature(signature, typeName);
    };

    void updateSystemSignature(Signature signature, System system) {
        //Create a "3D" iterator so that we can check each entity and see if it matches the changed system
        //This is a relatively expensive operation, so it should be only really used on signature set.
        SubscribeData data = systems[system];

        for (Display display = 0; display < MAX_DISPLAYS; display++) {
            for(Entity i = 0; i < displaySize[display]; i++) {
                Entity entity = displays[display][i];

                std::pair<uint32_t, uint32_t> allegedPosition = (*data.entityToPos)[entity];

                if((*data.localEntityList)[allegedPosition.first][allegedPosition.second] != entity || (*data.size) == 0) {
                    if ((entitySignatures[entity] & signature) == signature) {
                        (*data.localEntityList)[0][(*data.size)] = entity;
                        (*data.entityToPos)[entity] = std::pair(0, (*data.size));
                        (*data.size)++;
                    }
                }
            }
        }
    }

    void updateEntitySignature(Entity entity, Signature entitySig) {

        entitySignatures[entity] = entitySig;

        //Update System lists
        for(auto& pair : systems) {

            auto const& type = pair.first;
            auto const& system = pair.second;
            auto const& systemSig = systemSignatures[type];

            std::pair<Display, uint32_t> pos = entityToPos[entity];
            Display display = std::get<0>(pos);

            //Test if entity already exists on system and exit if it does
            Display allegedDisplay = std::get<0>((*system.entityToPos)[entity]);
            uint32_t allegedIndex = std::get<1>((*system.entityToPos)[entity]);

            //Check if we are potentially updating or adding entity
            if ((*system.localEntityList)[allegedDisplay][allegedIndex] == entity) {
                if(!((entitySig & systemSig) == systemSig)) {

                    (*system.size)--;
                    uint32_t index = std::get<1>((*system.entityToPos)[entity]);

                    //Get Entity in back
                    uint32_t backEntity = (*system.size);

                    //Repack array
                    (*system.localEntityList)[display][index] = (*system.localEntityList)[display][backEntity];

                    //Update maps to keep track of entity
                    (*system.entityToPos)[entity] = {display, *system.size};
                }
            }
            ////Entity must include needed signatures
            else if((entitySig & systemSig) == systemSig) {
                uint32_t newIndex = *system.size;

                (*system.localEntityList)[display][newIndex] = entity;
                (*system.entityToPos)[entity] = {display, newIndex};
                (*system.size)++;
            }
        }
    };

private:

    ///Entity's
    //Sorted by display, packed array of all active Entity's.
    // This is what will be passed to the renderer and other systems to iterate through.
    std::array<std::array<Entity, MAX_ENTITYS>, MAX_DISPLAYS> displays{};

    //Display + Index
    std::array<std::pair<Display, Entity>, MAX_ENTITYS> entityToPos{};

    //Log of last entity/size of each "display"
    std::array<uint32_t, MAX_DISPLAYS> displaySize{};

    //Queue of available entity id's
    std::queue<Entity> oldEntitys{};

private:

    ///Modules/Systems
    std::unordered_map<System, Signature> systemSignatures{};
    std::unordered_map<System, SubscribeData> systems{};
    std::unordered_map<Entity, Signature> entitySignatures{};


private:
    ///Components
    Component nextComponentID {0};
    std::unordered_map<System, Component> componentTypes{};
    std::unordered_map<System, std::shared_ptr<ComponentBase>> componentArrays{};


private:

    ECS() {
            std::cout << "MAX_ENTITYS: " << MAX_ENTITYS << std::endl;
    };
};



#endif //NINEENGINE_ECS_H
