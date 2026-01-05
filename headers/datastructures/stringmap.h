#pragma once 

// ------------------------------------
// --- Import -------------------------
// ------------------------------------
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// ------------------------------------
// --- Hash Functions -----------------
// ------------------------------------
// FNV algorithm https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp
static uint32_t String_Hash(char* string) {
    uint32_t hash = 2166136261u;
    for (uint8_t* p = (uint8_t*)string; *p; p++) {
        hash ^= *p;
        hash *= 16777619u;
    }
    return hash;
}


// -----------------------------------------------------
// --- Datastructure For Mapping (string -> generic) ---
// -----------------------------------------------------
typedef struct {
    uint32_t chunk;
    uint32_t index;
    uint32_t size;
} NU_Stringmap_Free_Element;

typedef struct {
    char** strings_map;        // sparse hash vector of char*
    char** buffer_chunks;      // array of char arrays (stores keys)
    void* item_data;           // sparse array of items
    NU_Stringmap_Free_Element* freelist;
    uint32_t first_chunk_size; // chars in first chunk
    uint16_t chunks_allocated;      
    uint16_t chunk_list_capacity; 
    uint32_t capacity;
    uint32_t total_buffer_capacity;
    uint32_t item_count;
    uint32_t item_size;
    uint32_t freelist_capacity;
    uint32_t freelist_size;
    uint32_t max_probes;
    uint32_t iterate_index;
    uint32_t iterate_count;
} NU_Stringmap;

void NU_Stringmap_Init(NU_Stringmap* map, uint32_t item_size, uint32_t chunk_size, uint32_t capacity)
{
    // Garuntee minimum capacity
    chunk_size = MAX(chunk_size, 100);
    capacity = MAX(capacity, 10);
    
    // Init state variables
    map->first_chunk_size = chunk_size;
    map->total_buffer_capacity = chunk_size;
    map->chunks_allocated = 1;
    map->item_count = 0;
    map->item_size = item_size;
    map->max_probes = 0;

    // Init chunks array with one char* array
    map->chunk_list_capacity = 4;
    map->buffer_chunks = malloc(sizeof(char*) * 4);
    map->buffer_chunks[0] = malloc(chunk_size);
    map->buffer_chunks[1] = NULL;
    map->buffer_chunks[2] = NULL;
    map->buffer_chunks[3] = NULL;

    // Init data, strings map and freelist
    map->capacity = (capacity * 2);
    map->strings_map = calloc(map->capacity, sizeof(char*)); // Set all slots to NULL
    map->item_data = malloc(item_size * map->capacity);

    // Init freelist
    map->freelist_capacity = 64;
    map->freelist_size = 0;
    map->freelist = malloc(sizeof(NU_Stringmap_Free_Element) * map->freelist_capacity);

    // Add first free element
    NU_Stringmap_Free_Element free;
    free.chunk = 0;
    free.index = 0;
    free.size = chunk_size;
    map->freelist_size = 1;
    map->freelist[0] = free;
}

void NU_Stringmap_Free(NU_Stringmap* map)
{
    for (uint16_t i=0; i<map->chunk_list_capacity; i++) 
    {
        if (map->buffer_chunks[i] != NULL) free(map->buffer_chunks[i]);
    }
    free(map->buffer_chunks);
    free(map->strings_map);
    free(map->item_data);
    free(map->freelist);
}

void* NU_Stringmap_Get(NU_Stringmap* map, char* key)
{
    uint32_t searches = 0;
    uint32_t hash = String_Hash(key);
    uint32_t max_searches = MIN(map->capacity, map->max_probes + 1);
    while (searches < max_searches) {
        uint32_t i = (hash + searches) % map->capacity;

        char* map_string = map->strings_map[i];

        // Item is not in the map
        if (map_string == NULL) {
            return NULL;
        }
        
        // Check for match 
        if (strcmp(map_string, key) == 0) {
            return (char*)map->item_data + i * map->item_size;
        }

        searches++;
    }
    return NULL; // Item is not in the map
}

bool NU_Stringmap_Contains(NU_Stringmap* map, char* key)
{
    uint32_t searches = 0;
    uint32_t hash = String_Hash(key);
    uint32_t max_searches = MIN(map->capacity, map->max_probes + 1);
    while (searches < max_searches) {
        uint32_t i = (hash + searches) % map->capacity;

        char* map_string = map->strings_map[i];

        // Item is not in the map
        if (map_string == NULL) {
            return false;
        }
        
        // Check for match 
        if (strcmp(map_string, key) == 0) {
            return true;
        }

        searches++;
    }

    return false; // Item is not in the map
}

