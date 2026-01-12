#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct Stringset
{
    uint8_t* occupancy;
    void* set;
    uint32_t capacity;
    uint32_t itemCount;
    uint32_t maxProbes;
} Stringset;

void StringsetFree(Stringset* set)
{
    if (!set) return;
    if (set->occupancy) free(set->occupancy);
    if (set->set) free(set->set);
    set->occupancy = NULL;
    set->set = NULL;
    set->capacity = 0;
    set->itemCount = 0;
    set->maxProbes = 0;
}

int StringsetInit(Stringset* set, uint32_t capacity, uint32_t chunkCapacity)
{
    // create hashmap
    set->capacity = capacity;
    uint32_t occupancyRemainder = set->capacity & 7;
    uint32_t occupancyBytes = set->capacity >> 3;
    occupancyBytes += 1 * (occupancyRemainder != 0);
    set->occupancy = (uint8_t*)calloc(occupancyBytes, 1); 
    if (set->occupancy == NULL) {
        StringsetFree(set); return 0;
    }
    set->set = malloc(sizeof(char*) * capacity);
    set->itemCount = 0;
    set->maxProbes = 1;

    // success
    return 1;
}

uint32_t StringsetHash(char* string) {
    uint32_t hash = 2166136261u;
    for (uint8_t* p = (uint8_t*)string; *p; p++) {
        hash ^= *p;
        hash *= 16777619u;
    }
    return hash;
}

static inline uint8_t StringsetSlotPresent(Stringset* set, uint32_t i)
{
    return set->occupancy[i >> 3] & (1u << (i & 7));
}

static inline void StringsetMarkSlot(Stringset* set, uint32_t i)
{
    set->occupancy[i >> 3] |= (uint8_t)(1 << (i & 7));
}

static inline void StringsetClearSlot(Stringset* set, uint32_t i)
{
    set->occupancy[i >> 3] &= ~(1u << (i & 7));
}

int StringsetRehash(Stringset* set)
{
    uint32_t oldMapCapacity = set->capacity;
    uint8_t* oldOccupancy = set->occupancy;
    void* oldMap = set->set;

    // reisze
    set->capacity *= 2;
    uint32_t occupancyRemainder = set->capacity & 7;
    uint32_t occupancyBytes = set->capacity >> 3;
    occupancyBytes += 1 * (occupancyRemainder != 0);
    set->occupancy = (uint8_t*)calloc(occupancyBytes, 1);
    if (set->occupancy == NULL) {
        set->capacity = oldMapCapacity;
        set->occupancy = oldOccupancy;
        return 0;
    }
    set->set = malloc(sizeof(char*) * set->capacity);
    if (set->set == NULL) {
        free(set->occupancy);
        set->capacity = oldMapCapacity;
        set->occupancy = oldOccupancy;
        set->set = oldMap;
        return 0;
    }
    set->maxProbes = 1;
    
    // re-insert items
    for (uint32_t j=0; j<oldMapCapacity; j++) {
        if (oldOccupancy[j >> 3] & (1u << (j & 7)))
        {
            char* oldBase = (char*)oldMap + j * sizeof(char*);
            char* string  = *(char**)oldBase;

            // add item
            uint32_t probes = 0;
            uint32_t hash = StringsetHash(string);
            while(probes < set->capacity) {
                uint32_t i = (hash + probes) % set->capacity;
                if (!StringsetSlotPresent(set, i)) {
                    char* base = (char*)set->set + i * sizeof(char*);
                    memcpy(base, &string, sizeof(char*));
                    StringsetMarkSlot(set, i);
                    break;
                }
                probes++;
            }
            if (probes + 1 > set->maxProbes) set->maxProbes = probes + 1;
        }
    }
    return 1;
}

char* StringsetAdd(Stringset* set, char* string)
{
    // resize if surpassed max load factor
    if (set->itemCount * 10 > set->capacity * 7) {
        if (!StringsetRehash(set)) {
            return 0;
        }
    }

    uint32_t probes = 0;
    uint32_t hash = StringsetHash(string);
    while(probes < set->capacity) {
        uint32_t i = (hash + probes) % set->capacity;
        if (!StringsetSlotPresent(set, i)) {
            StringsetMarkSlot(set, i);
            uint32_t len = strlen(string);
            char* storedKey = (char*)malloc(len + 1);
            memcpy(storedKey, string, len); storedKey[len] = '\0';
            if (storedKey == NULL) return 0;
            char* dst = (char*)set->set + i * sizeof(char*);
            memcpy(dst, &storedKey, sizeof(char*));
            set->itemCount++;
            return storedKey;
        }
        probes++;
    }
    if (probes + 1 > set->maxProbes) set->maxProbes = probes + 1;
    return NULL;
}

char* StringsetGet(Stringset* set, char* string)
{
    uint32_t probes = 0;
    uint32_t hash = StringsetHash(string);
    while(probes < set->capacity) {
        uint32_t i = (hash + probes) % set->capacity;
        if (StringsetSlotPresent(set, i)) {
            char* base = (char*)set->set + i * sizeof(char*);
            char* storedKey = *(char**)base;
            if (strcmp(string, storedKey) == 0) {
                return storedKey;
            }
        }
        probes++;
    }
    return NULL;
}

int StringsetContains(Stringset* set, char* string)
{
    uint32_t probes = 0;
    uint32_t hash = StringsetHash(string);
    while(probes < set->maxProbes) {
        uint32_t i = (hash + probes) % set->capacity;
        if (StringsetSlotPresent(set, i)) {
            char* base = (char*)set->set + i * sizeof(char*);
            char* storedKey = *(char**)base;
            if (strcmp(string, storedKey) == 0) {
                return 1;
            }
        }
        probes++;
    }
    return 0;
}

void StringsetDelete(Stringset* set, char* string)
{
    uint32_t probes = 0;
    uint32_t hash = StringsetHash(string);

    int holeIndex = -1;
    while(probes < set->capacity) {
        uint32_t i = (hash + probes) % set->capacity;
        if (StringsetSlotPresent(set, i)) {
            char* base = (char*)set->set + i * sizeof(char*);
            char* storedKey = *(char**)base;
            if (strcmp(string, storedKey) == 0) {
                holeIndex = (int)i;
                StringsetClearSlot(set, i);
                free(storedKey);
                break;
            }
        }
        else break;
        probes++;
    }
    
    if (holeIndex == -1) return; // string not found

    uint32_t i = (holeIndex + 1) % set->capacity;
    while (StringsetSlotPresent(set, i))
    {
        char* base = (char*)set->set + i * sizeof(char*);
        char* candidateKey = *(char**)base;
        uint32_t candidateHash = StringsetHash(candidateKey);
        uint32_t candidateHome = candidateHash % set->capacity;

        // can the candidate move into the hole?
        int canMoveCandidate;
        if (holeIndex <= i)
            canMoveCandidate = (candidateHome <= holeIndex || candidateHome > i);
        else
            canMoveCandidate = (candidateHome <= holeIndex && candidateHome > i);

        if (!canMoveCandidate) {
            i = (i + 1) % set->capacity;
            continue;
        }

        // move candidate into the hole
        memcpy(
            (char*)set->set + holeIndex * sizeof(char*),
            base,
            sizeof(char*)
        );

        StringsetClearSlot(set, i);
        StringsetMarkSlot(set, holeIndex);

        holeIndex = i;
        i = (i + 1) % set->capacity;
    }

    set->itemCount--;
}