#pragma once

#include <stdint.h>
#include "nu_memory_map.h"

#define NALLOC_NODE_CAPACITY 256000
#define INVALID_NODE_INDEX UINT32_MAX

typedef union NallocNode {
    Node node;
    u32 nextFree;
} NallocNode;

typedef struct Nalloc {
    NallocNode* allocation;
    u32 freeIndex;
    u32 count;
} Nalloc;

static inline Node* NodePtr(Nalloc* nalloc, u32 index)
{
    return &nalloc->allocation[index].node;
}

static inline u32 NodeIndex(Nalloc* nalloc, Node* ptr)
{
    return (u32)(ptr - &nalloc->allocation[0].node);
}

int Nalloc_Init(Nalloc* nalloc)
{
    if (!nalloc) return 0;

    nalloc->allocation = (NallocNode*)NU_Memory_Map(
        sizeof(NallocNode) * (size_t)NALLOC_NODE_CAPACITY
    );

    if (!nalloc->allocation) return 0;
    nalloc->freeIndex = INVALID_NODE_INDEX;
    nalloc->count = 0;
    return 1;
}


Node* Nalloc_Alloc(Nalloc* nalloc)
{
    // Reuse a previously freed node
    if (nalloc->freeIndex != INVALID_NODE_INDEX) {
        u32 resultIndex = nalloc->freeIndex;
        nalloc->freeIndex = nalloc->allocation[resultIndex].nextFree;
        return &nalloc->allocation[resultIndex].node;
    }

    // Allocate a new node at the end of the pool
    if (nalloc->count >= NALLOC_NODE_CAPACITY) return NULL;
    u32 resultIndex = nalloc->count++;
    return &nalloc->allocation[resultIndex].node;
}

void Nalloc_Free(Nalloc* nalloc, Node* node)
{
    u32 index = (u32)(node - &nalloc->allocation[0].node);
    nalloc->allocation[index].nextFree = nalloc->freeIndex;
    nalloc->freeIndex = index;
}

void Nalloc_Destroy(Nalloc* nalloc)
{
    if (!nalloc) return;

    if (nalloc->allocation) {
        NU_Memory_Unmap(
            nalloc->allocation,
            sizeof(NallocNode) * (size_t)NALLOC_NODE_CAPACITY
        );
        nalloc->allocation = NULL;
    }
    nalloc->freeIndex = INVALID_NODE_INDEX;
    nalloc->count = 0;
}
