// -----------------------------------------------------------------------------------------
// --- | PURPOSE:      to maintain a mapping from a node handle (uint32_t) to a stored Node*
// --- | USED IN:      nu_tree.h
// --- | DEPENDENCIES: nu_node.h
// -----------------------------------------------------------------------------------------


#pragma once
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#include <stdlib.h>
#include <stdint.h>

typedef struct NU_NodeTable
{
    struct Node** data;
    uint8_t* occupancy;
    uint32_t capacity;
    uint32_t used;
} NU_NodeTable;

void NU_Node_Table_Reserve(NU_NodeTable* table, uint32_t capacity)
{
    capacity = MAX(capacity, 32);
    table->capacity = capacity;
    table->used = 0;
    table->data = malloc(sizeof(struct Node*) * capacity);
    
    // Calculate number of bytes needed for occupancy bit array
    uint32_t occupancyBytes = (capacity + 7) / 8;
    table->occupancy = calloc(occupancyBytes, 1); 
}

struct Node* NU_Node_Table_Get(NU_NodeTable* table, uint32_t handle)
{
    if (handle >= table->capacity) return NULL;
    uint32_t rem = handle & 7;                                // i % 8
    uint32_t occupancyIndex = handle >> 3;                   // i / 8
    if (!(table->occupancy[occupancyIndex] & (1u << rem))) { // Found empty
        return NULL;
    }
    return table->data[handle];
}

void NU_Node_Table_Grow(NU_NodeTable* table)
{
    // Old occupancy size in bytes
    uint32_t oldOccupancyBytes = (table->capacity + 7) / 8;

    // Resize data array (in bytes!)
    table->capacity *= 2;
    table->data = realloc(table->data, table->capacity * sizeof(struct Node*));

    // New occupancy size in bytes
    uint32_t newOccupancyBytes = (table->capacity + 7) / 8;

    // Resize occupancy array and copy old data
    uint8_t* oldOccupancy = table->occupancy;
    table->occupancy = calloc(newOccupancyBytes, 1); // zero new bits
    memcpy(table->occupancy, oldOccupancy, oldOccupancyBytes);
    free(oldOccupancy);
}

void NU_Node_Table_Add(NU_NodeTable* table, struct Node* nodePtr)
{
    if (table->used == table->capacity)
        NU_Node_Table_Grow(table);

    uint32_t num_bytes = (table->capacity + 7) / 8;

    for (uint32_t byteIdx = 0; byteIdx < num_bytes; ++byteIdx)
    {
        uint8_t byte = table->occupancy[byteIdx];
        uint8_t free_bits = ~byte;

        // mask out bits beyond capacity in last byte
        if (byteIdx == num_bytes - 1)
        {
            uint32_t valid_bits = table->capacity - (byteIdx << 3);
            free_bits &= (1u << valid_bits) - 1;
        }

        if (free_bits == 0) continue; // skip fully occupied

        uint32_t bit = __builtin_ctz(free_bits);
        uint32_t handle = (byteIdx << 3) + bit;
        nodePtr->handle = handle;

        table->occupancy[byteIdx] |= (1u << bit);
        table->data[handle] = nodePtr;
        table->used++;
        return;
    }
}

void NU_Node_Table_Set(NU_NodeTable* table, uint32_t handle, struct Node* nodePtr)
{
    uint32_t rem = handle & 7;            // handle % 8
    uint32_t occupancyIndex = handle >> 3; // handle / 8
    table->data[handle] = nodePtr;
    table->occupancy[occupancyIndex] |= (uint8_t)(1 << rem); // Mark occupied
}

void NU_Node_Table_Update(NU_NodeTable* table, uint32_t handle, struct Node* nodePtr)
{
    table->data[handle] = nodePtr;
}

void NU_Node_Table_Delete(NU_NodeTable* table, uint32_t handle)
{
    uint32_t rem = handle & 7;            // handle % 8
    uint32_t occupancyIndex = handle >> 3; // handle / 8
    table->occupancy[occupancyIndex] &= ~(1u << rem); // Mark free
    table->data[handle] = NULL;
    table->used--;
}

void NU_Node_Table_Free(NU_NodeTable* table)
{
    free(table->data);
    free(table->occupancy);
}