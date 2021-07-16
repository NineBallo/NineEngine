////
//// Created by nineball on 7/12/21.
////
//
//#include "ECS.h"
//#include "iostream"
//
//
//template <typename T>
//std::array<T, MAX_ENTITYS> ComponentArray<T>::componentArray;
//
//Module::Module() = default;;
//
//ECS &ECS::Get() {
//    static ECS ecs;
//    return ecs;
//}
//
/////Entity Stuff
//
//Entity ECS::createEntity(Display display) {
//    //Grab next available entity
//    Entity newEntity = availableEntitys.front();
//    u_int32_t index = displaySize[display];
//
//    //Reserve entity
//    availableEntitys.pop();
//
//    //Get display, then add newEntity to end of packed array
//    displays[display][index] = newEntity;
//
//    //Log index&display that entity is residing on
//    entityToPos[newEntity] = {display, index};
//
//    //Update new size of packed array
//
//    displaySize[display]++;
//}
//
//bool ECS::destroyEntity(Entity entity) {
//
//    ///Step 1: Remove entityID from allocated list and repack array
//    //Get Display and Index
//    std::tuple<Display, u_int32_t> location = entityToPos[entity];
//    Display display = std::get<0>(location);
//    u_int32_t index = std::get<1>(location);
//
//    //Safety stuff
//    if(displays[display][index] != entity) {
//        std::runtime_error("Entity called for deletion does not exist");
//        return false;
//    }
//
//    u_int32_t size = displaySize[display];
//
//    //Overwrite entity if not at back to insure packed array remains
//    displays[display][entity] = displays[display][size];
//
//    //Reduce display size as there is one less entity
//    displaySize[display]--;
//
//    //Add it back to the list of available entity's
//    availableEntitys.push(entity);
//
//
//    ///Step 2: deallocate all components
//
//    for (auto& componentPair : componentArrays) {
//        auto& component = componentPair.second;
//
//        component->entityDestroyed(entity);
//    }
//}
//
//
//std::array<std::array<u_int32_t, MAX_ENTITYS>, MAX_DISPLAYS>&  ECS::getEntityList() {
//    return displays;
//}
//
//
/////Component stuff
//template<typename T>
//T& ECS::getComponent(Entity entityID) {
//    return ComponentArray<T>::getComponent(entityID);
//}
//
//template<typename T>
//Component ECS::getComponentType() {
//    const char* typeName = typeid(T).name();
//
//    return componentTypes[typeName];
//}
//
//template<typename T>
//void ECS::addComponent(Entity entityID, T component) {
//    ComponentArray<T>::setComponent(entityID, component);
//
//
//    const char* typeName = typeid(T).name();
//    Signature signature = entitySignatures[entityID];
//    signature.set(componentTypes[typeName]);
//    updateEntitySignature(entityID, signature);
//}
//
//template<typename T>
//bool ECS::removeComponent(Entity entityID) {
//    ComponentArray<T>::removeComponent(entityID);
//}
//
//template<typename T>
//void ECS::registerComponent() {
//
//    //Get name of type
//    const char *typeName = typeid(T).name();
//    componentArrays.insert({typeName, std::make_shared<ComponentArray<T>>});
//    componentTypes.insert({typeName, nextComponentID});
//    nextComponentID++;
//}
//
//template<typename T>
//std::shared_ptr<ComponentArray<T>> ECS::getComponentArray() {
//
//    const char* typeName = typeid(T).name();
//
//    return std::static_pointer_cast<ComponentArray<T>>(componentArrays[typeName]);
//}
//
//
//template<typename T>
//void ECS::registerSystem(SubscribeData subscriber) {
//    //Convert type to name
//    const char* typeName = typeid(T).name();
//
//    systems.insert({typeName, subscriber});
//}
//
//template <typename T>
//void ECS::setSystemSignature(Signature signature) {
//    const char* typeName = typeid(T).name();
//
//
//    systemSignatures[typeName] = signature;
//}
//
//
//void ECS::updateEntitySignature(Entity entity, Signature entitySig) {
//
//    entitySignatures[entity] = entitySig;
//
//    //Update System lists
//    for(auto& pair : systems) {
//
//        auto const& type = pair.first;
//        auto const& system = pair.second;
//        auto const& systemSig = systemSignatures[type];
//
//        std::pair<Display, uint32_t> pos = entityToPos[entity];
//        Display display = std::get<0>(pos);
//
//        //Test if entity already exists on system and exit if it does
//        Display allegedDisplay = std::get<0>((*system.entityToPos)[entity]);
//        uint32_t allegedIndex = std::get<1>((*system.entityToPos)[entity]);
//        if ((*system.localEntityList)[allegedIndex][allegedDisplay] == entity) {
//            return;
//        }
//
//        if (systemSig.test(MAX_COMPONENTS + 1)) {
//            //Entity must include needed signatures
//            if((entitySig & systemSig) == systemSig) {
//
//                (*system.localEntityList)[display][*system.size] = entity;
//                (*system.entityToPos)[entity] = {display, *system.size};
//                (*system.size)++;
//            }
//        }
//    }
//}