void NU_Stringmap_Grow_Rehash(NU_Stringmap* map)
{
    uint32_t old_map_capacity = map->capacity;
    char** old_strings_map = map->strings_map;
    void* old_item_data = map->item_data;
    
    // Resize strings map and init elements to NULL
    map->capacity *= 2;
    map->item_data = malloc(map->item_size * map->capacity);
    map->strings_map = calloc(map->capacity, sizeof(char*)); // Init all slots to NULL

    // --------------------------------------------------
    // --- Re-insert strings into new map
    // --------------------------------------------------
    map->max_probes = 0;
    for (uint32_t i=0; i<old_map_capacity; i++) 
    {
        char* old_string = old_strings_map[i];
        char* old_item_loc = (char*)old_item_data + i * map->item_size;
        if (old_string != NULL) // Re-hash and insert into new map
        {   
            // --------------------------------------------------
            // --- Rehash and insert existing string into new map
            // --------------------------------------------------
            uint32_t searches = 0;
            uint32_t hash = String_Hash(old_string);
            while (searches < map->capacity)
            {
                uint32_t idx = (hash + searches) % map->capacity;
                char* new_map_string = map->strings_map[idx];
                if (new_map_string == NULL) // Found a slot in new map
                {
                    map->strings_map[idx] = old_string; // Set string (key)
                    char* item_dst = (char*)map->item_data + idx * map->item_size;
                    memcpy(item_dst, old_item_loc, map->item_size);
                    break;
                }
                searches++;
            }
            map->max_probes = MAX(map->max_probes, searches);
            // --------------------------------------------------
            // --- Rehash and insert existing string into new map
            // --------------------------------------------------
        }
    }
    free(old_strings_map);
    free(old_item_data);
}

void* NU_Stringmap_Set(NU_Stringmap* map, char* key, void* item)
{   
    // If the key already exists in the map -> update value and return
    void* exists = NU_Stringmap_Get(map, key);
    if (exists != NULL) {
        memcpy(exists, item, map->item_size);
        return exists;
    }

    // If the hashmap load factor is too high -> expand and rehash
    float load_factor = (float)map->item_count / (float)map->capacity;
    if (load_factor > 0.7f) {
        NU_Stringmap_Grow_Rehash(map);
    }

    // --------------------------------------------------
    // --- Rehash and insert existing key into new map
    // --------------------------------------------------
    uint32_t string_len = (uint32_t)strlen(key) + 1;

    // Search key freelist for space
    int space_index = -1;
    uint32_t chunk_index = 0;
    for (uint32_t i=0; i<map->freelist_size; i++)
    {
        NU_Stringmap_Free_Element* free_element = &map->freelist[i];
        if (string_len < free_element->size) { // Consume part of free element
            space_index = (int)free_element->index;
            chunk_index = free_element->chunk;
            free_element->size -= string_len;
            free_element->index += string_len;
            break;
        }
        else if (string_len == free_element->size) { // Consume entire free element -> remove from list
            space_index = (int)free_element->index;
            chunk_index = free_element->chunk;
            map->freelist_size -= 1;
            if (i != map->freelist_size) {
                memmove(
                    &map->freelist[i],                                         // destination
                    &map->freelist[i + 1],                                     // source (next element)
                    (map->freelist_size - i) * sizeof(NU_Stringmap_Free_Element) // bytes to move
                );
            }
            break;
        }
    }

    // No space anywhere? -> allocate a new chunk
    if (space_index == -1)
    {
        uint32_t chunk_size = map->total_buffer_capacity; // Effectively doubles the total capacity
        map->total_buffer_capacity *= 2;

        // Add another chunk
        map->chunks_allocated++;
        if (map->chunks_allocated == map->chunk_list_capacity) {
            map->chunk_list_capacity *= 2;
            map->buffer_chunks = realloc(map->buffer_chunks, map->chunk_list_capacity * sizeof(char*));
            for (uint16_t z=map->chunks_allocated; z<map->chunk_list_capacity; z++) {
                map->buffer_chunks[z] = NULL;
            }
        }
        map->buffer_chunks[map->chunks_allocated-1] = malloc(chunk_size);

        // Add a free element
        if (map->freelist_size == map->freelist_capacity) {
            map->freelist_capacity *= 2;
            map->freelist = realloc(map->freelist, map->freelist_capacity * sizeof(NU_Stringmap_Free_Element));
        }
        NU_Stringmap_Free_Element new_free;
        new_free.chunk = map->chunks_allocated-1;
        new_free.index = string_len;
        new_free.size = chunk_size - string_len;
        map->freelist[map->freelist_size] = new_free;
        map->freelist_size += 1;
        space_index = 0;
        chunk_index = new_free.chunk; 
    }

    // Insert key into underlying char buffer
    char* chunk = map->buffer_chunks[chunk_index];
    char* result = chunk + space_index;
    memcpy(result, key, string_len);
    map->item_count++;

    // Update string map
    uint32_t searches = 0;
    uint32_t hash = String_Hash(key);
    void* stored_item = NULL;
    while (searches < map->capacity)
    {
        uint32_t i = (hash + searches) % map->capacity;
        char* map_string = map->strings_map[i];
        if (map_string == NULL) // Found free slot
        {
            map->strings_map[i] = result; // Set key
            char* item_dst = (char*)map->item_data + i * map->item_size;
            stored_item = item_dst;
            memcpy(item_dst, item, map->item_size);
            break;
        }
        searches++;
    }
    map->max_probes = MAX(map->max_probes, searches);
    return stored_item;
}

