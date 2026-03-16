#pragma once
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int16_t i16;

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

typedef struct NU_Stylesheet_Item
{
    uint64_t propertyFlags;
    char* class;
    char* id;
    int tag;
    int item_index;
    GLuint glImageHandle;
    u16 preferred_width, preferred_height;
    u16 minWidth, maxWidth, minHeight, maxHeight;
    i16 left, right, top, bottom;
    u16 layoutFlags;
    u8 gap, padTop, padBottom, padLeft, padRight;
    u8 borderTop, borderBottom, borderLeft, borderRight;
    u8 borderRadiusTl, borderRadiusTr, borderRadiusBl, borderRadiusBr;
    u8 backgroundR, backgroundG, backgroundB;
    u8 borderR, borderG, borderB;
    u8 textR, textG, textB;
    u8 fontId; // index of NU_Font in stylesheet fonts vector
    char horizontalAlignment;
    char verticalAlignment;
    char horizontalTextAlignment;
    char verticalTextAlignment;
    u8 inputType;
} NU_Stylesheet_Item;

typedef struct NU_Stylesheet
{
    struct Vector items;
    LinearStringset class_string_set;
    LinearStringset id_string_set;
    struct Hashmap class_item_hashmap;
    struct Hashmap id_item_hashmap;
    struct Hashmap tag_item_hashmap;
    struct Hashmap tag_pseudo_item_hashmap; 
    struct Hashmap class_pseudo_item_hashmap;
    struct Hashmap id_pseudo_item_hashmap;
    LinearStringmap fontNameIndexMap;
    Container fonts;
    NU_Stylesheet_Item defaultStyleItem;
} NU_Stylesheet;

struct Style_Text_Ref
{
    uint32_t NU_Token_index;
    uint32_t src_index;
    uint8_t char_count; 
};