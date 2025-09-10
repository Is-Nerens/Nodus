#pragma once

#include <stdint.h>
#include <string.h>
#include "vector.h"

struct NU_Stylesheet_Freelist_Item
{
    uint32_t index;
    uint32_t size;
};

struct NU_Stylesheet
{
    struct Vector items;

    // Store class names and ids
    // with fast lookup capabilities
    struct Vector classnames_freelist;
    struct Vector ids_freelist;
    char* classnames_buffer;
    char* ids_buffer;
    uint32_t classnames_buffer_capacity;
    uint32_t ids_buffer_capacity;
};

struct NU_Stylesheet_Item
{
    uint64_t property_flags;
    char* class;
    char* id;
    int tag;
    int item_index;
    float preferred_width, preferred_height;
    float min_width, max_width, min_height, max_height;
    float gap;
    uint8_t pad_top, pad_bottom, pad_left, pad_right;
    uint8_t border_top, border_bottom, border_left, border_right;
    uint8_t border_radius_tl, border_radius_tr, border_radius_bl, border_radius_br;
    uint8_t background_r, background_g, background_b, background_a;
    uint8_t border_r, border_g, border_b, border_a;
    char layout_flags;
    char horizontal_alignment;
    char vertical_alignment;
};

void NU_Stylesheet_Init(struct NU_Stylesheet* ss)
{
    Vector_Reserve(&ss->items, sizeof(struct NU_Stylesheet_Item), 1000);
    Vector_Reserve(&ss->classnames_freelist, sizeof(struct NU_Stylesheet_Freelist_Item), 300);
    Vector_Reserve(&ss->ids_freelist, sizeof(struct NU_Stylesheet_Freelist_Item), 300);

    // Add one freelist item to each freelist
    struct NU_Stylesheet_Freelist_Item free = {0, 2000};
    Vector_Push(&ss->classnames_freelist, &free);
    Vector_Push(&ss->ids_freelist, &free);

    ss->classnames_buffer = malloc(2000);
    ss->ids_buffer = malloc(2000);
    ss->classnames_buffer_capacity = 2000;
    ss->ids_buffer_capacity = 2000;
}

// O(1) lookup function (at the moment this is O(N) quite slow)
int NU_Stylesheet_Get_Item_Index_From_Classname(struct NU_Stylesheet* ss, char* class)
{
    for (int i=0; i<ss->items.size; i++)
    {
        struct NU_Stylesheet_Item* item = (struct NU_Stylesheet_Item*) Vector_Get(&ss->items, i);
        if (item->class != NULL) {
            if (strcmp(item->class, class) == 0) {
                return i;
            }
        }
    }
    return -1;
}

// O(1) lookup function (at the moment this is O(N) quite slow)
int NU_Stylesheet_Get_Item_Index_From_Id(struct NU_Stylesheet* ss, char* id)
{
    for (int i=0; i<ss->items.size; i++)
    {
        struct NU_Stylesheet_Item* item = (struct NU_Stylesheet_Item*) Vector_Get(&ss->items, i);
        if (item->id != NULL) {
            if (strcmp(item->id, id) == 0) {
                return i;
            }
        }
    }
    return -1;
}

