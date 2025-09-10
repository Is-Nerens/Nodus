#pragma once

#include <stdint.h>
#include <string.h>
#include "vector.h"

struct String_Arena_Free_Element
{
    uint32_t index;
    uint32_t size;
};

struct String_Arena
{
    struct Vector freelist;
    struct Vector strings; // packed vector of char*
    struct Vector strings_map; // sparse hash vector of char*
    char* buffer;
    uint32_t buffer_capacity;
    uint32_t max_probe_dist;
};

void String_Arena_Init(struct String_Arena* arena, uint32_t buffer_capacity, uint32_t string_capacity, uint32_t freelist_count)
{
    Vector_Reserve(&arena->freelist, sizeof(struct String_Arena_Free_Element), freelist_count);
    Vector_Reserve(&arena->strings, sizeof(char*), string_capacity);
    Vector_Reserve(&arena->strings_map, sizeof(char*), string_capacity * 2);
    arena->buffer_capacity = buffer_capacity;
    arena->buffer = malloc(buffer_capacity);
    arena->max_probe_dist = 0;
}

// FNV algorithm https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp
static uint32_t String_Arena_Hash(char* key) {
    uint32_t hash = 2166136261u;
    for (uint8_t* p = (uint8_t*)key; *p; p++) {
        hash ^= *p;
        hash *= 16777619u;
    }
    return hash;
}

static void String_Arena_Indices_Resize(struct String_Arena* arena)
{
    Vector_Grow(&arena->strings_map);

    // Rebuild map
    arena->max_probe_dist = 0;
    for (int i=0; i<arena->strings_map.size; i++) {
        char* empty_slot = NULL;
        Vector_Set(&arena->strings_map, i, &empty_slot);
    }
    for (int i=0; i<arena->strings.size; i++) {
        char* string = *(char**) Vector_Get(&arena->strings, i);
        uint32_t searches = 0;
        int32_t hash = String_Arena_Hash(string);
        while (searches < arena->strings_map.capacity) {
            uint32_t i = (hash + searches) % arena->strings_map.capacity; 
            char* map_string = *(char**) Vector_Get(&arena->strings_map, i);
            if (map_string == NULL) {
                Vector_Set(&arena->strings_map, i, &string);
                break;
            }
            searches++;
        }
        arena->max_probe_dist = max(arena->max_probe_dist, searches);
    }
}

char* String_Arena_Add(struct String_Arena* arena, char* string)
{
    uint32_t len = (uint32_t)strlen(string) + 1;
    char* result;
    int space_found = 0;

    // Search freelist
    for (int i=0; i<arena->freelist.size; i++) 
    {
        struct String_Arena_Free_Element* free = Vector_Get(&arena->freelist, i);
        if (len < free->size) 
        {
            memcpy(arena->buffer + free->index, string, len);
            result = (char*)arena->buffer + free->index;
            free->index += len;
            free->size -= len;
            space_found = 1;
            break;
        }
        else if (len == free->size) 
        {
            memcpy(arena->buffer + free->index, string, len);
            Vector_Delete_Backfill(&arena->freelist, i);
            result = (char*)arena->buffer + free->index;
            space_found = 1;
            break;
        }
    }

    if (!space_found) 
    {
        // No space found -> grow buffer then add string to the arena
        arena->buffer_capacity = max(arena->buffer_capacity * 2, arena->buffer_capacity + len + 10);
        arena->buffer = realloc(arena->buffer, (size_t)arena->buffer_capacity);
        uint32_t buffer_index = (arena->freelist.size > 0) 
            ? ((struct String_Arena_Free_Element*)Vector_Get(&arena->freelist, arena->freelist.size - 1))->index +
              ((struct String_Arena_Free_Element*)Vector_Get(&arena->freelist, arena->freelist.size - 1))->size : 0;
        memcpy(arena->buffer + buffer_index, string, len);
        struct String_Arena_Free_Element new_free = { buffer_index + len, arena->buffer_capacity - buffer_index - len };
        Vector_Push(&arena->freelist, &new_free);
        result = (char*)arena->buffer + buffer_index;
    }

    // Add result to string vector
    Vector_Push(&arena->strings, &result);


    // Update string map
    uint32_t searches = 0;
    uint32_t hash = String_Arena_Hash(string);
    while (searches < arena->strings_map.capacity) {
        uint32_t i = (hash + searches) % arena->strings_map.capacity;
        char* map_string = *(char**) Vector_Get(&arena->strings_map, i);
        if (map_string == NULL) // Found free slot
        {
            Vector_Set(&arena->strings_map, i, &result);
            break;
        } 
        searches++;
    }
    arena->max_probe_dist = max(arena->max_probe_dist, searches);
    return result;
}   

char* String_Arena_Get(struct String_Arena* arena, char* key)
{
    uint32_t searches = 0;
    uint32_t hash = String_Arena_Hash(key);
    uint32_t max_searches = min(arena->strings_map.capacity, arena->max_probe_dist + 1);
    while (searches < max_searches) {
        uint32_t i = (hash + searches) % arena->strings_map.capacity; 
        char* map_string = *(char**) Vector_Get(&arena->strings_map, i);

        // Check for match
        if (map_string != NULL && strcmp(map_string, key) == 0) {
            arena->max_probe_dist = max(arena->max_probe_dist, searches);
            return map_string; // Found
        }
        searches++;
    }
    
    return NULL; // Not found
}
