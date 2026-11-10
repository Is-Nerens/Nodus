#pragma once

#include "stdlib.h"

typedef struct TokenArray {
    int* data;
    size_t size;
    size_t capacity;
} TokenArray;

TokenArray TokenArray_Create(size_t reserveCapacity)
{
    if (reserveCapacity == 0) reserveCapacity = 10;
    TokenArray array;
    array.size = 0;
    array.capacity = reserveCapacity;
    array.data = malloc(sizeof(int) * reserveCapacity);
    return array;
}

inline void TokenArray_Add(TokenArray* array, int token)
{
    if (array->size >= array->capacity) {
        array->capacity *= 2;
        array->data = realloc(array->data, sizeof(int) * array->capacity);
    }
    array->data[array->size] = token;
    array->size++;
}

inline int TokenArray_Get(TokenArray* array, size_t index)
{
    return array->data[index];
}

void TokenArray_Free(TokenArray* array)
{
    free(array->data);
    array->size = 0;
    array->capacity = 0;
    array->data = NULL;
}