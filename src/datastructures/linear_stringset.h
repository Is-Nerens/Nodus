// MIT License
// Copyright (c) 2026 Arran Stevens

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct LinearStringsetChunk
{
    char* buffer;
    uint32_t capacity;
    uint32_t used;
} LinearStringsetChunk;

typedef struct LinearStringset
{
    // chunk array
    LinearStringsetChunk* chunkArray;
    uint32_t chunkArraySize;
    uint32_t chunkArrayCapacity;
    uint32_t allocSearchStart;
    uint32_t totalChunkCapacity;
    
    // hashmap
    uint8_t* occupancy;
    void* set;
    uint32_t mapCapacity;
    uint32_t itemCount;
    uint32_t maxProbes;
} LinearStringset;

void LinearStringset_Free(LinearStringset* set)
{
    if (!set) return;
    if (set->chunkArray) free(set->chunkArray);
    if (set->occupancy) free(set->occupancy);
    if (set->set) free(set->set);
    set->chunkArray = NULL;
    set->occupancy = NULL;
    set->set = NULL;
    set->chunkArraySize = 0;
    set->chunkArrayCapacity = 0;
    set->allocSearchStart = 0;
    set->totalChunkCapacity = 0;
    set->mapCapacity = 0;
    set->itemCount = 0;
    set->maxProbes = 0;
}

int LinearStringset_Init(LinearStringset* set, uint32_t mapCapacity, uint32_t chunkCapacity)
{
    // create chunk buffer array
    if (chunkCapacity < 512) chunkCapacity = 512;
    set->chunkArrayCapacity = 8;
    set->chunkArray = (LinearStringsetChunk*)malloc(sizeof(LinearStringsetChunk) * set->chunkArrayCapacity);
    if (set->chunkArray == NULL) return 0;
    set->chunkArraySize = 1;
    set->allocSearchStart = 0;
    set->totalChunkCapacity = chunkCapacity;

    // create first chunk
    set->chunkArray[0].buffer = (char*)malloc(chunkCapacity);
    if (set->chunkArray[0].buffer == NULL) {
        LinearStringset_Free(set); return 0;
    }
    set->chunkArray[0].capacity = chunkCapacity;
    set->chunkArray[0].used = 0;

    // create hashmap
    set->mapCapacity = mapCapacity;
    uint32_t occupancyRemainder = set->mapCapacity & 7;
    uint32_t occupancyBytes = set->mapCapacity >> 3;
    occupancyBytes += 1 * (occupancyRemainder != 0);
    set->occupancy = (uint8_t*)calloc(occupancyBytes, 1); 
    if (set->occupancy == NULL) {
        LinearStringset_Free(set); return 0;
    }
    set->set = malloc(sizeof(char*) * mapCapacity);
    set->itemCount = 0;
    set->maxProbes = 1;

    // success
    return 1;
}

uint32_t LinearStringset_Hash(const char* string) {
    uint32_t hash = 2166136261u;
    for (uint8_t* p = (uint8_t*)string; *p; p++) {
        hash ^= *p;
        hash *= 16777619u;
    }
    return hash;
}

static inline uint8_t LinearStringset_IsOccupied(LinearStringset* set, uint32_t i)
{
    return set->occupancy[i >> 3] & (1u << (i & 7));
}

static inline void LinearStringset_SetOccupied(LinearStringset* set, uint32_t i)
{
    set->occupancy[i >> 3] |= (uint8_t)(1 << (i & 7));
}

static inline void LinearStringset_SetVacant(LinearStringset* set, uint32_t i)
{
    set->occupancy[i >> 3] &= ~(1u << (i & 7));
}

