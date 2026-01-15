#pragma once
#include <stdlib.h>
#include <stdint.h>
#include "nu_node.h"
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef struct Layer
{   
    NodeP* nodeArray;
    u32 capacity;
    u32 size;
    u32 count;
} Layer;

void LayerCreate(Layer* layer, u32 capacity)
{
    layer->nodeArray = malloc(sizeof(NodeP) * capacity);
    layer->capacity = capacity;
    layer->size = 0;
    layer->count = 0;
}

void LayerFree(Layer* layer)
{
    free(layer->nodeArray);
    layer->nodeArray = NULL;
    layer->capacity = 0;
    layer->size = 0;
    layer->count = 0;
}

inline NodeP* LayerGet(Layer* layer, u32 index)
{
    return &layer->nodeArray[index];
}