#pragma once

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// A static hashmap is one whereby (key [string], item [generic]) pairs
// can only be added and not removed
// this is a major optimisation for 
// cache efficiency and it negates
// the need for a key heap allocator
// keys can be added in a linear fashion
struct sHashmap 
{
    uint8_t* occupancy; 
    char* keybuffer; // text arena of keys
    char** keys; // partitions the key buffer into individual strings
    void* data; // packed vector of underlying data
    uint32_t capacity;
    uint32_t keybuffer_capacity;
    uint32_t keybuffer_end;
    uint32_t item_size;
    uint32_t item_count;
    uint32_t max_probes;
};

void sHashmap_Init(struct sHashmap* shmap, uint32_t item_size, uint32_t item_capacity)
{
    
    shmap->capacity = max(10, item_capacity * 2);

    // Compute number of occupancy bytes
    uint32_t occupancy_remainder = shmap->capacity & 7; // capacity % 8
    uint32_t occupancy_bytes = shmap->capacity >> 3; // capacity / 8
    if (occupancy_remainder != 0) occupancy_bytes += 1;
    shmap->occupancy = calloc(occupancy_bytes, 1); 

    shmap->keybuffer_capacity = max(10, item_capacity) * 12; // expecting ~12 chars per key
    shmap->keybuffer_end = 0;
    shmap->keybuffer = malloc(shmap->keybuffer_capacity);
    shmap->keys = malloc(shmap->capacity * sizeof(char*));
    shmap->item_size = item_size;
    shmap->item_count = 0;
    shmap->max_probes = 0;
    shmap->data = malloc(shmap->capacity * item_size);
}

void sHashmap_Free(struct sHashmap* shmap)
{
    free(shmap->occupancy);
    free(shmap->keybuffer);
    free(shmap->keys);
    free(shmap->data);
    shmap->capacity = 0;
    shmap->keybuffer_capacity = 0;
    shmap->item_size = 0;
    shmap->item_count = 0;
    shmap->max_probes = 0;
}

// FNV algorithm https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp
static uint32_t Hash_String(char* key)
{
    uint32_t hash = 2166136261u;
    for (uint8_t* p = (uint8_t*)key; *p; p++) {
        hash ^= *p;
        hash *= 16777619u;
    }
    return hash;
}

static void sHashmap_add(struct sHashmap* shmap, char* key, void* value)
{
    uint32_t probes = 0;
    uint32_t hash = Hash_String(key);
    while (probes < shmap->capacity) {
        uint32_t i = (hash + probes) % shmap->capacity;
        uint32_t rem = i & 7; // i % 8
        uint32_t occupancy_index = i >> 3; // i / 8

        if (!(shmap->occupancy[occupancy_index] & (uint8_t)(1 << rem))) { // Found empty slot
            shmap->occupancy[occupancy_index] |= (uint8_t)(1 << rem); // Mark occupied

            // set key
            size_t key_len = strlen(key) + 1; // +1 for '\0'
            char* key_dst = shmap->keybuffer + shmap->keybuffer_end;
            memcpy(key_dst, key, key_len);
            shmap->keys[i] = key_dst;
            shmap->keybuffer_end += (uint32_t)key_len;

            // set value
            char* dst = (char*)shmap->data + shmap->item_size * i;
            memcpy(dst, value, shmap->item_size);

            shmap->item_count++;
            break;
        }
        probes++;
    }
    shmap->max_probes = max(shmap->max_probes, probes);
}

static void sHashmap_Resize(struct sHashmap* shmap)
{
    uint32_t old_capacity = shmap->capacity;
    uint8_t* old_occupancy = shmap->occupancy;
    char** old_keys = shmap->keys;
    char* old_data = (char*)shmap->data;

    // Double capacity
    shmap->capacity *= 2;
    uint32_t occupancy_remainder = shmap->capacity & 7; // hmap->capacity % 8
    uint32_t occupancy_bytes = shmap->capacity >> 3; // hmap->capacity / 8
    if (occupancy_remainder != 0) occupancy_bytes += 1;
    shmap->occupancy = calloc(occupancy_bytes, 1);
    shmap->keys = malloc(shmap->capacity * sizeof(char*));
    shmap->data = malloc(shmap->capacity * shmap->item_size);
    shmap->max_probes = 0;
    shmap->item_count = 0;

    // Re-insert all old items
    for (uint32_t i=0; i<old_capacity; i++) {
        uint32_t rem = i & 7; // i % 8
        uint32_t occupancy_index = i >> 3; // i / 8
        if (old_occupancy[occupancy_index] & (1u << rem)) {
            void* value = old_data + i * shmap->item_size;
            char* key = old_keys[i];

            // Add item 
            sHashmap_add(shmap, key, value);
        }
    }

    free(old_occupancy);
    free(old_keys);
    free(old_data);
}

