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

#define MAX_ENTITYS 1000
#define MAX_DISPLAYS 5
#define MAX_COMPONENTS 10
#define MAX_SYSTEMS 15
#define MAX_MODULES 2

using Entity = uint32_t;
using Display = uint8_t;
using Signature = std::bitset<MAX_COMPONENTS>;

using Component = std::string_view;
using ComponentID = unsigned short;

using System = std::string_view;
using SystemID = uint32_t;


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

///Subscribers will submit this
///Redundant but creates a faster interface as I dont have to lookup the vtable
struct SubscribeData {
    //Packed array for fast iteration
    std::array<Entity, MAX_ENTITYS>* localEntityList = nullptr;

    //Current size/index of last entity + 1
    uint32_t * size = nullptr;

    //Map for packed array, may be unnecessary
    std::array<uint32_t, MAX_ENTITYS>* entityToPos = nullptr;

    //display used will be (display - 1) display 1 will be a ref to index 0, and display 0 will be a ref to all.
    uint32_t display = 0;


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

        //TODO check if this impacts performance as I dont think its necessary
        indexToEntity[entityToIndex[entityID]] = 0;
        entityToIndex[entityID] = 0;
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

        //Grab next available entity
        Entity newEntity = availableEntitys.front();
        u_int32_t index = displaySize[display];

        //Reserve entity
        availableEntitys.pop();

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
        availableEntitys.push(entity);

        ///Step 2: deallocate all components

        for (auto& component : componentArrays) {
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
    SystemID registerSystem(SubscribeData subscriber) {

        systems[systemSize] = subscriber;
        return systemSize++;
    };


    void setSystemSignature(SystemID systemID, Signature signature) {


        systemSigs[systemID] = signature;

        updateSystemSignature(signature, systemID);
    };

    void updateSystemSignature(Signature signature, SystemID systemID) {
        //Create a "3D" iterator so that we can check each entity and see if it matches the changed system
        //This is a relatively expensive operation, so it should be only really used on signature set.
        SubscribeData data = systems[system];

        for (Display display = 0; display < MAX_DISPLAYS; display++) {
            for(Entity i = 0; i < displaySize[display]; i++) {
                Entity entity = displays[display][i];

                uint32_t allegedPosition = (*data.entityToPos)[entity];

                if((*data.localEntityList)[allegedPosition] != entity || (*data.size) == 0) {
                    if ((entitySignatures[entity] & signature) == signature) {
                        (*data.localEntityList)[(*data.size)] = entity;
                        (*data.entityToPos)[entity] = (*data.size);
                        (*data.size)++;
                    }
                }
            }
        }
    }

    void updateEntitySignature(Entity entity, Signature entitySig) {

        entitySignatures[entity] = entitySig;

        //Update System lists
        for(uint32_t i = 0; i < systemSize; i++) {

            auto const& system = systems[i];
            auto const& systemSig = systemSigs[i];

            std::pair<Display, uint32_t> pos = entityToPos[entity];
            Display display = std::get<0>(pos);

            //Test if entity already exists on system and exit if it does
            uint32_t allegedIndex = (*system.entityToPos)[entity];

            //Check if we are potentially updating or adding entity
            if ((*system.localEntityList)[allegedIndex] == entity) {
                if(!((entitySig & systemSig) == systemSig)) {

                    (*system.size)--;
                    uint32_t index = (*system.entityToPos)[entity];

                    //Get Entity in back
                    uint32_t backEntity = (*system.size);

                    //Repack array
                    (*system.localEntityList)[index] = (*system.localEntityList)[backEntity];

                    //Update maps to keep track of entity
                    (*system.entityToPos)[entity] = (*system.size);
                }
            }
            ////Entity must include needed signatures
            else if((entitySig & systemSig) == systemSig) {
                uint32_t newIndex = *system.size;

                (*system.localEntityList)[newIndex] = entity;
                (*system.entityToPos)[entity] = newIndex;
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
    std::queue<Entity> availableEntitys{};

private:
    ///Modules/Systems
    //Sig pos == sys position
    std::array<Signature, MAX_SYSTEMS> systemSigs{};

    //Packed array of SubscribeData pointers
    std::array<SubscribeData, MAX_SYSTEMS> systems{};

    //The systemID will map to an index
    std::array<uint32_t, MAX_SYSTEMS> sysToPos{};

    //The index will map to a systemID
    std::array<SystemID , MAX_SYSTEMS> posToSys{};
    uint32_t systemSize {0};

    std::unordered_map<Entity, Signature> entitySignatures{};

private:
    ///Components
    uint32_t nextComponentID {0};

    //Component typename maps to a component signature
    std::array<Component, MAX_COMPONENTS> componentTypes{};

    //key = sysid, value = class that holds a list of one component types
    std::array<std::shared_ptr<ComponentBase>, MAX_COMPONENTS> componentArrays{};

private:

    ECS() {
            for (int entity = 0; entity < MAX_ENTITYS; ++entity)
            {
                availableEntitys.push(entity);
            }
            std::cout << "MAX_ENTITYS: " << MAX_ENTITYS << std::endl;
    };
};



#endif //NINEENGINE_ECS_H
