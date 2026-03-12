#pragma once
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// ------------------------------------------------------
// --- Datastructure For Set (generic -> presence) ------
// ------------------------------------------------------
typedef struct Set
{
    uint8_t* occupancy;
    void* data;               // only stores keys
    uint32_t keySize;
    uint32_t itemCount;
    uint32_t capacity;
    uint32_t maxProbes;
} Set;

typedef struct SetIterator
{
    Set* set;
    uint32_t index;
} SetIterator;

void SetInit(Set* set, uint32_t keySize, uint32_t capacity)
{
    if (capacity < 10) capacity = 10; // ensure capacity for at least 10 elements

    // allocate space for occupancy bit array
    uint32_t occupancyRemainder = capacity & 7;
    uint32_t occupancyBytes = capacity >> 3;
    occupancyBytes += 1 * (occupancyRemainder != 0);
    set->occupancy = (uint8_t*)calloc(occupancyBytes, 1); 

    // allocate space for keys only
    set->data = malloc(keySize * capacity);

    // initialise tracking variables
    set->keySize = keySize;
    set->itemCount = 0;
    set->capacity = capacity;
    set->maxProbes = 1;
}

// FNV algorithm https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp
static uint32_t SetHash(void* key, uint32_t len)
{
    uint32_t hash = 2166136261u;
    uint8_t* p = (uint8_t*)key;
    for (uint32_t i = 0; i < len; i++) {
        hash ^= p[i];
        hash *= 16777619u;
    }
    return hash;
}

inline uint8_t SetSlotPresent(Set* set, uint32_t i)
{
    return set->occupancy[i >> 3] & (1u << (i & 7));
}

inline void SetMarkSlot(Set* set, uint32_t i)
{
    set->occupancy[i >> 3] |= (uint8_t)(1 << (i & 7));
}

inline void SetClearSlot(Set* set, uint32_t i)
{
    set->occupancy[i >> 3] &= ~(1u << (i & 7));
}

void SetResizeAdd(Set* set, void* key)
{
    uint32_t probes = 0;
    uint32_t hash = SetHash(key, set->keySize);
    while (probes < set->capacity) {
        uint32_t i = (hash + probes) % set->capacity;

        if (!SetSlotPresent(set, i)) { // Found empty slot

            // set key
            char* base = (char*)set->data + i * set->keySize;
            memcpy(base, key, set->keySize);

            // mark slot and increase item count
            SetMarkSlot(set, i);
            set->itemCount++;
            break;
        }
        probes++;
    }
    if (probes + 1 > set->maxProbes) set->maxProbes = probes + 1;
}

void SetResize(Set* set)
{
    uint32_t oldCapacity = set->capacity;
    uint8_t* oldOccupancy = set->occupancy;
    void* oldData = set->data;

    // Resize
    set->capacity *= 2;
    uint32_t occupancyRemainder = set->capacity & 7;
    uint32_t occupancyBytes = set->capacity >> 3;
    occupancyBytes += 1 * (occupancyRemainder != 0);
    set->occupancy = calloc(occupancyBytes, 1);
    set->data = malloc(set->capacity * set->keySize);
    set->maxProbes = 0;
    set->itemCount = 0;

    // Re-insert all old keys
    for (uint32_t i = 0; i < oldCapacity; i++) {
        uint8_t isPresent = oldOccupancy[i >> 3] & (1u << (i & 7));
        if (isPresent) {
            char* base = (char*)oldData + i * set->keySize;
            void* key = base;
            SetResizeAdd(set, key);
        }
    }

    free(oldOccupancy);
    free(oldData);
}

int SetContains(Set* set, void* key)
{
    uint32_t probes = 0;
    uint32_t hash = SetHash(key, set->keySize);
    while (probes < set->maxProbes) {
        uint32_t i = (hash + probes) % set->capacity;
        if (SetSlotPresent(set, i)) {
            // Check if key matches
            char* base = (char*)set->data + i * set->keySize;
            if (memcmp(base, key, set->keySize) == 0) {
                return 1;
            }
        } else {
            return 0;
        }
        probes++;
    }

    return 0;
}