void sHashmap_Add(struct sHashmap* shmap, char* key, void* value)
{
    // Resize keybuffer if new key would overflow capacity
    uint32_t key_len = (uint32_t)strlen(key) + 1;
    if (shmap->keybuffer_end + key_len > shmap->keybuffer_capacity) {
        shmap->keybuffer_capacity = max(shmap->keybuffer_capacity * 2, shmap->keybuffer_capacity + key_len);
        shmap->keybuffer = realloc(shmap->keybuffer, (size_t)shmap->keybuffer_capacity);
    }

    // Resize if surpased max load factor 
    if ((float)shmap->item_count / (float)shmap->capacity > 0.5f) {
        sHashmap_Resize(shmap);
    }


    uint32_t probes = 0;
    uint32_t hash = Hash_String(key);
    while (probes < shmap->capacity) {
        uint32_t i = (hash + probes) % shmap->capacity;
        uint32_t rem = i & 7; // i % 8
        uint32_t occupancy_index = i >> 3; // i / 8

        if (!(shmap->occupancy[occupancy_index] & (1u << rem))) { // Found empty slot
            shmap->occupancy[occupancy_index] |= (uint8_t)(1 << rem); // Mark occupied

            // set key
            size_t key_len = strlen(key) + 1; // +1 for '\0'
            char* key_dst = shmap->keybuffer + shmap->keybuffer_end;
            memcpy(key_dst, key, key_len);
            shmap->keys[i] = key_dst;
            shmap->keybuffer_end += (uint32_t)key_len;

            // set value
            char* dst = (char*)shmap->data + shmap->item_size * i;
            memcpy(dst, value, shmap->item_size);

            shmap->item_count++;
            break;
        }
        probes++;
    }
    shmap->max_probes = max(shmap->max_probes, probes);
}

void* sHashmap_Find(struct sHashmap* shmap, char* key)
{   
    uint32_t probes = 0;
    uint32_t hash = Hash_String(key);
    uint32_t max_searches = min(shmap->capacity, shmap->max_probes + 1);
    while (probes < max_searches) {
        uint32_t i = (hash + probes) % shmap->capacity;
        uint32_t rem = i & 7; // i % 8
        uint32_t occupancy_index = i >> 3; // i / 8

        if (shmap->occupancy[occupancy_index] & (1u << rem)) { // Found item

            // check if key matches
            char* item_key = shmap->keys[i];
            if (strcmp(key, item_key) == 0) {
                return (char*)shmap->data + i * shmap->item_size;
            }
        } else { // empty slot -> not present
            return NULL;
        }
        probes++;
    }
    return NULL;
}













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

void Hashmap_Init(struct Hashmap* hmap, uint32_t key_size, uint32_t item_size, uint32_t capacity)
{
    capacity = max(capacity, 2);

    // Compute number of occupancy bytes
    uint32_t occupancy_remainder = capacity & 7; // capacity % 8
    uint32_t occupancy_bytes = capacity >> 3; // capacity / 8
    if (occupancy_remainder != 0) occupancy_bytes += 1;

    hmap->occupancy = calloc(occupancy_bytes, 1); 
    hmap->data = malloc((key_size + item_size) * capacity);
    hmap->key_size = key_size;
    hmap->item_size = item_size;
    hmap->capacity = capacity;
}

static void Hashmap_Resize_Readd(struct Hashmap* hmap, void* key, void* value)
{
    uint32_t probes = 0;
    uint32_t hash = Hash_Generic(key, hmap->key_size);
    while (probes < hmap->capacity) {
        uint32_t i = (hash + probes) % hmap->capacity;
        uint32_t rem = i & 7; // i % 8
        uint32_t occupancy_index = i >> 3; // i / 8

        if (!(hmap->occupancy[occupancy_index] & (uint8_t)(1 << rem))) { // Found empty slot
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
            Hashmap_Resize_Readd(hmap, key, value); // Re-add item 
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
            void* check_key = hmap->data + i * (hmap->key_size + hmap->item_size);
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
            void* check_key = hmap->data + i * (hmap->key_size + hmap->item_size);
            if (memcmp(check_key, key, hmap->key_size) == 0) {
                return hmap->data + i * (hmap->key_size + hmap->item_size) + hmap->item_size;
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