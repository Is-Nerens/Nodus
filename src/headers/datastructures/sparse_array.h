#pragma once
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#include <stdint.h>

typedef struct {
    void* data;
    uint32_t item_size;
    uint32_t capacity;
} Sparse_Array;


Sparse_Array_Init(Sparse_Array* sparse, uint32_t item_size, uint32_t capacity)
{
    capacity = max(capacity, 2);

    // Allocate space for data array
    sparse->data = malloc((1 + item_size) * capacity);

    // Set occupancy bytes to zero
    for (uint32_t i=0; i<capacity; i++) {
        char* location = (char*)sparse->data + i * (1 + item_size);
        uint8_t* val = (uint8_t*)location;
        *val = 0;
    }

    // Initialise tracking variables
    sparse->item_size = item_size;
    sparse->capacity = capacity;
}

void Sparse_Array_Free(struct Sparse_Array* sparse)
{
    free(sparse->data);
}