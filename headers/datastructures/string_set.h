#pragma once
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint32_t index;
    uint32_t size;
    uint16_t chunk;
} String_Set_Free_Element;

typedef struct {
    struct Vector freelist;
    char** strings_map;        // sparse hash vector of char*
    char** buffer_chunks;      // array of char arrays
    uint32_t chunk_size;       // chars in each chunk
    uint16_t chunks_used;      // number of chunks
    uint16_t chunks_available; // number of chunks
    uint32_t string_map_capacity;
    uint32_t total_buffer_capacity;
    uint32_t string_count;
    uint32_t max_probes;
} String_Set; 

void String_Set_Init(String_Set* set, uint32_t chunk_size, uint32_t string_capacity)
{
    // Garuntee minimum capacity
    chunk_size = MAX(chunk_size, 128);
    string_capacity = MAX(string_capacity, 16);
    
    // Init state variables
    set->chunk_size = chunk_size;
    set->total_buffer_capacity = chunk_size;
    set->chunks_used = 1;
    set->string_count = 0;
    set->max_probes = 0;

    // Init chunks array with one char* array
    set->chunks_available = 4;
    set->buffer_chunks = malloc(sizeof(char*) * 4);
    set->buffer_chunks[0] = malloc(chunk_size);
    set->buffer_chunks[1] = NULL;
    set->buffer_chunks[2] = NULL;
    set->buffer_chunks[3] = NULL;

    // Init strings map and freelist
    set->string_map_capacity = (string_capacity * 2);
    set->strings_map = calloc(set->string_map_capacity, sizeof(char*)); // Init all slots to NULL
    Vector_Reserve(&set->freelist, sizeof(String_Set_Free_Element), string_capacity / 2);

    // Add one free element
    String_Set_Free_Element first_free;
    first_free.chunk = 0;
    first_free.index = 0;
    first_free.size = chunk_size;
    Vector_Push(&set->freelist, &first_free);
}

void String_Set_Free(String_Set* set)
{
    Vector_Free(&set->freelist);
    free(set->strings_map);
    for (uint16_t i=0; i<set->chunks_available; i++) {
        if (set->buffer_chunks[i] != NULL) free(set->buffer_chunks[i]);
    }
    free(set->buffer_chunks);
}

static uint32_t String_Set_Hash(char* string) {

    // FNV algorithm https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp
    uint32_t hash = 2166136261u;
    for (uint8_t* p = (uint8_t*)string; *p; p++) {
        hash ^= *p;
        hash *= 16777619u;
    }
    return hash;
}

char* String_Set_Get(String_Set* set, char* key)
{
    uint32_t searches = 0;
    uint32_t hash = String_Set_Hash(key);
    uint32_t max_searches = MIN(set->string_map_capacity, set->max_probes + 1);
    while (searches < max_searches) {
        uint32_t i = (hash + searches) % set->string_map_capacity;
        char* map_string = set->strings_map[i];

        // String is not in the set
        if (map_string == NULL) {
            return NULL;
        }

        // Check for match 
        if (strcmp(map_string, key) == 0) {
            // set->max_probes = MAX(set->max_probes, searches);
            return map_string;
        }

        searches++;
    }
    return NULL; // String is not in the set
}

static void String_Set_Rehash(String_Set* set)
{
    uint32_t old_map_capacity = set->string_map_capacity;
    char** old_strings_map = set->strings_map;
    
    // Resize strings map and init elements to NULL
    set->string_map_capacity *= 2;
    set->strings_map = calloc(set->string_map_capacity, sizeof(char*)); // Init all slots to NULL

    // --------------------------------------------------
    // --- Re-insert strings into new map
    // --------------------------------------------------
    set->max_probes = 0;
    for (uint32_t i=0; i<old_map_capacity; i++) 
    {
        char* old_string = old_strings_map[i];
        if (old_string != NULL) // Re-hash and insert into new map
        {   
            // --------------------------------------------------
            // --- Rehash and insert existing string into new map
            // --------------------------------------------------
            uint32_t searches = 0;
            uint32_t hash = String_Set_Hash(old_string);
            while (searches < set->string_map_capacity)
            {
                uint32_t idx = (hash + searches) % set->string_map_capacity;
                char* new_map_string = set->strings_map[idx];
                if (new_map_string == NULL) // Found a slot in new map
                {
                    set->strings_map[idx] = old_string;
                    break;
                }
                searches++;
            }
            set->max_probes = MAX(set->max_probes, searches);
            // --------------------------------------------------
            // --- Rehash and insert existing string into new map
            // --------------------------------------------------
        }
    }
    free(old_strings_map);
}

char* String_Set_Add(String_Set* set, char* string)
{
    // Ensure string is not already in the set
    char* get = String_Set_Get(set, string);
    if (get != NULL) return get;

    // If the hashmap load factor is too high -> expand and rehash
    float load_factor = (float)set->string_count / (float)set->string_map_capacity;
    if (load_factor > 0.7f) {
        String_Set_Rehash(set);
    }

    uint32_t string_len = (uint32_t)strlen(string) + 1; 

    // Search freelist for space
    int space_index = -1;
    uint16_t space_chunk = 0;
    for (uint32_t i=0; i<set->freelist.size; i++)
    {
        String_Set_Free_Element* free_element = Vector_Get(&set->freelist, i);
        if (string_len < free_element->size) { // Consume part of free element
            space_index = (int)free_element->index;
            space_chunk = free_element->chunk;
            free_element->size -= string_len;
            free_element->index += string_len;
            break;
        } 
        else if (string_len == free_element->size) { // Consume entire free element -> remove from list
            space_index = (int)free_element->index;
            space_chunk = free_element->chunk;
            Vector_Delete_Backshift(&set->freelist, i);
            break;
        }
    }

    // No space anywhere? -> allocate a new chunk
    if (space_index == -1)
    {
        // Increase chunk size
        set->chunk_size = set->total_buffer_capacity;
        set->total_buffer_capacity *= 2;

        // Add another chunk
        set->chunks_used++;
        if (set->chunks_used == set->chunks_available) {
            set->chunks_available *= 2;
            set->buffer_chunks = realloc(set->buffer_chunks, set->chunks_available * sizeof(char*));
        }
        set->buffer_chunks[set->chunks_used-1] = malloc(set->chunk_size);

        // Add a free element for new chunk
        String_Set_Free_Element new_free = { 
            0, 
            set->chunk_size, 
            set->chunks_used-1, 
        };
        Vector_Push(&set->freelist, &new_free);
        space_index = 0;
        space_chunk = set->chunks_used-1;
    }

    // Insert string into underlying char buffer
    char* chunk = set->buffer_chunks[space_chunk];
    char* result = chunk + space_index;
    memcpy(result, string, string_len);
    set->string_count++;

    // Update string map
    uint32_t searches = 0;
    uint32_t hash = String_Set_Hash(string);
    while (searches < set->string_map_capacity)
    {
        uint32_t i = (hash + searches) % set->string_map_capacity;
        char* map_string = set->strings_map[i];
        if (map_string == NULL) // Found free slot
        {
            set->strings_map[i] = result;
            break;
        }
        searches++;
    }
    set->max_probes = MAX(set->max_probes, searches);
    return result;
}