void NU_Stringmap_Delete(NU_Stringmap* map, char* key)
{
    // Mark the key as free in strings_map 
    char* stored_key = NULL;
    uint32_t hash = String_Hash(key);
    uint32_t max_searches = MIN(map->capacity, map->max_probes + 1);
    uint32_t searches = 0;
     while (searches < max_searches) {
        uint32_t i = (hash + searches) % map->capacity;
        char* map_string = map->strings_map[i];
        if (map_string == NULL) { // Key not found
            return; 
        }
        if (strcmp(map_string, key) == 0) {
            stored_key = map_string;
            map->strings_map[i] = NULL; // mark as free
            break;
        }
        searches++;
    }


    uint32_t string_len = (uint32_t)strlen(key) + 1;
    uint32_t string_index = UINT32_MAX;
    uint32_t chunk_index = UINT32_MAX;
    for (uint32_t i=0; i<map->chunks_allocated; i++) {
        char* chunk = map->buffer_chunks[i];

        // Calculate the chunk size
        uint64_t chunk_size = (i < 2) 
            ? (uint64_t)map->first_chunk_size 
            : (uint64_t)map->first_chunk_size << (i - 1);

        // Check if string is inside chunk
        if (stored_key >= chunk && stored_key + string_len <= chunk + chunk_size) {
            string_index = (uint32_t)(stored_key - chunk);
            chunk_index = i;
            break; 
        }
    }

    if (string_index == UINT32_MAX) return; // Key string is not in the map buffers

    // Create a new free struct
    NU_Stringmap_Free_Element new_free;
    new_free.chunk = chunk_index;
    new_free.index = string_index;
    new_free.size = string_len;

    // Update the freelist
    int left_adjacent_touch_idx = -1;
    int right_adjacent_touch_idx = -1;
    uint32_t insert_idx = 0;
    for (uint32_t i=0; i<map->freelist_size; i++) {
        NU_Stringmap_Free_Element* f = &map->freelist[i];
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
        map->freelist[left_adjacent_touch_idx].size += new_free.size + map->freelist[right_adjacent_touch_idx].size;

        // remove right neighbor
        map->freelist_size--;
        memmove(&map->freelist[right_adjacent_touch_idx],
                &map->freelist[right_adjacent_touch_idx + 1],
                (map->freelist_size - right_adjacent_touch_idx) * sizeof(NU_Stringmap_Free_Element));
    }
    // Case 2: Free item touches left adjacent free item -> merge with left
    else if (left_adjacent_touch_idx != -1) {
        map->freelist[left_adjacent_touch_idx].size += new_free.size;
    }
    // Case 3: Free item touches right adjacent free item -> merge with right
    else if (right_adjacent_touch_idx != -1) {
        map->freelist[right_adjacent_touch_idx].index = new_free.index;
        map->freelist[right_adjacent_touch_idx].size += new_free.size;
    }
    // Case 4: Free item touches no adjacent free items
    else {
        // Case 4: no neighbors -> insert sorted
        if (map->freelist_size >= map->freelist_capacity) {
            map->freelist_capacity *= 2;
            map->freelist = realloc(map->freelist,
                                    map->freelist_capacity * sizeof(NU_Stringmap_Free_Element));
        }

        memmove(&map->freelist[insert_idx + 1],
                &map->freelist[insert_idx],
                (map->freelist_size - insert_idx) * sizeof(NU_Stringmap_Free_Element));

        map->freelist[insert_idx] = new_free;
        map->freelist_size++;
    }

    map->item_count--;
}

void NU_Stringmap_Clear(NU_Stringmap* map)
{
    if (map->item_count == 0) return;

    // 1. Reset the freelist
    map->freelist_size = 0;
    NU_Stringmap_Free_Element free_item;
    free_item.chunk = 0;
    free_item.index = 0;
    free_item.size = map->first_chunk_size;
    map->freelist_size = 1;
    map->freelist[0] = free_item;

    // 2. Free unused chunks
    for (uint16_t i=1; i<map->chunks_allocated; i++) {
        free(map->buffer_chunks[i]);
    }
    map->chunks_allocated = 1;

    // 3. Set all strings_map slots to NULL
    memset(map->strings_map, 0, map->capacity * sizeof(char*));

    map->item_count = 0;
    map->max_probes = 0;
}

void NU_Stringmap_Iterate_Begin(NU_Stringmap* map)
{
    map->iterate_index = 0;
    map->iterate_count = 0;
}

int NU_Stringmap_Iterate_Continue(NU_Stringmap* map)
{   
    return map->iterate_index < map->capacity && map->item_count > 0 && map->iterate_count < map->item_count;
}

void NU_Stringmap_Iterate_Get(NU_Stringmap* map, char** return_key, void** return_val)
{
    if (map->item_count == 0) {
        *return_key = NULL;
        *return_val = NULL;
        return;
    }

    while (map->iterate_index < map->capacity) {
        char* map_string = map->strings_map[map->iterate_index];

        if (map_string != NULL) {
            *return_key = map_string;
            *return_val = (char*)map->item_data + map->iterate_index * map->item_size;
            map->iterate_index++;
            map->iterate_count++;
            return;
        }

        map->iterate_index++;
    }

    *return_key = NULL;
    *return_val = NULL;
}