#pragma once
#include <stdlib.h>
#include <stdint.h>
#include "nu_node.h"
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef struct NodeTable
{
    NodeP** pointers;
    u32 capacity;
    u32 count;
} NodeTable;

void NodeTableCreate(NodeTable* table, u32 capacity)
{
    if (capacity < 32) capacity = 32;
    table->capacity = capacity;
    table->count = 0;
    table->pointers = calloc(capacity, sizeof(NodeP*));
}

void NodeTableFree(NodeTable* table)
{
    free(table->pointers);
    table->capacity = 0;
    table->count = 0;
}

inline NodeP* NodeTableGet(NodeTable* table, u32 handle)
{
    if (handle >= table->capacity) return NULL;
    return table->pointers[handle];
}

void NodeTableGrow(NodeTable* table)
{
    u32 prevCapacity = table->capacity;
    table->capacity *= 2;
    table->pointers = realloc(table->pointers, table->capacity * sizeof(NodeP*));
    memset(table->pointers + prevCapacity, 0, (table->capacity - prevCapacity) * sizeof(NodeP*));
}


// FNV algorithm https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp
static u32 HashPointer(void* ptr)
{
    uintptr_t val = (uintptr_t)ptr;
    u32 hash = 2166136261u;
    for (size_t i=0; i<sizeof(uintptr_t); i++) {
        hash ^= (uint8_t)(val & 0xFF);
        hash *= 16777619u;
        val >>= 8;
    }
    return hash;
}

void NodeTableAdd(NodeTable* table, NodeP* node)
{
    if (table->count * 10 > table->capacity * 7) {
        NodeTableGrow(table);
    }

    u32 hash = HashPointer(node);
    u32 probes = 0;
    while(1) {
        u32 i = (hash + probes) % table->capacity;
        if (table->pointers[i] == NULL) {
            table->pointers[i] = node;
            node->handle = i;
            break;
        }
        probes++;
    }
    table->count++;
}

inline void NodeTableUpdate(NodeTable* table, u32 handle, NodeP* node)
{
    if (handle >= table->capacity) return;
    table->pointers[handle] = node;
}

inline void NodeTableDelete(NodeTable* table, u32 handle)
{
    if (handle >= table->capacity) return;
    table->pointers[handle] = NULL;
    table->count--;
}