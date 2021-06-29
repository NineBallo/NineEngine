//
// Created by nineball on 6/2/21.
//

#ifndef NINEENGINE_COORDINATOR_H
#define NINEENGINE_COORDINATOR_H


#include <memory>
#include "ComponentManager.h"
#include "EntityManager.h"
#include "SystemManager.h"

class Coordinator
{
public:

    Coordinator(const Coordinator&) = delete;

    static Coordinator& Get(){
        static Coordinator coordinator1;
        return coordinator1;
    }


    void Init()
    {
        // Create pointers to each manager
        mComponentManager = std::make_unique<ComponentManager>();
        mEntityManager = std::make_unique<EntityManager>();
        mSystemManager = std::make_unique<SystemManager>();
    }


    // Entity methods
    size_t CreateEntity(uint32_t display)
    {
        return mEntityManager->CreateEntity(display);
    }

    void DestroyEntity(uint32_t entity)
    {
        mEntityManager->DestroyEntity(entity);

        mComponentManager->EntityDestroyed(entity);

        mSystemManager->EntityDestroyed(entity, mEntityManager->getEntityDisplay(entity));


    }


    // Component methods
    template<typename T>
    void RegisterComponent()
    {
        mComponentManager->RegisterComponent<T>();
    }

    template<typename T>
    void AddComponent(uint32_t entity, T component)
    {
        mComponentManager->AddComponent<T>(entity, component);

        auto signature = mEntityManager->GetSignature(entity);
        signature.set(mComponentManager->GetComponentType<T>(), true);
        mEntityManager->SetSignature(entity, signature);

        mSystemManager->EntitySignatureChanged(entity, mEntityManager->getEntityDisplay(entity), signature);
    }

    template<typename T>
    void RemoveComponent(uint32_t entity)
    {
        mComponentManager->RemoveComponent<T>(entity);

        auto signature = mEntityManager->GetSignature(entity);
        signature.set(mComponentManager->GetComponentType<T>(), false);
        mEntityManager->SetSignature(entity, signature);

        mSystemManager->EntitySignatureChanged(entity, mEntityManager->getEntityDisplay(entity), signature);
    }

    template<typename T>
    T& GetComponent(uint32_t entity)
    {
        return mComponentManager->GetComponent<T>(entity);
    }

    template<typename T>
    ComponentType GetComponentType()
    {
        return mComponentManager->GetComponentType<T>();
    }


    // System methods
    template<typename T>
    std::shared_ptr<T> RegisterSystem()
    {
        return mSystemManager->RegisterSystem<T>();
    }

    template<typename T>
    void SetSystemSignature(Signature signature)
    {
        mSystemManager->SetSignature<T>(signature);
    }

private:
    std::unique_ptr<ComponentManager> mComponentManager;
    std::unique_ptr<EntityManager> mEntityManager;
    std::unique_ptr<SystemManager> mSystemManager;
    static Coordinator coordinator;
    Coordinator(){};
};


#endif //NINEENGINE_COORDINATOR_H
