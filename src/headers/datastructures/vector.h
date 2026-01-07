#pragma once
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct Vector
{
    uint32_t capacity;
    uint32_t size;
    void* data;
    uint32_t element_size;
} Vector;

void Vector_Reserve(Vector* vector, uint32_t element_size, uint32_t capacity)
{
    vector->capacity = capacity;
    vector->size = 0;
    vector->element_size = element_size;
    vector->data = malloc(capacity * element_size);
}

void Vector_Free(Vector* vector)
{
    free(vector->data);
    vector->data = NULL;
    vector->capacity = 0;
    vector->size = 0;
}

void Vector_Grow(Vector* vector)
{
    vector->capacity = MAX(vector->capacity * 2, 2);
    vector->data = realloc(vector->data, vector->capacity * vector->element_size);
}

void Vector_Push(Vector* vector, const void* element)
{
    if (vector->size == vector->capacity) {
        Vector_Grow(vector);
    }
    void* destination = (char*)vector->data + vector->size * vector->element_size;
    memcpy(destination, element, vector->element_size);
    vector->size += 1;
}

void Vector_Delete_Backfill(Vector* vector, uint32_t index)
{
    if (index >= vector->size) {
        return; // out of bounds
    }

    uint32_t last_index = vector->size - 1;

    // Copy last element into the deleted slot
    if (index != last_index) {
        void* dst = (char*)vector->data + index * vector->element_size;
        void* src = (char*)vector->data + last_index * vector->element_size;
        memcpy(dst, src, vector->element_size);
    }

    // Reduce size (no need to free memory here)
    vector->size -= 1;
}

void Vector_Delete_Backshift(Vector* vector, uint32_t index)
{
    if (index >= vector->size) {
        return;
    }
    if (index < vector->size-1)
    {
        void* dest = (char*)vector->data + (index * vector->element_size);
        void* src  = (char*)vector->data + ((index + 1) * vector->element_size);
        uint32_t num_bytes_to_move = (vector->size - index - 1) * vector->element_size;
        memmove(dest, src, num_bytes_to_move);
    }
    vector->size--;
}

inline void* Vector_Get(Vector* vector, uint32_t index)
{
    return (char*) vector->data + index * vector->element_size;
}

inline void Vector_Set(Vector* vector, uint32_t index, void* value) // Unsafe but very fast
{
    void* dst = (char*)vector->data + index * vector->element_size;
    memcpy(dst, value, vector->element_size);
}

void* Vector_Create_Uninitialised(Vector* vector)
{
    if (vector->size == vector->capacity) {
        Vector_Grow(vector);
    }
    void* destination = (char*)vector->data + vector->size * vector->element_size;
    vector->size += 1;
    return destination;
}