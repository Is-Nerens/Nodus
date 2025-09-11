#pragma once

#include <stdint.h>
#include <string.h>
#include "vector.h"

struct String_Set_Free_Element
{
    uint32_t index;
    uint32_t size;
};

struct String_Set
{
    struct Vector freelist;
    struct Vector strings; // packed vector of char*
    struct Vector strings_map; // sparse hash vector of char*
    char* buffer;
    uint32_t buffer_capacity;
    uint32_t max_probe_dist;
};

void String_Set_Init(struct String_Set* set, uint32_t buffer_capacity, uint32_t string_capacity, uint32_t freelist_count)
{
    Vector_Reserve(&set->freelist, sizeof(struct String_Set_Free_Element), freelist_count);
    Vector_Reserve(&set->strings, sizeof(char*), string_capacity);
    Vector_Reserve(&set->strings_map, sizeof(char*), string_capacity * 2);
    set->buffer_capacity = buffer_capacity;
    set->buffer = malloc(buffer_capacity);
    set->max_probe_dist = 0;
}

void String_Set_Free(struct String_Set* set)
{
    Vector_Free(&set->freelist);
    Vector_Free(&set->strings);
    Vector_Free(&set->strings_map);
    free(set->buffer);
}

// FNV algorithm https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp
static uint32_t String_Set_Hash(char* key) {
    uint32_t hash = 2166136261u;
    for (uint8_t* p = (uint8_t*)key; *p; p++) {
        hash ^= *p;
        hash *= 16777619u;
    }
    return hash;
}

static void String_Set_Indices_Resize(struct String_Set* set)
{
    Vector_Grow(&set->strings_map);

    // Rebuild map
    set->max_probe_dist = 0;
    for (int i=0; i<set->strings_map.size; i++) {
        char* empty_slot = NULL;
        Vector_Set(&set->strings_map, i, &empty_slot);
    }
    for (int i=0; i<set->strings.size; i++) {
        char* string = *(char**) Vector_Get(&set->strings, i);
        uint32_t searches = 0;
        int32_t hash = String_Set_Hash(string);
        while (searches < set->strings_map.capacity) {
            uint32_t i = (hash + searches) % set->strings_map.capacity; 
            char* map_string = *(char**) Vector_Get(&set->strings_map, i);
            if (map_string == NULL) {
                Vector_Set(&set->strings_map, i, &string);
                break;
            }
            searches++;
        }
        set->max_probe_dist = max(set->max_probe_dist, searches);
    }
}
char* String_Set_Get(struct String_Set* set, char* key)
{
    uint32_t searches = 0;
    uint32_t hash = String_Set_Hash(key);
    uint32_t max_searches = min(set->strings_map.capacity, set->max_probe_dist + 1);
    while (searches < max_searches) {
        uint32_t i = (hash + searches) % set->strings_map.capacity; 
        char* map_string = *(char**) Vector_Get(&set->strings_map, i);

        if (map_string == NULL) return NULL;

        // Check for match
        if (strcmp(map_string, key) == 0) {
            set->max_probe_dist = max(set->max_probe_dist, searches);
            return map_string; // Found
        }
        searches++;
    }
    
    return NULL; // Not found
}

char* String_Set_Add(struct String_Set* set, char* string)
{
    // If the string already exists in the set -> return to avoid duplicates
    char* get = String_Set_Get(set, string);
    if (get != NULL) return get;

    uint32_t len = (uint32_t)strlen(string) + 1;
    char* result;
    int space_found = 0;
    int right_most_free_index = -1;

    // Search freelist
    for (int i=0; i<set->freelist.size; i++) 
    {
        struct String_Set_Free_Element* free = Vector_Get(&set->freelist, i);

        right_most_free_index = max(right_most_free_index, free->index + free->size);

        if (len < free->size) 
        {
            memcpy(set->buffer + free->index, string, len);
            result = (char*)set->buffer + free->index;
            free->index += len;
            free->size -= len;
            space_found = 1;
            break;
        }
        else if (len == free->size) 
        {
            memcpy(set->buffer + free->index, string, len);
            Vector_Delete_Backfill(&set->freelist, i);
            result = (char*)set->buffer + free->index;
            space_found = 1;
            break;
        }
    }

    if (!space_found) // No space found -> grow buffer then add string to the set
    {
        uint32_t buffer_index = right_most_free_index == -1 ? set->buffer_capacity : right_most_free_index;
        set->buffer_capacity = max(set->buffer_capacity * 2, set->buffer_capacity + len + 10);
        set->buffer = realloc(set->buffer, (size_t)set->buffer_capacity);
        memcpy(set->buffer + buffer_index, string, len);
        struct String_Set_Free_Element new_free = { buffer_index + len, set->buffer_capacity - buffer_index - len };
        Vector_Push(&set->freelist, &new_free);
        result = (char*)set->buffer + buffer_index;
    }

    // Add result to string vector
    Vector_Push(&set->strings, &result);


    // Update string map
    uint32_t searches = 0;
    uint32_t hash = String_Set_Hash(string);
    while (searches < set->strings_map.capacity) {
        uint32_t i = (hash + searches) % set->strings_map.capacity;
        char* map_string = *(char**) Vector_Get(&set->strings_map, i);
        if (map_string == NULL) // Found free slot
        {
            Vector_Set(&set->strings_map, i, &result);
            break;
        } 
        searches++;
    }
    set->max_probe_dist = max(set->max_probe_dist, searches);
    return result;
}   
