#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct Stringmap
{
    uint8_t* occupancy;
    void* map;
    uint32_t capacity;
    uint32_t itemSize;
    uint32_t itemCount;
    uint32_t maxProbes;
} Stringmap;

void Stringmap_Free(Stringmap* map)
{
    if (!map) return;
    if (map->occupancy) free(map->occupancy);
    if (map->map) free(map->map);
    map->occupancy = NULL;
    map->map = NULL;
    map->capacity = 0;
    map->itemSize = 0;
    map->itemCount = 0;
    map->maxProbes = 0;
}

int Stringmap_Init(Stringmap* map, uint32_t itemSize, uint32_t capacity, uint32_t chunkCapacity)
{
    // create hashmap
    map->capacity = capacity;
    uint32_t occupancyRemainder = map->capacity & 7;
    uint32_t occupancyBytes = map->capacity >> 3;
    occupancyBytes += 1 * (occupancyRemainder != 0);
    map->occupancy = (uint8_t*)calloc(occupancyBytes, 1); 
    if (map->occupancy == NULL) {
        Stringmap_Free(map); return 0;
    }
    map->map = malloc((sizeof(char*) + itemSize) * capacity);
    map->itemSize = itemSize;
    map->itemCount = 0;
    map->maxProbes = 1;

    // success
    return 1;
}

uint32_t Stringmap_Hash(const char* string) {
    uint32_t hash = 2166136261u;
    for (uint8_t* p = (uint8_t*)string; *p; p++) {
        hash ^= *p;
        hash *= 16777619u;
    }
    return hash;
}

static inline uint8_t Stringmap_IsOccupied(Stringmap* map, uint32_t i)
{
    return map->occupancy[i >> 3] & (1u << (i & 7));
}

static inline void Stringmap_SetOccupied(Stringmap* map, uint32_t i)
{
    map->occupancy[i >> 3] |= (uint8_t)(1 << (i & 7));
}

static inline void Stringmap_SetVacant(Stringmap* map, uint32_t i)
{
    map->occupancy[i >> 3] &= ~(1u << (i & 7));
}

int Stringmap_Resize(Stringmap* map)
{
    uint32_t oldMapCapacity = map->capacity;
    uint8_t* oldOccupancy = map->occupancy;
    void* oldMap = map->map;

    // reisze
    map->capacity *= 2;
    uint32_t occupancyRemainder = map->capacity & 7;
    uint32_t occupancyBytes = map->capacity >> 3;
    occupancyBytes += 1 * (occupancyRemainder != 0);
    map->occupancy = (uint8_t*)calloc(occupancyBytes, 1);
    if (map->occupancy == NULL) {
        map->capacity = oldMapCapacity;
        map->occupancy = oldOccupancy;
        return 0;
    }
    map->map = malloc((sizeof(char*) + map->itemSize) * map->capacity);
    if (map->map == NULL) {
        free(map->occupancy);
        map->capacity = oldMapCapacity;
        map->occupancy = oldOccupancy;
        map->map = oldMap;
        return 0;
    }
    map->maxProbes = 1;
    
    // re-insert items
    for (uint32_t j=0; j<oldMapCapacity; j++) {
        if (oldOccupancy[j >> 3] & (1u << (j & 7)))
        {
            char* oldBase = (char*)oldMap + j * (sizeof(char*) + map->itemSize);
            char* key  = *(char**)oldBase;
            void* value = oldBase + sizeof(char*);

            // add item
            uint32_t probes = 0;
            uint32_t hash = Stringmap_Hash(key);
            while(probes < map->capacity) {
                uint32_t i = (hash + probes) % map->capacity;
                if (!Stringmap_IsOccupied(map, i)) {
                    char* base = (char*)map->map + i * (sizeof(char*) + map->itemSize);
                    memcpy(base, &key, sizeof(char*));
                    memcpy(base + sizeof(char*), value, map->itemSize);
                    Stringmap_SetOccupied(map, i);
                    break;
                }
                probes++;
            }
            if (probes + 1 > map->maxProbes) map->maxProbes = probes + 1;
        }
    }
    return 1;
}

