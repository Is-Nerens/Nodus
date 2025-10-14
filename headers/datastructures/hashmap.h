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

static uint32_t Hash_Generic(void* key, uint32_t len) // FNV algorithm https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp
{
    uint32_t hash = 2166136261u;
    uint8_t* p = (uint8_t*)key;
    for (uint32_t i=0; i<len; i++) {
        hash ^= p[i];
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
} String_Map_Free_Element;

typedef struct {
    char** strings_map;        // sparse hash vector of char*
    char** buffer_chunks;      // array of char arrays
    void* item_data;           // sparse array of items
    String_Map_Free_Element* freelist;
    uint32_t first_chunk_size; // chars in first chunk
    uint16_t chunks_used;      // number of chunks
    uint16_t chunks_available; // number of chunks
    uint32_t string_map_capacity;
    uint32_t total_buffer_capacity;
    uint32_t string_count;
    uint32_t item_size;
    uint32_t freelist_capacity;
    uint32_t freelist_size;
    uint32_t max_probes;
} String_Map;

void String_Map_Init(String_Map* map, uint32_t item_size, uint32_t chunk_size, uint32_t capacity)
{
    // Garuntee minimum capacity
    chunk_size = MAX(chunk_size, 100);
    capacity = MAX(capacity, 10);
    
    // Init state variables
    map->first_chunk_size = chunk_size;
    map->total_buffer_capacity = chunk_size;
    map->chunks_used = 1;
    map->string_count = 0;
    map->item_size = item_size;
    map->max_probes = 0;

    // Init chunks array with one char* array
    map->chunks_available = 4;
    map->buffer_chunks = malloc(sizeof(char*) * 4);
    map->buffer_chunks[0] = malloc(chunk_size);
    map->buffer_chunks[1] = NULL;
    map->buffer_chunks[2] = NULL;
    map->buffer_chunks[3] = NULL;

    // Init data, strings map and freelist
    map->string_map_capacity = (capacity * 2);
    map->strings_map = calloc(map->string_map_capacity, sizeof(char*)); // Set all slots to NULL
    map->item_data = malloc(item_size * map->string_map_capacity);

    // Init freelist
    map->freelist_capacity = 64;
    map->freelist_size = 0;
    map->freelist = malloc(sizeof(String_Map_Free_Element) * map->freelist_capacity);

    // Add first free element
    String_Map_Free_Element free;
    free.chunk = 0;
    free.index = 0;
    free.size = chunk_size;
    map->freelist_size = 1;
    map->freelist[0] = free;
}

void String_Map_Free(String_Map* map)
{
    for (uint16_t i=0; i<map->chunks_available; i++) 
    {
        if (map->buffer_chunks[i] != NULL) free(map->buffer_chunks[i]);
    }
    free(map->buffer_chunks);
    free(map->strings_map);
    free(map->item_data);
    free(map->freelist);
}

void* String_Map_Get(String_Map* map, char* key)
{
    uint32_t searches = 0;
    uint32_t hash = String_Hash(key);
    uint32_t max_searches = MIN(map->string_map_capacity, map->max_probes + 1);
    while (searches < max_searches) {
        uint32_t i = (hash + searches) % map->string_map_capacity;
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

void String_Map_Grow_Rehash(String_Map* map)
{
    uint32_t old_map_capacity = map->string_map_capacity;
    char** old_strings_map = map->strings_map;
    void* old_item_data = map->item_data;
    
    // Resize strings map and init elements to NULL
    map->string_map_capacity *= 2;
    map->item_data = malloc(map->item_size * map->string_map_capacity);
    map->strings_map = calloc(map->string_map_capacity, sizeof(char*)); // Init all slots to NULL

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
            while (searches < map->string_map_capacity)
            {
                uint32_t idx = (hash + searches) % map->string_map_capacity;
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

void String_Map_Set(String_Map* map, char* key, void* item)
{   
    // If the hashmap load factor is too high -> expand and rehash
    float load_factor = (float)map->string_count / (float)map->string_map_capacity;
    if (load_factor > 0.7f) {
        String_Map_Grow_Rehash(map);
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
        String_Map_Free_Element* free_element = &map->freelist[i];
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
                    (map->freelist_size - i) * sizeof(String_Map_Free_Element) // bytes to move
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
        map->chunks_used++;
        if (map->chunks_used == map->chunks_available) {
            map->chunks_available *= 2;
            map->buffer_chunks = realloc(map->buffer_chunks, map->chunks_available * sizeof(char*));
        }
        map->buffer_chunks[map->chunks_used-1] = malloc(chunk_size);

        // Add a free element
        if (map->freelist_size == map->freelist_capacity) {
            map->freelist_capacity *= 2;
            map->freelist = realloc(map->freelist, map->freelist_capacity * sizeof(String_Map_Free));
        }
        String_Map_Free_Element new_free;
        new_free.chunk = map->chunks_used-1;
        new_free.index = 0;
        new_free.size = chunk_size;
        map->freelist[map->freelist_size] = new_free;
        map->freelist_size += 1;
        space_index = 0;
        chunk_index = new_free.chunk; 
    }

    // Insert key into underlying char buffer
    char* chunk = map->buffer_chunks[chunk_index];
    char* result = chunk + space_index;
    memcpy(result, key, string_len);
    map->string_count++;

    // Update string map
    uint32_t searches = 0;
    uint32_t hash = String_Hash(key);
    while (searches < map->string_map_capacity)
    {
        uint32_t i = (hash + searches) % map->string_map_capacity;
        char* map_string = map->strings_map[i];
        if (map_string == NULL) // Found free slot
        {
            map->strings_map[i] = result; // Set key
            char* item_dst = (char*)map->item_data + i * map->item_size;
            memcpy(item_dst, item, map->item_size);
            break;
        }
        searches++;
    }
    map->max_probes = MAX(map->max_probes, searches);
}

void String_Map_Delete(String_Map* map, char* key)
{
    uint32_t string_len = (uint32_t)strlen(key);
    uint32_t string_index = UINT32_MAX;
    uint32_t chunk_index = UINT32_MAX;
    for (uint32_t i=0; i<map->chunks_used; i++) {
        char* chunk = map->buffer_chunks[i];

        // Calculate the chunk size
        uint64_t chunk_size = (i < 2) 
            ? (uint64_t)map->first_chunk_size 
            : (uint64_t)map->first_chunk_size << (i - 1);

        // Check if string is inside chunk
        if (key >= chunk && key + string_len <= chunk + chunk_size) {
            string_index = (uint32_t)(key - chunk);
            chunk_index = i;
            break; 
        }
    }

    if (string_index == UINT32_MAX) return; // String is not in the map buffers

    // Create a new free struct
    String_Map_Free_Element new_free;
    new_free.chunk = chunk_index;
    new_free.index = string_index;
    new_free.size = string_len;

    // Update the freelist
    int left_adjacent_touch_idx = -1;
    int right_adjacent_touch_idx = -1;
    uint32_t insert_idx = 0;
    for (uint32_t i=0; i<map->freelist_size; i++) {
        String_Map_Free_Element* f = &map->freelist[i];
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
                (map->freelist_size - right_adjacent_touch_idx) * sizeof(String_Map_Free_Element));
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
                                    map->freelist_capacity * sizeof(String_Map_Free_Element));
        }

        memmove(&map->freelist[insert_idx + 1],
                &map->freelist[insert_idx],
                (map->freelist_size - insert_idx) * sizeof(String_Map_Free_Element));

        map->freelist[insert_idx] = new_free;
        map->freelist_size++;
    }
}














// ------------------------------------------------------
// --- Datastructure For Mapping (generic -> generic) ---
// ------------------------------------------------------
struct Hashmap
{
    uint8_t* occupancy;
    void* data;
    uint32_t key_size;
    uint32_t item_size;
    uint32_t item_count;
    uint32_t capacity;
    uint32_t max_probes;
};

void Hashmap_Init(struct Hashmap* hmap, uint32_t key_size, uint32_t item_size, uint32_t capacity)
{
    capacity = max(capacity, 10); // Ensure capacity for at least 10 element

    // Calculate number of bytes needed for occupancy bit array
    uint32_t occupancy_remainder = capacity & 7; // capacity % 8 
    uint32_t occupancy_bytes = capacity >> 3;    // capacity / 8
    if (occupancy_remainder != 0) occupancy_bytes += 1;

    // Allocate space for occupancy bit array and sparse data array
    hmap->occupancy = calloc(occupancy_bytes, 1); 
    hmap->data = malloc((key_size + item_size) * capacity);

    // Initialise tracking variables
    hmap->key_size = key_size;
    hmap->item_size = item_size;
    hmap->item_count = 0;
    hmap->capacity = capacity;
    hmap->max_probes = 0;
}

void Hashmap_Resize_Add(struct Hashmap* hmap, void* key, void* value)
{
    uint32_t probes = 0;
    uint32_t hash = Hash_Generic(key, hmap->key_size);
    while (probes < hmap->capacity) {
        uint32_t i = (hash + probes) % hmap->capacity;
        uint32_t rem = i & 7; // i % 8
        uint32_t occupancy_index = i >> 3; // i / 8

        if (!(hmap->occupancy[occupancy_index] & (uint8_t)(1 << rem))) { // Found empty slot
            hmap->occupancy[occupancy_index] |= (uint8_t)(1 << rem);     // Mark occupied

            // --- Set key and data ---
            char* base = (char*)hmap->data + i * (hmap->key_size + hmap->item_size);
            memcpy(base, key, hmap->key_size);
            memcpy(base + hmap->key_size, value, hmap->item_size);

            hmap->item_count++;
            break;
        }
        probes++;
    }
    hmap->max_probes = max(hmap->max_probes, probes);
}

void Hashmap_Resize(struct Hashmap* hmap)
{
    uint32_t old_capacity = hmap->capacity;
    uint8_t* old_occupancy = hmap->occupancy;
    void* old_data = hmap->data;

    // Resize
    hmap->capacity *= 2;
    uint32_t occupancy_remainder = hmap->capacity & 7; // hmap->capacity % 8
    uint32_t occupancy_bytes = hmap->capacity >> 3; // hmap->capacity / 8
    if (occupancy_remainder != 0) occupancy_bytes += 1;
    hmap->occupancy = calloc(occupancy_bytes, 1);
    hmap->data = malloc(hmap->capacity * (hmap->key_size + hmap->item_size));
    hmap->max_probes = 0;
    hmap->item_count = 0;

    // Re-insert all old items
    for (uint32_t i=0; i<old_capacity; i++) {
        uint32_t rem = i & 7; // i % 8
        uint32_t occupancy_index = i >> 3; // i / 8
        if (old_occupancy[occupancy_index] & (1u << rem)) // Found item
        { 
            char* base = (char*)old_data + i * (hmap->key_size + hmap->item_size);
            void* key = base;
            void* value = base + hmap->key_size;
            Hashmap_Resize_Add(hmap, key, value); // Re-add item 
        }
    }

    free(old_occupancy);
    free(old_data);
}

int Hashmap_Contains(struct Hashmap* hmap, void* key)
{
    uint32_t probes = 0;
    uint32_t hash = Hash_Generic(key, hmap->key_size);
    uint32_t max_probes = min(hmap->capacity, hmap->max_probes + 1);
    while (probes < max_probes) {
        uint32_t i = (hash + probes) % hmap->capacity;
        uint32_t rem = i & 7; // i % 8
        uint32_t occupancy_index = i >> 3; // i / 8

        if (hmap->occupancy[occupancy_index] & (1u << rem)) { // Found item
            
            // Check if key matches
            char* base = (char*)hmap->data + i * (hmap->key_size + hmap->item_size);
            void* check_key = base;
            if (memcmp(check_key, key, hmap->key_size) == 0) {
                return 1;
            }
        } else {
            return 0;
        }
        probes++;
    }

    return 0;
}

void* Hashmap_Get(struct Hashmap* hmap, void* key)
{
    uint32_t probes = 0;
    uint32_t hash = Hash_Generic(key, hmap->key_size);
    uint32_t max_probes = min(hmap->capacity, hmap->max_probes + 1);
    while (probes < max_probes) {
        uint32_t i = (hash + probes) % hmap->capacity;
        uint32_t rem = i & 7; // i % 8
        uint32_t occupancy_index = i >> 3; // i / 8

        if (hmap->occupancy[occupancy_index] & (1u << rem)) { // Found item
            
            // Check if key matches
            char* base = (char*)hmap->data + i * (hmap->key_size + hmap->item_size);
            void* check_key = base;
            if (memcmp(check_key, key, hmap->key_size) == 0) {
                return base + hmap->key_size;
            }
        }
        else {
            return NULL;
        }
        probes++;
    }

    return NULL;
}

void Hashmap_Set(struct Hashmap* hmap, void* key, void* value)
{
    // Resize if surpased max load factor 
    if ((float)hmap->item_count / (float)hmap->capacity > 0.5f) {
        Hashmap_Resize(hmap);
    }

    uint32_t probes = 0;
    uint32_t hash = Hash_Generic(key, hmap->key_size);
    while (probes < hmap->capacity) {
        uint32_t i = (hash + probes) % hmap->capacity;
        uint32_t rem = i & 7; // i % 8
        uint32_t occupancy_index = i >> 3; // i / 8

        if (!(hmap->occupancy[occupancy_index] & (1u << rem))) { // Found empty slot
            hmap->occupancy[occupancy_index] |= (uint8_t)(1 << rem); // Mark occupied
            
            // --- Set key and data ---
            char* base = (char*)hmap->data + i * (hmap->key_size + hmap->item_size);
            memcpy(base, key, hmap->key_size);
            memcpy(base + hmap->key_size, value, hmap->item_size);

            hmap->item_count++;
            break;
        }
        probes++;
    }
    hmap->max_probes = max(hmap->max_probes, probes);
}

void Hashmap_Delete(struct Hashmap* hmap, void* key)
{
    uint32_t probes = 0;
    uint32_t hash = Hash_Generic(key, hmap->key_size);
    uint32_t max_probes = min(hmap->capacity, hmap->max_probes + 1);
    int found_index = -1;
    int last_collision_match = -1;
    while (probes < max_probes) {
        uint32_t i = (hash + probes) % hmap->capacity;
        uint32_t rem = i & 7; // i % 8
        uint32_t occupancy_index = i >> 3; // i / 8
        uint8_t is_present = hmap->occupancy[occupancy_index] & (1u << rem);

        if (found_index == -1 && is_present) { // Found item
            
            // Check if key matches
            void* check_key = hmap->data + i * (hmap->key_size + hmap->item_size);
            if (memcmp(check_key, key, hmap->key_size) == 0) { // Found correct item -> delete
                hmap->occupancy[occupancy_index] &= ~(1u << rem); // Mark as free
                found_index = (int)i;
                hmap->item_count--;
            }
        } 
        else if (found_index != -1 && is_present) // Check if this one hashes to the same value
        {
            void* candidate_key = (char*)hmap->data + i * (hmap->key_size + hmap->item_size);
            uint32_t candidate_hash = Hash_Generic(candidate_key, hmap->key_size);
            if ((candidate_hash % hmap->capacity) == (uint32_t)found_index) 
            {
                last_collision_match = (int)i;
            }
            else if (last_collision_match != -1) // Backfill data and exit early 
            { 
                // Backfill data and exit
                uint32_t rem_from = last_collision_match & 7;
                uint32_t occupancy_index_from = last_collision_match >> 3;
                uint32_t rem_to = found_index & 7;
                uint32_t occupancy_index_to = found_index >> 3;
                memcpy((char*)hmap->data + found_index * (hmap->key_size + hmap->item_size),
                    (char*)hmap->data + last_collision_match * (hmap->key_size + hmap->item_size),
                    hmap->key_size + hmap->item_size);
                hmap->occupancy[occupancy_index_from] &= ~(1u << rem_from); // old slot free
                hmap->occupancy[occupancy_index_to] |= (1u << rem_to); 
                return;
            }
        }
        else if (found_index != -1 && !is_present) 
        {
            if (last_collision_match == -1) return; // No backfill necessary

            // Backfill data and exit
            uint32_t rem_from = last_collision_match & 7;
            uint32_t occupancy_index_from = last_collision_match >> 3;
            uint32_t rem_to = found_index & 7;
            uint32_t occupancy_index_to = found_index >> 3;
            memcpy((char*)hmap->data + found_index * (hmap->key_size + hmap->item_size),
                (char*)hmap->data + last_collision_match * (hmap->key_size + hmap->item_size),
                hmap->key_size + hmap->item_size);
            hmap->occupancy[occupancy_index_from] &= ~(1u << rem_from); // old slot free
            hmap->occupancy[occupancy_index_to] |= (1u << rem_to); 
            return;
        }
        probes++;
    }
}

void Hashmap_Free(struct Hashmap* hmap)
{
    free(hmap->occupancy);
    free(hmap->data);
}