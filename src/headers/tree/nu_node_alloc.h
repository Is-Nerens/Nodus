#pragma once
#include <stdlib.h>
#include <stdint.h>

typedef union NallocChunk NallocChunk;
union NallocChunk {
    NallocChunk* next;
    char buffer[256];
};

typedef struct ArrayStart ArrayStart;
typedef struct ArrayStart
{ 
    NallocChunk* array;
    ArrayStart* next;
} ArrayStart;

typedef struct Nalloc
{
    NallocChunk* freeChunk;
    ArrayStart* arrayStart;
    uint32_t chunksPerArray;
} Nalloc;

int Nalloc_Init(Nalloc* pool, uint32_t itemsPerBlock)
{
    if (!pool) return 0;
    
    NallocChunk* array = (NallocChunk*)malloc(itemsPerBlock * sizeof(NallocChunk));
    if (!array) return 0;

    ArrayStart* node = (ArrayStart*)malloc(sizeof(ArrayStart));
    if (!node) {
        free(array);
        return 0;
    }

    for (uint32_t i = 0; i < itemsPerBlock - 1; i++) {
        array[i].next = &array[i + 1];
    }
    array[itemsPerBlock - 1].next = NULL;

    pool->freeChunk       = array;
    pool->arrayStart      = node;
    pool->chunksPerArray  = itemsPerBlock;

    node->array = array;
    node->next  = NULL;
    return 1;
}

int Nalloc_Expand(Nalloc* pool, uint32_t extraChunks)
{
    if (pool == NULL) return 0;

    NallocChunk* extraArray = (NallocChunk*)malloc(extraChunks * sizeof(NallocChunk));
    if (extraArray == NULL) return 0;

    ArrayStart* arrayStart = (ArrayStart*)malloc(sizeof(ArrayStart));
    if (arrayStart == NULL) {
        free(extraArray);
        return 0;
    }

    for (uint32_t i=0; i<extraChunks-1; i++) {
        extraArray[i].next = &extraArray[i+1];
    }

    extraArray[extraChunks-1].next = pool->freeChunk;
    pool->freeChunk = extraArray;

    arrayStart->array = extraArray;
    arrayStart->next = pool->arrayStart;
    pool->arrayStart = arrayStart;
    return 1;
}

void* Nalloc_Alloc(Nalloc* pool) {
    if (pool == NULL) return NULL;

    if (pool->freeChunk == NULL) {
        if (!Nalloc_Expand(pool, pool->chunksPerArray)) return NULL;
    }

    NallocChunk* result = pool->freeChunk;
    pool->freeChunk = pool->freeChunk->next;
    return result;
}

void Nalloc_Free(Nalloc* pool, void* ptr) {
    if (pool == NULL || ptr == NULL) return;
    NallocChunk* freed = (NallocChunk*)ptr;
    freed->next = pool->freeChunk;
    pool->freeChunk = freed;
}

void Nalloc_Destroy(Nalloc* pool) {
    if (pool == NULL) return;

    ArrayStart* arrayStart = pool->arrayStart;
    while (arrayStart != NULL) {
        ArrayStart* next = arrayStart->next;
        free(arrayStart->array);
        free(arrayStart);
        arrayStart = next;
    }
    
    pool->arrayStart = NULL;
    pool->freeChunk = NULL;
}