void SetInsert(Set* set, void* key)
{
    // Resize if surpassed max load factor 
    if ((float)set->itemCount / (float)set->capacity > 0.5f) {
        SetResize(set);
    }

    uint32_t probes = 0;
    uint32_t hash = SetHash(key, set->keySize);
    while (probes < set->capacity) {
        uint32_t i = (hash + probes) % set->capacity;

        if (SetSlotPresent(set, i)) {
            char* base = (char*)set->data + i * set->keySize;
            if (memcmp(base, key, set->keySize) == 0) {
                return; // already exists
            }
        } 
        else 
        {
            SetMarkSlot(set, i);
            
            // set key
            char* base = (char*)set->data + i * set->keySize;
            memcpy(base, key, set->keySize);

            set->itemCount++;
            break;
        }
        probes++;
    }
    if (probes + 1 > set->maxProbes) set->maxProbes = probes + 1;
}

void SetDelete(Set* set, void* key)
{
    uint32_t probes = 0;
    uint32_t hash = SetHash(key, set->keySize);

    int holeIndex = -1;
    while (probes < set->maxProbes) 
    {
        uint32_t i = (hash + probes) % set->capacity;

        if (SetSlotPresent(set, i)) {
            char* checkKey = (char*)set->data + i * set->keySize;
            if (memcmp(checkKey, key, set->keySize) == 0) {
                holeIndex = (int)i;
                SetClearSlot(set, i);
                break;
            }
        }
        else break;

        probes++;
    }

    if (holeIndex == -1) return; // key not found

    uint32_t i = (holeIndex + 1) % set->capacity;
    while (SetSlotPresent(set, i)) 
    {
        char* candidateKey = (char*)set->data + i * set->keySize;
        uint32_t candidateHash = SetHash(candidateKey, set->keySize);
        uint32_t candidateHome = candidateHash % set->capacity;

        // Can the candidate move into the hole?
        bool canMoveCandidate;
        if (holeIndex <= i)
            canMoveCandidate = (candidateHome <= holeIndex || candidateHome > i);
        else
            canMoveCandidate = (candidateHome <= holeIndex && candidateHome > i);

        if (!canMoveCandidate) {
            i = (i + 1) % set->capacity;
            continue;
        }

        // Move candidate into hole
        memcpy(
            (char*)set->data + holeIndex * set->keySize,
            candidateKey,
            set->keySize
        );

        SetClearSlot(set, i);
        SetMarkSlot(set, holeIndex);

        holeIndex = i;
        i = (i + 1) % set->capacity;
    }

    set->itemCount--;
}

SetIterator SetCreateIterator(Set* set)
{
    SetIterator iterator;
    iterator.set = set;
    iterator.index = 0;
    return iterator;
}

int SetIteratorNext(SetIterator* it, void** keyOut)
{
    Set* set = it->set; 

    // no items -> done
    if (set->itemCount == 0) {
        return 0;
    }

    while (it->index < set->capacity) {

        // found item -> set key
        if (SetSlotPresent(set, it->index)) { 
            char* base = (char*)set->data + it->index * set->keySize;
            it->index++;
            *keyOut = base;
            return 1;
        }
        it->index++;
    }

    // done -> reset index
    it->index = 0;
    return 0;
}

void SetClear(Set* set)
{
    if (set->capacity == 0) return;
    uint32_t occupancyRemainder = set->capacity & 7;
    uint32_t occupancyBytes = set->capacity >> 3;
    occupancyBytes += 1 * (occupancyRemainder != 0);
    memset(set->occupancy, 0, occupancyBytes);
    set->itemCount = 0;
    set->maxProbes = 0;
}

void SetFree(Set* set)
{
    free(set->occupancy);
    free(set->data);
    set->occupancy = NULL;
    set->data = NULL;
    set->capacity = 0;
    set->itemCount = 0;
}