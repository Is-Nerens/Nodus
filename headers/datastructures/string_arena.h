#pragma once
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct
{
    uint32_t chunk;
    uint32_t index;
    uint32_t size;
} StringArenaFree;

typedef struct
{
    char** buffer_chunks;
    StringArenaFree* freelist;
    uint32_t string_count;
    uint32_t chunks_used;
    uint32_t chunks_available;
    uint32_t first_chunk_size;
    uint32_t total_buffer_capacity;
    uint32_t freelist_capacity;
    uint32_t freelist_size;
} StringArena;

void StringArena_Init(StringArena* arena, uint32_t chunk_size)
{
    // Garuntee miminum capacity
    chunk_size = MAX(chunk_size, 128);

    // Init state variables
    arena->string_count = 0;
    arena->chunks_used  = 1;
    arena->first_chunk_size = chunk_size;
    arena->total_buffer_capacity = chunk_size;

    // Init chunks array with one char* array
    arena->chunks_available = 4;
    arena->buffer_chunks    = malloc(sizeof(char*) * 4);
    arena->buffer_chunks[0] = malloc(chunk_size);
    arena->buffer_chunks[1] = NULL;
    arena->buffer_chunks[2] = NULL;
    arena->buffer_chunks[3] = NULL;

    // Init freelist
    arena->freelist_capacity = 64;
    arena->freelist_size     = 0;
    arena->freelist = malloc(sizeof(StringArenaFree) * arena->freelist_capacity);

    // Add one free element
    StringArenaFree free;
    free.chunk = 0;
    free.index = 0;
    free.size = chunk_size;
    arena->freelist_size = 1;
    arena->freelist[0] = free;
}

void StringArena_Free(StringArena* arena)
{
    for (uint32_t i=0; i<arena->chunks_used; i++) {
        if (arena->buffer_chunks[i] != NULL) free(arena->buffer_chunks[i]);
    }
    free(arena->buffer_chunks);
    free(arena->freelist);
}

char* StringArena_Add(StringArena* arena, char* string)
{
    uint32_t string_len = (uint32_t)strlen(string) + 1;

    // Search the freelist for space
    int space_index = -1;
    uint32_t chunk_index = 0;
    for (uint32_t i=0; i<arena->freelist_size; i++)
    {
        StringArenaFree* free = &arena->freelist[i];
        if (string_len < free->size) { // Consume first part of free element
            space_index = (int)free->index;
            chunk_index = free->chunk;
            free->size -= string_len;
            free->index += string_len;
            break;
        }
        else if (string_len == free->size) { // Consume entire free element -> remove from list
            space_index = (int)free->index;
            chunk_index = free->chunk;
            arena->freelist_size -= 1;
            if (i != arena->freelist_size) { // Backshift free items
                memmove(
                    &arena->freelist[i],                                 // destination
                    &arena->freelist[i + 1],                             // source (next element)
                    (arena->freelist_size - i) * sizeof(StringArenaFree) // bytes to move
                );
            }
            break;
        }
    }

    // No space anywhere? -> allocate a new chunk
    if (space_index == -1)
    {
        uint32_t chunk_size = arena->total_buffer_capacity; // Effectively doubles the total capacity
        arena->total_buffer_capacity *= 2;

        // Add another chunk
        arena->chunks_used++;
        if (arena->chunks_used == arena->chunks_available) {
            arena->chunks_available *= 2;
            arena->buffer_chunks = realloc(arena->buffer_chunks, arena->chunks_available * sizeof(char*));
        }
        arena->buffer_chunks[arena->chunks_used-1] = malloc(chunk_size);

        // Add a free element for new chunk
        if (arena->freelist_size == arena->freelist_capacity) {
            arena->freelist_capacity *= 2;
            arena->freelist = realloc(arena->freelist, arena->freelist_capacity * sizeof(StringArenaFree));
        }
        StringArenaFree free;
        free.chunk = arena->chunks_used-1;
        free.index = 0;
        free.size = chunk_size;
        arena->freelist[arena->freelist_size] = free;
        arena->freelist_size += 1;
        space_index = 0;
        chunk_index = free.chunk;
    }

    // Copy string into underlying char buffer
    char* chunk = arena->buffer_chunks[chunk_index];
    char* result = chunk + space_index;
    memcpy(result, string, string_len);
    arena->string_count++;

    // Return underlying string
    return result;
}