int LinearStringset_Resize(LinearStringset* set)
{
    uint32_t oldMapCapacity = set->mapCapacity;
    uint8_t* oldOccupancy = set->occupancy;
    void* oldMap = set->set;

    // resize
    set->mapCapacity *= 2;
    uint32_t occupancyRemainder = set->mapCapacity & 7;
    uint32_t occupancyBytes = set->mapCapacity >> 3;
    occupancyBytes += 1 * (occupancyRemainder != 0);
    set->occupancy = (uint8_t*)calloc(occupancyBytes, 1);
    if (set->occupancy == NULL) {
        set->mapCapacity = oldMapCapacity;
        set->occupancy = oldOccupancy;
        return 0;
    }
    set->set = malloc(sizeof(char*) * set->mapCapacity);
    if (set->set == NULL) {
        free(set->occupancy);
        set->mapCapacity = oldMapCapacity;
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
            uint32_t hash = LinearStringset_Hash(string);
            while(probes < set->mapCapacity) {
                uint32_t i = (hash + probes) % set->mapCapacity;
                if (!LinearStringset_IsOccupied(set, i)) {
                    char* base = (char*)set->set + i * sizeof(char*);
                    memcpy(base, &string, sizeof(char*));
                    LinearStringset_SetOccupied(set, i);
                    break;
                }
                probes++;
            }
            if (probes + 1 > set->maxProbes) set->maxProbes = probes + 1;
        }
    }
    return 1;
}

char* LinearStringset_AddString(LinearStringset* set, const char* string)
{
    char* storedKey;
    uint32_t stringLen = strlen(string);
    
    // search chunks for space
    for (uint32_t i=set->allocSearchStart; i<set->chunkArraySize; i++) {
        LinearStringsetChunk* chunk = &set->chunkArray[i];
        if (chunk->capacity - chunk->used >= stringLen + 1) {
            storedKey = (char*)chunk->buffer + chunk->used;
            memcpy(storedKey, string, stringLen);
            storedKey[stringLen] = '\0';
            chunk->used += stringLen + 1;
            return storedKey;
        }
        else if (chunk->capacity - chunk->used < 20)
        {
            set->allocSearchStart = i + 1;
        }
    }

    // no space? -> add new chunk
    if (set->chunkArrayCapacity == set->chunkArraySize) {
        set->chunkArrayCapacity *= 2;
        set->chunkArray = (LinearStringsetChunk*)realloc(set->chunkArray, sizeof(LinearStringsetChunk) * set->chunkArrayCapacity);
        if (set->chunkArray == NULL) return NULL;
    }
    set->chunkArraySize++;
    uint32_t newChunkCap = set->totalChunkCapacity;
    if (newChunkCap < stringLen * 2) newChunkCap = stringLen * 2;
    set->totalChunkCapacity += newChunkCap;
    LinearStringsetChunk* newChunk = &set->chunkArray[set->chunkArraySize - 1];
    newChunk->capacity = newChunkCap;
    newChunk->buffer = (char*)malloc(newChunkCap);
    if (newChunk->buffer == NULL) return NULL;
    newChunk->used = stringLen + 1;

    // copy string into new chunk
    storedKey = (char*)newChunk->buffer;
    memcpy(storedKey, string, stringLen);
    storedKey[stringLen] = '\0';
    return storedKey;
}

char* LinearStringset_Add(LinearStringset* set, const char* string)
{
    // resize if surpassed max load factor
    if (set->itemCount * 10 > set->mapCapacity * 7) {
        if (!LinearStringset_Resize(set)) {
            return 0;
        }
    }

    uint32_t probes = 0;
    uint32_t hash = LinearStringset_Hash(string);
    while(probes < set->mapCapacity) {
        uint32_t i = (hash + probes) % set->mapCapacity;
        if (!LinearStringset_IsOccupied(set, i)) {
            LinearStringset_SetOccupied(set, i);
            char* storedKey = LinearStringset_AddString(set, string);
            if (storedKey == NULL) return 0;
            char* dst = (char*)set->set + i * sizeof(char*);
            memcpy(dst, &storedKey, sizeof(char*));
            return storedKey;
            set->itemCount++;
            break;
        }
        probes++;
    }
    if (probes + 1 > set->maxProbes) set->maxProbes = probes + 1;
    return NULL;
}

char* LinearStringset_Get(LinearStringset* set, const char* string)
{
    uint32_t probes = 0;
    uint32_t hash = LinearStringset_Hash(string);
    while(probes < set->mapCapacity) {
        uint32_t i = (hash + probes) % set->mapCapacity;
        if (LinearStringset_IsOccupied(set, i)) {
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

int LinearStringset_Contains(LinearStringset* set, const char* string)
{
    uint32_t probes = 0;
    uint32_t hash = LinearStringset_Hash(string);
    while(probes < set->maxProbes) {
        uint32_t i = (hash + probes) % set->mapCapacity;
        if (LinearStringset_IsOccupied(set, i)) {
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