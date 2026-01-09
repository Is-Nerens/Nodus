#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <datastructures/string.h>
#include <datastructures/hashmap.h>

typedef struct GrowStringmapChunk
{
    char* buffer;
    uint32_t capacity;
    uint32_t used;
} GrowStringmapChunk;

typedef struct GrowStringmap
{
    // chunk array
    GrowStringmapChunk* chunkArray;
    uint32_t chunkArraySize;
    uint32_t chunkArrayCapacity;
    uint32_t allocSearchStart;
    
    // hashmap
    uint8_t* occupancy;
    void* map;
    uint32_t mapCapacity;
    uint32_t itemSize;
    uint32_t itemCount;
    uint32_t maxProbes;
} GrowStringmap;

void GrowStringmapFree(GrowStringmap* map)
{
    if (!map) return;
    if (map->chunkArray) free(map->chunkArray);
    if (map->occupancy) free(map->occupancy);
    if (map->map) free(map->map);
    map->chunkArray = NULL;
    map->occupancy = NULL;
    map->map = NULL;
    map->chunkArraySize = 0;
    map->chunkArrayCapacity = 0;
    map->allocSearchStart = 0;
    map->mapCapacity = 0;
    map->itemSize = 0;
    map->itemCount = 0;
    map->maxProbes = 0;
}

int GrowStringmapInit(GrowStringmap* map, uint32_t itemSize, uint32_t mapCapacity, uint32_t chunkCapacity)
{
    // create chunk buffer array
    if (chunkCapacity < 512) chunkCapacity = 512;
    map->chunkArrayCapacity = 8;
    map->chunkArray = malloc(sizeof(GrowStringmapChunk) * map->chunkArrayCapacity);
    if (map->chunkArray == NULL) return 0;
    map->chunkArraySize = 1;
    map->allocSearchStart = 0;

    // create first chunk
    map->chunkArray[0].buffer = malloc(chunkCapacity);
    if (map->chunkArray[0].buffer == NULL) {
        GrowStringmapFree(map); return 0;
    }
    map->chunkArray[0].capacity = chunkCapacity;
    map->chunkArray[0].used = 0;

    // create hashmap
    map->mapCapacity = mapCapacity;
    uint32_t occupancyRemainder = map->mapCapacity & 7;
    uint32_t occupancyBytes = map->mapCapacity >> 3;
    occupancyBytes += 1 * (occupancyRemainder != 0);
    map->occupancy = (uint8_t*)calloc(occupancyBytes, 1); 
    if (map->occupancy == NULL) {
        GrowStringmapFree(map); return 0;
    }
    map->map = malloc((sizeof(char*) + itemSize) * mapCapacity);
    map->itemSize = itemSize;
    map->itemCount = 0;
    map->maxProbes = 1;

    // success
    return 1;
}

uint32_t GrowStringmapHash(char* string) {
    uint32_t hash = 2166136261u;
    for (uint8_t* p = (uint8_t*)string; *p; p++) {
        hash ^= *p;
        hash *= 16777619u;
    }
    return hash;
}

static inline uint8_t GrowStringmapSlotPresent(GrowStringmap* map, uint32_t i)
{
    return map->occupancy[i >> 3] & (1u << (i & 7));
}

static inline void GrowStringmapMarkSlot(GrowStringmap* map, uint32_t i)
{
    map->occupancy[i >> 3] |= (uint8_t)(1 << (i & 7));
}

static inline void GrowStringmapClearSlot(GrowStringmap* map, uint32_t i)
{
    map->occupancy[i >> 3] &= ~(1u << (i & 7));
}

