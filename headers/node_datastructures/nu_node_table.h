// -----------------------------------------------------------------------------------------
// --- | PURPOSE:      to maintain a mapping from a node handle (uint32_t) to a stored Node*
// --- | USED IN:      nu_tree.h
// --- | DEPENDENCIES: nu_node.h
// -----------------------------------------------------------------------------------------


#pragma once
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#include <stdlib.h>
#include <stdint.h>

typedef struct NU_Node_Table
{
    struct Node** data;
    uint8_t* occupancy;
    uint32_t capacity;
    uint32_t used;
} NU_Node_Table;

void NU_Node_Table_Reserve(NU_Node_Table* table, uint32_t capacity)
{
    capacity = MAX(capacity, 32);
    table->capacity = capacity;
    table->used = 0;
    table->data = malloc(sizeof(struct Node*) * capacity);
    
    // Calculate number of bytes needed for occupancy bit array
    uint32_t occupancy_bytes = (capacity + 7) / 8;
    table->occupancy = calloc(occupancy_bytes, 1); 
}

struct Node* NU_Node_Table_Get(NU_Node_Table* table, uint32_t handle)
{
    if (handle >= table->capacity) return NULL;
    uint32_t rem = handle & 7;                                // i % 8
    uint32_t occupancy_index = handle >> 3;                   // i / 8
    if (!(table->occupancy[occupancy_index] & (1u << rem))) { // Found empty
        return NULL;
    }
    return table->data[handle];
}

void NU_Node_Table_Grow(NU_Node_Table* table)
{
    // Old occupancy size in bytes
    uint32_t old_occupancy_bytes = (table->capacity + 7) / 8;

    // Resize data array (in bytes!)
    table->capacity *= 2;
    table->data = realloc(table->data, table->capacity * sizeof(struct Node*));

    // New occupancy size in bytes
    uint32_t new_occupancy_bytes = (table->capacity + 7) / 8;

    // Resize occupancy array and copy old data
    uint8_t* old_occupancy = table->occupancy;
    table->occupancy = calloc(new_occupancy_bytes, 1); // zero new bits
    memcpy(table->occupancy, old_occupancy, old_occupancy_bytes);
    free(old_occupancy);
}

void NU_Node_Table_Add(NU_Node_Table* table, struct Node* node_ptr)
{
    if (table->used == table->capacity)
        NU_Node_Table_Grow(table);

    uint32_t num_bytes = (table->capacity + 7) / 8;

    for (uint32_t byte_idx = 0; byte_idx < num_bytes; ++byte_idx)
    {
        uint8_t byte = table->occupancy[byte_idx];
        uint8_t free_bits = ~byte;

        // mask out bits beyond capacity in last byte
        if (byte_idx == num_bytes - 1)
        {
            uint32_t valid_bits = table->capacity - (byte_idx << 3);
            free_bits &= (1u << valid_bits) - 1;
        }

        if (free_bits == 0) continue; // skip fully occupied

        uint32_t bit = __builtin_ctz(free_bits);
        uint32_t handle = (byte_idx << 3) + bit;
        node_ptr->handle = handle;

        table->occupancy[byte_idx] |= (1u << bit);
        table->data[handle] = node_ptr;
        table->used++;
        return;
    }
}

void NU_Node_Table_Set(NU_Node_Table* table, uint32_t handle, struct Node* node_ptr)
{
    uint32_t rem = handle & 7;            // handle % 8
    uint32_t occupancy_index = handle >> 3; // handle / 8
    table->data[handle] = node_ptr;
    table->occupancy[occupancy_index] |= (uint8_t)(1 << rem); // Mark occupied
}

void NU_Node_Table_Update(NU_Node_Table* table, uint32_t handle, struct Node* node_ptr)
{
    table->data[handle] = node_ptr;
}

void NU_Node_Table_Delete(NU_Node_Table* table, uint32_t handle)
{
    uint32_t rem = handle & 7;            // handle % 8
    uint32_t occupancy_index = handle >> 3; // handle / 8
    table->occupancy[occupancy_index] &= ~(1u << rem); // Mark free
    table->data[handle] = NULL;
    table->used--;
}

void NU_Node_Table_Free(NU_Node_Table* table)
{
    free(table->data);
    free(table->occupancy);
}