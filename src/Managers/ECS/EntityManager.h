//
// Created by nineball on 5/30/21.
//

#ifndef NINEENGINE_ENTITYMANAGER_H
#define NINEENGINE_ENTITYMANAGER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <bitset>
#include "array"
#include "queue"
//#include "Entity.h"


#define MAX_ENTITIES 5000
#define MAX_COMPONENTS 8

using Entity = size_t;
using ComponentType = unsigned short;
using Signature = std::bitset<MAX_COMPONENTS>;

class EntityManager
{
public:
    EntityManager()
    {
        // Initialize the queue with all possible entity IDs
        for (int entity = 0; entity < MAX_ENTITIES; ++entity)
        {
            mAvailableEntities.push(entity);
        }
        std::cout << MAX_ENTITIES << "aa\n";
    }

    int CreateEntity(uint32_t display)
    {
    //    assert(mLivingEntityCount < MAX_ENTITIES && "Too many entities in existence.");

        // Take an ID from the front of the queue
        int id = mAvailableEntities.front();
        mAvailableEntities.pop();
        ++mLivingEntityCount;

        mEntityToDisplay[id] = display;
        return id;
    }

    void DestroyEntity(uint32_t entity)
    {
      //  assert(entity < MAX_ENTITIES && "Entity out of range.");

        // Invalidate the destroyed entity's signature
        mDisplays[mEntityToDisplay[entity]] [entity].reset();

        // Put the destroyed ID at the back of the queue
        mAvailableEntities.push(entity);
        --mLivingEntityCount;
    }

    void SetSignature(uint32_t entity, Signature signature)
    {
     //   assert(entity < MAX_ENTITIES && "Entity out of range.");

        // Put this entity's signature into the array

        mDisplays[mEntityToDisplay[entity]] [entity] = signature;
    }

    Signature GetSignature(uint32_t entity)
    {
     //   assert(entity < MAX_ENTITIES && "Entity out of range.");

        // Get this entity's signature from the array

        return mDisplays[mEntityToDisplay[entity]][entity];
    }

    uint32_t getEntityDisplay(uint32_t entity) {
        return mEntityToDisplay[entity];
    }


private:
    // Queue of unused entity IDs
    std::queue<int> mAvailableEntities{};

    //// Array of signatures where the index corresponds to the entity ID
    //std::array<Signature, MAX_ENTITIES> mSignatures{};

    //Array of displays, each display array contains the entity's in itself.
    std::array<std::array<Signature, MAX_ENTITIES>, 10> mDisplays{};
    //The index is a entity the value is its display
    std::array<uint32_t, MAX_ENTITIES> mEntityToDisplay{};


    // Total living entities - used to keep limits on how many exist
    uint32_t mLivingEntityCount{};
};




#endif //NINEENGINE_ENTITYMANAGER_H