int Stringmap_Set(Stringmap* map, const char* key, void* value)
{
    // resize if surpassed max load factor
    if (map->itemCount * 10 > map->capacity * 7) {
        if (!Stringmap_Resize(map)) {
            return 0;
        }
    }

    uint32_t probes = 0;
    uint32_t hash = Stringmap_Hash(key);
    while(probes < map->capacity) {
        uint32_t i = (hash + probes) % map->capacity;

        // if key exists -> update value
        if (Stringmap_IsOccupied(map, i)) {
            char* base = (char*)map->map + i * (sizeof(char*) + map->itemSize);
            char* storedKey = *(char**)base;
            if (strcmp(key, storedKey) == 0) {
                memcpy(base + sizeof(char*), value, map->itemSize);
                return 1;
            }
        }
        else
        {
            Stringmap_SetOccupied(map, i);
            uint32_t len = strlen(key);
            char* storedKey = (char*)malloc(len + 1);
            memcpy(storedKey, key, len); storedKey[len] = '\0';
            if (storedKey == NULL) return 0;

            char* base = (char*)map->map + i * (sizeof(char*) + map->itemSize);
            memcpy(base, &storedKey, sizeof(char*));
            memcpy(base + sizeof(char*), value, map->itemSize);
            map->itemCount++;
            break;
        }
        probes++;
    }
    if (probes + 1 > map->maxProbes) map->maxProbes = probes + 1;
    return 1;
}

void* Stringmap_Get(Stringmap* map, const char* key)
{
    uint32_t probes = 0;
    uint32_t hash = Stringmap_Hash(key);
    while(probes < map->capacity) {
        uint32_t i = (hash + probes) % map->capacity;
        if (Stringmap_IsOccupied(map, i)) {
            char* base = (char*)map->map + i * (sizeof(char*) + map->itemSize);
            char* storedKey = *(char**)base;
            if (strcmp(key, storedKey) == 0) {
                return (char*)base + sizeof(char*);
            }
        }
        probes++;
    }
    return NULL;
}

int Stringmap_Contains(Stringmap* map, const char* key)
{
    uint32_t probes = 0;
    uint32_t hash = Stringmap_Hash(key);
    while(probes < map->maxProbes) {
        uint32_t i = (hash + probes) % map->capacity;
        if (Stringmap_IsOccupied(map, i)) {
            char* base = (char*)map->map + i * (sizeof(char*) + map->itemSize);
            char* storedKey = *(char**)base;
            if (strcmp(key, storedKey) == 0) {
                return 1;
            }
        }
        probes++;
    }
    return 0;
}

void Stringmap_Delete(Stringmap* map, const char* key)
{
    uint32_t probes = 0;
    uint32_t hash = Stringmap_Hash(key);

    int holeIndex = -1;
    while(probes < map->capacity) {
        uint32_t i = (hash + probes) % map->capacity;
        if (Stringmap_IsOccupied(map, i)) {
            char* base = (char*)map->map + i * (sizeof(char*) + map->itemSize);
            char* storedKey = *(char**)base;
            if (strcmp(key, storedKey) == 0) {
                holeIndex = (int)i;
                Stringmap_SetVacant(map, i);
                free(storedKey);
                break;
            }
        }
        else break;
        probes++;
    }
    
    if (holeIndex == -1) return; // key not found

    uint32_t i = (holeIndex + 1) % map->capacity;
    while (Stringmap_IsOccupied(map, i))
    {
        char* base = (char*)map->map + i * (sizeof(char*) + map->itemSize);
        char* candidateKey = *(char**)base;
        uint32_t candidateHash = Stringmap_Hash(candidateKey);
        uint32_t candidateHome = candidateHash % map->capacity;

        // can the candidate move into the hole?
        int canMoveCandidate;
        if (holeIndex <= i)
            canMoveCandidate = (candidateHome <= holeIndex || candidateHome > i);
        else
            canMoveCandidate = (candidateHome <= holeIndex && candidateHome > i);

        if (!canMoveCandidate) {
            i = (i + 1) % map->capacity;
            continue;
        }

        // move candidate into the hole
        memcpy(
            (char*)map->map + holeIndex * (sizeof(char*) + map->itemSize),
            base,
            sizeof(char*) + map->itemSize
        );

        Stringmap_SetVacant(map, i);
        Stringmap_SetOccupied(map, holeIndex);

        holeIndex = i;
        i = (i + 1) % map->capacity;
    }

    map->itemCount--;
}