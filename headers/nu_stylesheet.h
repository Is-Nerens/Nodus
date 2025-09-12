#pragma once

#include <stdint.h>
#include <string.h>
#include "vector.h"
#include "string_set.h"
#include "hashmap.h"

struct NU_Stylesheet
{
    struct Vector items;
    struct String_Set class_string_set;
    struct String_Set id_string_set;
    struct sHashmap class_item_hashmap;
    struct sHashmap id_item_hashmap;
    struct Hashmap tag_item_hashmap;
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
    Vector_Reserve(&ss->items, sizeof(struct NU_Stylesheet_Item), 500);
    String_Set_Init(&ss->class_string_set, 10000, 100, 100);
    String_Set_Init(&ss->id_string_set, 4000, 40, 40);
    sHashmap_Init(&ss->class_item_hashmap, sizeof(uint32_t), 100);
    sHashmap_Init(&ss->id_item_hashmap, sizeof(uint32_t), 100);
    Hashmap_Init(&ss->tag_item_hashmap, sizeof(int), sizeof(uint32_t), 20);
}

void NU_Stylesheet_Free(struct NU_Stylesheet* ss)
{
    Vector_Free(&ss->items);
    String_Set_Free(&ss->class_string_set);
    String_Set_Free(&ss->id_string_set);
    sHashmap_Free(&ss->class_item_hashmap);
    sHashmap_Free(&ss->id_item_hashmap);
    Hashmap_Free(&ss->tag_item_hashmap);
}