int GrowStringmapGrowRehash(GrowStringmap* map)
{
    uint32_t oldMapCapacity = map->mapCapacity;
    uint8_t* oldOccupancy = map->occupancy;
    void* oldMap = map->map;

    // reisze
    map->mapCapacity *= 2;
    uint32_t occupancyRemainder = map->mapCapacity & 7;
    uint32_t occupancyBytes = map->mapCapacity >> 3;
    occupancyBytes += 1 * (occupancyRemainder != 0);
    map->occupancy = calloc(occupancyBytes, 1);
    if (map->occupancy == NULL) {
        map->mapCapacity = oldMapCapacity;
        map->occupancy = oldOccupancy;
        return 0;
    }
    map->map = malloc((sizeof(char*) + map->itemSize) * map->mapCapacity);
    if (map->map == NULL) {
        free(map->occupancy);
        map->mapCapacity = oldMapCapacity;
        map->occupancy = oldOccupancy;
        map->map = oldMap;
        return 0;
    }
    map->maxProbes = 1;
    
    // re-insert items
    for (uint32_t j=0; j<oldMapCapacity; j++) {
        if (oldOccupancy[j >> 3] & (1u << (j & 7)))
        {
            char* base = (char*)oldMap + j * (sizeof(char*) + map->itemSize);
            char* key  = *(char**)base;
            void* value = base + sizeof(char*);

            // add item
            uint32_t probes = 0;
            uint32_t hash = GrowStringmapHash(key);
            while(probes < map->mapCapacity) {
                uint32_t i = (hash + probes) % map->mapCapacity;
                if (!GrowStringmapSlotPresent(map, i)) {
                    char* base = (char*)map->map + i * (sizeof(char*) + map->itemSize);
                    memcpy(base, key, sizeof(char*));
                    memcpy(base + sizeof(char*), value, map->itemSize);
                    GrowStringmapMarkSlot(map, i);
                    break;
                }
                probes++;
            }
            if (probes + 1 > map->maxProbes) map->maxProbes = probes + 1;
        }
    }
    return 1;
}

char* GrowStringmapAddKey(GrowStringmap* map, char* key)
{
    char* storedKey;
    uint32_t keyLen = strlen(key);
    
    // search chunks for space
    for (uint32_t i=map->allocSearchStart; i<map->chunkArraySize; i++) {
        GrowStringmapChunk* chunk = &map->chunkArray[i];
        if (chunk->capacity - chunk->used >= keyLen + 1) {
            storedKey = (char*)chunk->buffer + chunk->used;
            memcpy(storedKey, key, keyLen);
            storedKey[keyLen] = '\0';
            chunk->used += keyLen + 1;
            return storedKey;
        }
        else if (chunk->capacity - chunk->used < 20)
        {
            map->allocSearchStart++;
        }
    }

    // no space? -> add new chunk
    if (map->chunkArrayCapacity == map->chunkArraySize) {
        map->chunkArrayCapacity *= 2;
        map->chunkArray = realloc(map->chunkArray, sizeof(GrowStringmapChunk) * map->chunkArrayCapacity);
        if (map->chunkArray == NULL) return NULL;
    }
    map->chunkArraySize++;
    uint32_t newChunkCap = map->chunkArray[0].capacity;
    if (newChunkCap < keyLen * 2) newChunkCap = keyLen * 2;
    GrowStringmapChunk* newChunk = &map->chunkArray[map->chunkArraySize - 1];
    newChunk->capacity = newChunkCap;
    newChunk->buffer = malloc(newChunkCap);
    if (newChunk->buffer == NULL) return NULL;
    newChunk->used = keyLen + 1;

    // copy key into new chunk
    storedKey = (char*)newChunk->buffer;
    memcpy(storedKey, key, keyLen);
    storedKey[keyLen] = '\0';
    return storedKey;
}

int GrowStringmapSet(GrowStringmap* map, char* key, void* value)
{
    // resize if surpassed max load factor
    if (map->itemCount * 10 > map->mapCapacity * 7) {
        if (!GrowStringmapGrowRehash(map)) {
            return 0;
        }
    }

    uint32_t probes = 0;
    uint32_t hash = GrowStringmapHash(key);
    while(probes < map->mapCapacity) {
        uint32_t i = (hash + probes) % map->mapCapacity;

        // if key exists -> update value
        if (GrowStringmapSlotPresent(map, i)) {
            char* base = (char*)map->map + i * (sizeof(char*) + map->itemSize);
            char* storedKey = *(char**)base;
            if (strcmp(key, storedKey) == 0) {
                memcpy(base + sizeof(char*), value, map->itemSize);
                return 1;
            }
        }
        else
        {
            GrowStringmapMarkSlot(map, i);
            char* storedKey = GrowStringmapAddKey(map, key);
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

void* GrowStringmapGet(GrowStringmap* map, char* key)
{
    uint32_t probes = 0;
    uint32_t hash = GrowStringmapHash(key);
    while(probes < map->mapCapacity) {
        uint32_t i = (hash + probes) % map->mapCapacity;
        if (GrowStringmapSlotPresent(map, i)) {
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

int GrowStringmapContains(GrowStringmap* map, char* key)
{
    uint32_t probes = 0;
    uint32_t hash = GrowStringmapHash(key);
    while(probes < map->mapCapacity) {
        uint32_t i = (hash + probes) % map->mapCapacity;
        if (GrowStringmapSlotPresent(map, i)) {
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