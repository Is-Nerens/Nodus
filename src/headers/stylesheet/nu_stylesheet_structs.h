#pragma once

typedef struct NU_Stylesheet_Tag_Pseudo_Pair
{
    int tag;
    int pseudo_class;
} NU_Stylesheet_Tag_Pseudo_Pair;

typedef struct NU_Stylesheet_String_Pseudo_Pair
{
    char* string;
    int pseudo_class;
} NU_Stylesheet_String_Pseudo_Pair;

typedef struct NU_Stylesheet
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

    Stringmap font_name_index_map;

    Vector fonts;
} NU_Stylesheet;

typedef struct NU_Stylesheet_Item
{
    uint64_t propertyFlags;
    char* class;
    char* id;
    int tag;
    int item_index;
    GLuint glImageHandle;
    float preferred_width, preferred_height;
    float minWidth, maxWidth, minHeight, maxHeight;
    float gap;
    float left, right, top, bottom;
    uint8_t padTop, padBottom, padLeft, padRight;
    uint8_t borderTop, borderBottom, borderLeft, borderRight;
    uint8_t borderRadiusTl, borderRadiusTr, borderRadiusBl, borderRadiusBr;
    uint8_t backgroundR, backgroundG, backgroundB;
    uint8_t borderR, borderG, borderB;
    uint8_t textR, textG, textB;
    uint8_t fontId; // index of NU_Font in stylesheet fonts vector
    char layoutFlags;
    char horizontalAlignment;
    char verticalAlignment;
    char horizontalTextAlignment;
    char verticalTextAlignment;
    bool hideBackground;
} NU_Stylesheet_Item;

struct Style_Text_Ref
{
    uint32_t NU_Token_index;
    uint32_t src_index;
    uint8_t char_count; 
};