void StringArena_Delete(StringArena* arena, char* string)
{
    uint32_t string_len = (uint32_t)strlen(string);
    uint32_t string_index = UINT32_MAX;
    uint32_t chunk_index = UINT32_MAX;
    for (uint32_t i=0; i<arena->chunks_used; i++) {
        char* chunk = arena->buffer_chunks[i];

        // Calculate the chunk size
        uint64_t chunk_size = (i < 2) 
            ? (uint64_t)arena->first_chunk_size 
            : (uint64_t)arena->first_chunk_size << (i - 1);

        // Check if string is inside chunk
        if (string >= chunk && string + string_len <= chunk + chunk_size) {
            string_index = (uint32_t)(string - chunk);
            chunk_index = i;
            break; 
        }
    }

    if (string_index == UINT32_MAX) return; // String is not in the arena buffers

    // Create new free struct
    StringArenaFree new_free;
    new_free.chunk = chunk_index;
    new_free.index = string_index;
    new_free.size = string_len;

    // Update the freelist
    int left_adjacent_touch_idx = -1;
    int right_adjacent_touch_idx = -1;
    uint32_t insert_idx = 0;
    for (uint32_t i=0; i<arena->freelist_size; i++) {
        StringArenaFree* f = &arena->freelist[i];
        if (f->chunk != new_free.chunk) continue;

        // Touches left neighbor
        if (f->index + f->size == new_free.index) left_adjacent_touch_idx = (int)i;
        
        // Touches right neighbor
        if (new_free.index + new_free.size == f->index && new_free.chunk == f->chunk) right_adjacent_touch_idx = (int)i;

        // For Case 4: first entry greater than new_free
        if (new_free.chunk < f->chunk || (new_free.chunk == f->chunk && new_free.index < f->index)) {
            insert_idx = i;
            break;
        }
        insert_idx = i + 1; // if we reach the end, insert at the end
    }

    // Case 1: Free item touches two adjacent free items -> merge with left & remove right
    if (left_adjacent_touch_idx != -1 && right_adjacent_touch_idx != -1) {
        arena->freelist[left_adjacent_touch_idx].size += new_free.size + arena->freelist[right_adjacent_touch_idx].size;

        // remove right neighbor
        arena->freelist_size--;
        memmove(&arena->freelist[right_adjacent_touch_idx],
                &arena->freelist[right_adjacent_touch_idx + 1],
                (arena->freelist_size - right_adjacent_touch_idx) * sizeof(StringArenaFree));
    }
    // Case 2: Free item touches left adjacent free item -> merge with left
    else if (left_adjacent_touch_idx != -1) {
        arena->freelist[left_adjacent_touch_idx].size += new_free.size;
    }
    // Case 3: Free item touches right adjacent free item -> merge with right
    else if (right_adjacent_touch_idx != -1) {
        arena->freelist[right_adjacent_touch_idx].index = new_free.index;
        arena->freelist[right_adjacent_touch_idx].size += new_free.size;
    }
    // Case 4: Free item touches no adjacent free items
    else {
        // Case 4: no neighbors -> insert sorted
        if (arena->freelist_size >= arena->freelist_capacity) {
            arena->freelist_capacity *= 2;
            arena->freelist = realloc(arena->freelist,
                                      arena->freelist_capacity * sizeof(StringArenaFree));
        }

        memmove(&arena->freelist[insert_idx + 1],
                &arena->freelist[insert_idx],
                (arena->freelist_size - insert_idx) * sizeof(StringArenaFree));

        arena->freelist[insert_idx] = new_free;
        arena->freelist_size++;
    }
}