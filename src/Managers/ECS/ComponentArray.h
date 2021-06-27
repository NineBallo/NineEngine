//
// Created by nineball on 6/2/21.
//

#ifndef NINEENGINE_COMPONENTARRAY_H
#define NINEENGINE_COMPONENTARRAY_H


#include <cstdio>
#include <cstdint>
#include <bitset>
#include <array>
#include <cassert>
#include <iostream>

#define MAX_ENTITIES 5000
#define MAX_COMPONENTS 8

using Entity = size_t;
using ComponentType = unsigned short;
using Signature = std::bitset<MAX_COMPONENTS>;

// The one instance of virtual inheritance in the entire implementation.
// An interface is needed so that the ComponentManager (seen later)
// can tell a generic ComponentArray that an entity has been destroyed
// and that it needs to update its array mappings.
class IComponentArray
{
public:
    virtual ~IComponentArray() = default;
    virtual void EntityDestroyed(size_t entity) = 0;
};


template<typename T>
class ComponentArray : public IComponentArray
{
public:
    void InsertData(size_t entity, T component)
    {
    //    assert(mEntityToIndexMap.find(entity) == mEntityToIndexMap.end() && "Component added to same entity more than once.");

        // Put new entry at end and update the maps
        size_t newIndex = mSize;
        mEntityToIndexMap[entity] = {newIndex};
        mIndexToEntityMap[newIndex] = {entity};
        mComponentArray[newIndex] = {component};
        ++mSize;
    }

    void RemoveData(size_t entity)
    {
    //    assert(mEntityToIndexMap.find(entity) != mEntityToIndexMap.end() && "Removing non-existent component.");

        if(entity > MAX_COMPONENTS || mEntityToIndexMap[entity] == NULL)
        {
            std::cout << "Removing non-existent component.\n";
            return;
        }

        // Copy element at end into deleted element's place to maintain density
        size_t indexOfRemovedEntity = mEntityToIndexMap[entity];
        size_t indexOfLastElement = mSize - 1;
        mComponentArray[indexOfRemovedEntity] = mComponentArray[indexOfLastElement];

        // Update map to point to moved spot
        int entityOfLastElement = mIndexToEntityMap[indexOfLastElement];

        //Maintaining packed array, move last entity into deleted entity's place
        //then move index of last element to the deleted ones place
        mEntityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
        mIndexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;

        //Remove reference to old entity
        //Remove reference to deleted entity's index
        mEntityToIndexMap[entity] = NULL;
        //Remove outdated duplicate reference of moved entity's index.
        mIndexToEntityMap[indexOfLastElement] = NULL;

        --mSize;
    }

    T& GetData(size_t entity)
    {
        // Return a reference to the entity's component
        return mComponentArray[mEntityToIndexMap[entity]];
    }

    void EntityDestroyed(size_t entity) override
    {
        if (mEntityToIndexMap[entity] != NULL)
        {
            // Remove the entity's component if it existed
            RemoveData(entity);
        }
    }

private:
    // The packed array of components (of generic type T),
    // set to a specified maximum amount, matching the maximum number
    // of entities allowed to exist simultaneously, so that each entity
    // has a unique spot.
    std::array<T, MAX_ENTITIES> mComponentArray;

    std::array<size_t, MAX_ENTITIES> mEntityToIndexMap;

    // Map from an array index to an entity ID.
    std::array<size_t, MAX_ENTITIES> mIndexToEntityMap;

    // Total size of valid entries in the array.
    size_t mSize;
};


#endif //NINEENGINE_COMPONENTARRAY_H
