#pragma once

#include <stdint.h>
#include <string.h>
#include "datastructures/vector.h"
#include "datastructures/string_set.h"
#include "datastructures/hashmap.h"

struct NU_Stylesheet_Tag_Pseudo_Pair
{
    int tag;
    int pseudo_class;
};

struct NU_Stylesheet_String_Pseudo_Pair
{
    char* string;
    int pseudo_class;
};

struct NU_Stylesheet
{
    struct Vector items;
    String_Set class_string_set;
    String_Set id_string_set;
    struct Hashmap class_item_hashmap;
    struct Hashmap id_item_hashmap;
    struct Hashmap tag_item_hashmap;

    // (NU_Stylesheet_Tag_Pseudo_Pair -> style item index) in items vector
    struct Hashmap tag_pseudo_item_hashmap; 
    struct Hashmap class_pseudo_item_hashmap;
    struct Hashmap id_pseudo_item_hashmap;
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
    uint8_t background_r, background_g, background_b;
    uint8_t border_r, border_g, border_b;
    uint8_t text_r, text_g, text_b;
    char layout_flags;
    char horizontal_alignment;
    char vertical_alignment;
    char horizontal_text_alignment;
    char vertical_text_alignment;
    bool hide_background;
};

void NU_Stylesheet_Init(struct NU_Stylesheet* ss)
{
    Vector_Reserve(&ss->items, sizeof(struct NU_Stylesheet_Item), 500);
    String_Set_Init(&ss->class_string_set, 1024, 100);
    String_Set_Init(&ss->id_string_set, 1024, 100);
    Hashmap_Init(&ss->class_item_hashmap, sizeof(char*), sizeof(uint32_t), 100);
    Hashmap_Init(&ss->id_item_hashmap, sizeof(char*), sizeof(uint32_t), 100);
    Hashmap_Init(&ss->tag_item_hashmap, sizeof(int), sizeof(uint32_t), 20);
    Hashmap_Init(&ss->tag_pseudo_item_hashmap, sizeof(struct NU_Stylesheet_Tag_Pseudo_Pair), sizeof(uint32_t), 20);
    Hashmap_Init(&ss->class_pseudo_item_hashmap, sizeof(struct NU_Stylesheet_String_Pseudo_Pair), sizeof(uint32_t), 20);
    Hashmap_Init(&ss->id_pseudo_item_hashmap, sizeof(struct NU_Stylesheet_String_Pseudo_Pair), sizeof(uint32_t), 20);
}

void NU_Stylesheet_Free(struct NU_Stylesheet* ss)
{
    Vector_Free(&ss->items);
    String_Set_Free(&ss->class_string_set);
    String_Set_Free(&ss->id_string_set);
    Hashmap_Free(&ss->class_item_hashmap);
    Hashmap_Free(&ss->id_item_hashmap);
    Hashmap_Free(&ss->tag_item_hashmap);
    Hashmap_Free(&ss->tag_pseudo_item_hashmap);
    Hashmap_Free(&ss->class_pseudo_item_hashmap);
    Hashmap_Free(&ss->id_pseudo_item_hashmap);
}
