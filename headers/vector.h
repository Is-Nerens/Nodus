#pragma once

#define MAX(a, b) ((a) > (b) ? (a) : (b))

struct Vector
{
    uint32_t capacity;
    uint32_t size;
    void* data;
    size_t element_size;
};

void Vector_Reserve(struct Vector* vector, size_t element_size, uint32_t capacity)
{
    vector->capacity = capacity;
    vector->size = 0;
    vector->element_size = element_size;
    vector->data = malloc(capacity * element_size);
}

void Vector_Free(struct Vector* vector)
{
    free(vector->data);
    vector->data = NULL;
    vector->capacity = 0;
    vector->size = 0;
}

void Vector_Grow(struct Vector* vector)
{
    vector->capacity = MAX(vector->capacity * 2, 2);
    vector->data = realloc(vector->data, vector->capacity * vector->element_size);
}

void Vector_Push(struct Vector* vector, const void* element)
{
    if (vector->size == vector->capacity) {
        Vector_Grow(vector);
    }
    void* destination = (char*)vector->data + vector->size * vector->element_size;
    memcpy(destination, element, vector->element_size);
    vector->size += 1;
}

void Vector_Delete_Backfill(struct Vector* vector, uint32_t index)
{
    if (index >= vector->size) {
        return; // out of bounds
    }

    uint32_t last_index = vector->size - 1;

    if (index != last_index) {
        // Copy last element into the deleted slot
        void* dst = (char*)vector->data + index * vector->element_size;
        void* src = (char*)vector->data + last_index * vector->element_size;
        memcpy(dst, src, vector->element_size);
    }

    // Reduce size (no need to free memory here)
    vector->size -= 1;
}

void* Vector_Get(struct Vector* vector, uint32_t index)
{
    return (char*) vector->data + index * vector->element_size;
}

void Vector_Set(struct Vector* vector, uint32_t index, void* value) // Unsafe but very fast
{
    void* dst = (char*)vector->data + index * vector->element_size;
    memcpy(dst, value, vector->element_size);
}