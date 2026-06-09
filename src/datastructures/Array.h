// MIT License
// Copyright (c) 2026 Arran Stevens

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct Array
{
    void* data;  
    size_t elementSize;
    size_t size;     
    size_t capacity; 
} Array; 

void Array_Init(Array* array, size_t elementSize, size_t initialCapacity) 
{
    array->data = malloc(initialCapacity * elementSize);
    array->elementSize = elementSize;
    array->size = 0;
    array->capacity = initialCapacity;
}

void Array_Push(Array* array, void* value) 
{
    if (array->size >= array->capacity) {
        array->capacity *= 2;
        array->data = realloc(array->data, array->capacity * array->elementSize);
        if (!array->data) {
            return;
        }
    }
    memcpy((char*)array->data + (array->size * array->elementSize), value, array->elementSize);
    array->size++;
}

void* Array_PushEmpty(Array* array)
{
    if (array->size >= array->capacity) {
        array->capacity *= 2;
        array->data = realloc(array->data, array->capacity * array->elementSize);
        if (!array->data) {
            return NULL;
        }
    }
    char* dst = (char*)array->data + (array->size * array->elementSize);
    array->size++;
    return dst;
}

void Array_DeleteBackfill(Array* array, size_t index)
{
    if (index == array->size - 1) {
        array->size--;
        return;
    }
    void* target = (char*)array->data + (index * array->elementSize);
    void* last = (char*)array->data + ((array->size - 1) * array->elementSize);
    memcpy(target, last, array->elementSize);
    array->size--;
}

void Array_DeleteBackshift(Array* array, size_t index)
{
    if (index < array->size-1)
    {
        void* dest = (char*)array->data + (index * array->elementSize);
        void* src = (char*)array->data + ((index + 1) * array->elementSize);
        size_t bytesToMove = (array->size - index - 1) * array->elementSize;
        memmove(dest, src, bytesToMove);
    }
    array->size--;
}

void Array_DeleteBatchBackshift(Array* array, size_t index, size_t count)
{
    if (count == 0 || index >= array->size) return;
    if (index + count > array->size) count = array->size - index;
    size_t elementsAfter = array->size - (index + count);
    if (elementsAfter > 0) {
        void* dest = (char*)array->data + (index * array->elementSize);
        void* src  = (char*)array->data + ((index + count) * array->elementSize);
        size_t bytesToMove = elementsAfter * array->elementSize;
        memmove(dest, src, bytesToMove);
    }
    array->size -= count;
}

inline void* Array_Get(Array* array, size_t index) 
{
    return (char*)array->data + (index * array->elementSize);
}

inline void Array_Clear(Array* array)
{
    array->size = 0;
}

inline void Array_Free(Array* array) 
{
    free(array->data);
    array->size = array->capacity = 0;
    array->data = NULL;
}