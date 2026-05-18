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

typedef struct NU_Stylesheet_Item
{
    uint64_t propertyFlags;
    char* class;
    char* id;
    int tag;
    int item_index;
    GLuint glImageHandle;
    u16 prefWidth, prefHeight;
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

typedef struct NU_Stylesheet_Scrollbar_Style {
    // Bar
    u8 width;
    u8 height;
    bool overlay;
    // Thumb
    u8 thumbMinSize;
    u8 thumbBorderTop, thumbBorderBottom, thumbBorderLeft, thumbBorderRight;
    u8 thumbBorderRadiusTl, thumbBorderRadiusTr, thumbBorderRadiusBl, thumbBorderRadiusBr;
    u8 thumbBackgroundR, thumbBackgroundG, thumbBackgroundB;
    u8 thumbBorderR, thumbBorderG, thumbBorderB;
    // Track
    u8 trackPadTop, trackPadBottom, trackPadLeft, trackPadRight;
    u8 trackBorderTop, trackBorderBottom, trackBorderLeft, trackBorderRight;
    u8 trackBorderRadiusTl, trackBorderRadiusTr, trackBorderRadiusBl, trackBorderRadiusBr;
    u8 trackBackgroundR, trackBackgroundG, trackBackgroundB;
    u8 trackBorderR, trackBorderG, trackBorderB;
} NU_Stylesheet_Scrollbar_Style;

typedef struct NU_Stylesheet
{
    Array items;
    Array pseudoItems;
    LinearStringset class_string_set;
    LinearStringset id_string_set;
    Hashmap class_item_hashmap;
    Hashmap id_item_hashmap;
    Hashmap tag_item_hashmap;
    Hashmap tag_pseudo_item_hashmap; 
    Hashmap class_pseudo_item_hashmap;
    Hashmap id_pseudo_item_hashmap;
    LinearStringmap fontNameIndexMap;
    Container fonts;
    NU_Stylesheet_Item defaultStyleItem;
    NU_Stylesheet_Scrollbar_Style scrollbarStyle;
} NU_Stylesheet;

struct Style_Text_Ref
{
    u32 NU_Token_index;
    u32 src_index;
    u8 char_count; 
};