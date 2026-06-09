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
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct LinallocChunk {
    char* data;
    uint32_t size;
    uint32_t capacity;
} LinallocChunk;

typedef struct Linalloc {
    LinallocChunk* chunks; // pointer to array of chunks
    uint32_t chunksReserved;
    uint32_t chunkCount;
} Linalloc;

void Linalloc_Init(Linalloc* linalloc, uint32_t chunkSize)
{
    linalloc->chunksReserved = 4;
    linalloc->chunkCount = 1;
    linalloc->chunks = malloc(sizeof(LinallocChunk) * linalloc->chunksReserved);
    if (linalloc->chunks == NULL) abort();

    // Create first chunk
    linalloc->chunks[0].capacity = chunkSize;
    linalloc->chunks[0].size = 0;
    linalloc->chunks[0].data = malloc(chunkSize);
    if (linalloc->chunks[0].data == NULL) abort();
}

void Linalloc_Destroy(Linalloc* linalloc)
{
    // Free chunk data
    for (uint32_t i=0; i<linalloc->chunkCount; i++) {
        free(linalloc->chunks[i].data);
    }

    // Free chunk array
    free(linalloc->chunks);

    // Set defaults
    linalloc->chunks = NULL;
    linalloc->chunksReserved = 0;
    linalloc->chunkCount = 0;
}

LinallocChunk* Linalloc_Expand(Linalloc* linalloc, uint32_t minChunkCapacity)
{
    // Grow chunk array if necessary
    if (linalloc->chunkCount == linalloc->chunksReserved) {
        linalloc->chunksReserved *= 2;
        linalloc->chunks = realloc(linalloc->chunks, sizeof(LinallocChunk) * linalloc->chunksReserved);
        if (linalloc->chunks == NULL) abort();
    }

    // Get last chunk item in array
    LinallocChunk* lastChunk = &linalloc->chunks[linalloc->chunkCount - 1];

    // Create new chunk
    LinallocChunk* newChunk = &linalloc->chunks[linalloc->chunkCount];
    newChunk->capacity = lastChunk->capacity * 2;
    if (newChunk->capacity < minChunkCapacity) newChunk->capacity = minChunkCapacity;
    newChunk->data = malloc(newChunk->capacity); if (newChunk->data == NULL) abort();
    newChunk->size = 0;

    linalloc->chunkCount++;
    return newChunk;
}

static inline uint32_t AlignUp(uint32_t x, uint32_t align)
{
    return (x + (align - 1)) & ~(align - 1);
}


void* Linalloc_Alloc(Linalloc* linalloc, uint32_t size)
{
    LinallocChunk* allocChunk = NULL;
    const uint32_t ALIGN = 8;
    size = AlignUp(size, ALIGN);

    // Search chunks to find space at the end
    for (uint32_t i=0; i<linalloc->chunkCount; i++) {

        uint32_t alignedSize = AlignUp(linalloc->chunks[i].size, ALIGN);
        uint32_t remainingSpace = linalloc->chunks[i].capacity - alignedSize;

        if (size <= remainingSpace) {
            allocChunk = &linalloc->chunks[i];
            allocChunk->size = alignedSize;
            break;
        }
    }

    // Create a new chunk if there was no space left
    if (allocChunk == NULL) {
        allocChunk = Linalloc_Expand(linalloc, size);
    }

    // Consume space in allocChunk and return pointer
    char* allocPtr = allocChunk->data + allocChunk->size;
    allocChunk->size += size;
    return allocPtr;
}

int Linalloc_Empty(Linalloc* linalloc)
{
    for (uint32_t i=0; i<linalloc->chunkCount; i++) {
        if (linalloc->chunks[i].size != 0) return 0;
    }
    return 1;
}

void Linalloc_Clear(Linalloc* linalloc)
{
    if (linalloc->chunkCount == 0)
        return;

    // Free all chunks except the first
    for (uint32_t i=1; i<linalloc->chunkCount; i++) {
        free(linalloc->chunks[i].data);
        linalloc->chunks[i].data = NULL;
        linalloc->chunks[i].size = 0;
        linalloc->chunks[i].capacity = 0;
    }

    // Reset first chunk
    linalloc->chunks[0].size = 0;
    linalloc->chunkCount = 1;
}