char* NU_Stylesheet_Add_Classname(struct NU_Stylesheet* ss, char* classname) {
    uint32_t len = (uint32_t)strlen(classname) + 1;

    // Check if classname already exists
    int existing_index = NU_Stylesheet_Get_Item_Index_From_Classname(ss, classname);
    if (existing_index != -1) {
        struct NU_Stylesheet_Freelist_Item* free = (struct NU_Stylesheet_Freelist_Item*)Vector_Get(&ss->classnames_freelist, existing_index);
        return ss->classnames_buffer + free->index;
    }

    uint32_t buffer_index = 0;
    int found_space = 0;

    // Search freelist
    for (int i=0; i<ss->classnames_freelist.size; i++) {
        struct NU_Stylesheet_Freelist_Item* free = (struct NU_Stylesheet_Freelist_Item*)Vector_Get(&ss->classnames_freelist, i);
        if (len < free->size) {
            buffer_index = free->index;
            memcpy(ss->classnames_buffer + buffer_index, classname, len);
            free->index += len;
            free->size -= len;
            found_space = 1;
            break;
        } else if (len == free->size) {
            buffer_index = free->index;
            memcpy(ss->classnames_buffer + buffer_index, classname, len);
            Vector_Delete_Backfill(&ss->classnames_freelist, i);
            found_space = 1;
            break;
        }
    }
    

    if (!found_space) {
        // Grow buffer
        ss->classnames_buffer_capacity = max(ss->classnames_buffer_capacity * 2, ss->classnames_buffer_capacity + len + 10);
        ss->classnames_buffer = realloc(ss->classnames_buffer, (size_t)ss->classnames_buffer_capacity);

        buffer_index = (ss->classnames_freelist.size > 0)
            ? ((struct NU_Stylesheet_Freelist_Item*)Vector_Get(&ss->classnames_freelist, ss->classnames_freelist.size - 1))->index +
              ((struct NU_Stylesheet_Freelist_Item*)Vector_Get(&ss->classnames_freelist, ss->classnames_freelist.size - 1))->size
            : 0;

        memcpy(ss->classnames_buffer + buffer_index, classname, len);

        // Push remaining free block
        struct NU_Stylesheet_Freelist_Item new_free = {
            buffer_index + len,
            ss->classnames_buffer_capacity - (buffer_index + len)
        };
        Vector_Push(&ss->classnames_freelist, &new_free);
    }

    return ss->classnames_buffer + buffer_index;
}

char* NU_Stylesheet_Add_Id(struct NU_Stylesheet* ss, char* id) {
    uint32_t len = (uint32_t)strlen(id) + 1;

    // Check if id already exists
    int existing_index = NU_Stylesheet_Get_Item_Index_From_Id(ss, id);
    if (existing_index != -1) {
        struct NU_Stylesheet_Freelist_Item* free = (struct NU_Stylesheet_Freelist_Item*)Vector_Get(&ss->ids_freelist, existing_index);
        return ss->ids_buffer + free->index;
    }

    uint32_t buffer_index = 0;
    int found_space = 0;

    // Search freelist
    for (int i = 0; i < ss->ids_freelist.size; i++) {
        struct NU_Stylesheet_Freelist_Item* free = (struct NU_Stylesheet_Freelist_Item*)Vector_Get(&ss->ids_freelist, i);
        if (len < free->size) {
            buffer_index = free->index;
            memcpy(ss->ids_buffer + buffer_index, id, len);
            free->index += len;
            free->size -= len;
            found_space = 1;
            break;
        } else if (len == free->size) {
            buffer_index = free->index;
            memcpy(ss->ids_buffer + buffer_index, id, len);
            Vector_Delete_Backfill(&ss->ids_freelist, i);
            found_space = 1;
            break;
        }
    }

    if (!found_space) {
        // Grow buffer
        ss->ids_buffer_capacity = max(ss->ids_buffer_capacity * 2, ss->ids_buffer_capacity + len + 10);
        ss->ids_buffer = realloc(ss->ids_buffer, (size_t)ss->ids_buffer_capacity);

        buffer_index = (ss->ids_freelist.size > 0)
            ? ((struct NU_Stylesheet_Freelist_Item*)Vector_Get(&ss->ids_freelist, ss->ids_freelist.size - 1))->index +
              ((struct NU_Stylesheet_Freelist_Item*)Vector_Get(&ss->ids_freelist, ss->ids_freelist.size - 1))->size
            : 0;

        memcpy(ss->ids_buffer + buffer_index, id, len);

        // Push remaining free block
        struct NU_Stylesheet_Freelist_Item new_free = {
            buffer_index + len,
            ss->ids_buffer_capacity - (buffer_index + len)
        };
        Vector_Push(&ss->ids_freelist, &new_free);
    }

    return ss->ids_buffer + buffer_index;
}