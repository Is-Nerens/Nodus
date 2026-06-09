#pragma once
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <utils/nu_convert.h>
#include <filesystem/nu_file.h>
#include "nu_stylesheet_tokens.h"
#include "nu_stylesheet_structs.h"
#include "nu_stylesheet_tokeniser.h"
#include "nu_stylesheet_parser.h"
#include "nu_stylesheet_apply.h"

void Stylesheet_Init(Stylesheet* ss)
{
    Array_Init(&ss->items, sizeof(Stylesheet_Item), 512);
    LinearStringset_Init(&ss->class_string_set, 1024, 128);
    LinearStringset_Init(&ss->id_string_set, 1024, 128);
    Hashmap_Init(&ss->class_item_hashmap, sizeof(char*), sizeof(u32), 128);
    Hashmap_Init(&ss->id_item_hashmap, sizeof(char*), sizeof(u32), 128);
    Hashmap_Init(&ss->tag_item_hashmap, sizeof(int), sizeof(u32), 16);
    Hashmap_Init(&ss->tag_pseudo_item_hashmap, sizeof(Stylesheet_Tag_Pseudo_Pair), sizeof(u32), 16);
    Hashmap_Init(&ss->class_pseudo_item_hashmap, sizeof(Stylesheet_String_Pseudo_Pair), sizeof(u32), 16);
    Hashmap_Init(&ss->id_pseudo_item_hashmap, sizeof(Stylesheet_String_Pseudo_Pair), sizeof(u32), 16);
    LinearStringmap_Init(&ss->fontNameIndexMap, sizeof(int), 12, 128);
    ss->fonts = Container_Create(sizeof(NU_Font));

    // Create default style item
    Stylesheet_Item* item = &ss->defaultStyleItem;
    memset(item, 0, sizeof(Stylesheet_Item)); // zero base
    item->propertyFlags = ~(uint64_t)0; // apply all properties
    item->propertyFlags &= ~PROPERTY_FLAG_IMAGE; // do not apply certain properties
    item->propertyFlags &= ~PROPERTY_FLAG_HIDDEN;
    item->propertyFlags &= ~PROPERTY_FLAG_TOP;
    item->propertyFlags &= ~PROPERTY_FLAG_BOTTOM;
    item->propertyFlags &= ~PROPERTY_FLAG_LEFT;
    item->propertyFlags &= ~PROPERTY_FLAG_RIGHT;
    item->propertyFlags &= ~PROPERTY_FLAG_MIN_WIDTH;
    item->propertyFlags &= ~PROPERTY_FLAG_MIN_HEIGHT;
    item->propertyFlags &= ~PROPERTY_FLAG_MAX_WIDTH;
    item->propertyFlags &= ~PROPERTY_FLAG_MAX_HEIGHT;
    item->propertyFlags &= ~PROPERTY_FLAG_PREFERRED_WIDTH;
    item->propertyFlags &= ~PROPERTY_FLAG_PREFERRED_HEIGHT;
    item->backgroundR = 50; // colors
    item->backgroundG = 50;
    item->backgroundB = 50;
    item->borderR = 100;
    item->borderG = 100;
    item->borderB = 100;
    item->textR = 255;
    item->textG = 255;
    item->textB = 255;
    item->horizontalTextAlignment = 1; // alignment
    item->verticalTextAlignment = 1;

    // Set default scrollbar styles
    memset(&ss->scrollbarStyle, 0, sizeof(Stylesheet_Scrollbar_Style)); // zero base
    ss->scrollbarStyle.width = 8;
    ss->scrollbarStyle.height = 8;
    ss->scrollbarStyle.overlay = false;
    ss->scrollbarStyle.thumbMinSize = 4;
    ss->scrollbarStyle.thumbBackgroundR = 210;
    ss->scrollbarStyle.thumbBackgroundG = 210;
    ss->scrollbarStyle.thumbBackgroundB = 210;
    ss->scrollbarStyle.thumbBorderR = 240;
    ss->scrollbarStyle.thumbBorderG = 240;
    ss->scrollbarStyle.thumbBorderB = 240;
    ss->scrollbarStyle.trackBackgroundR = 40;
    ss->scrollbarStyle.trackBackgroundG = 40;
    ss->scrollbarStyle.trackBackgroundB = 40;
    ss->scrollbarStyle.trackBorderR = 40;
    ss->scrollbarStyle.trackBorderG = 40;
    ss->scrollbarStyle.trackBorderB = 40;
}

void Stylesheet_Free(Stylesheet* ss)
{
    Array_Free(&ss->items);
    LinearStringset_Free(&ss->class_string_set);
    LinearStringset_Free(&ss->id_string_set);
    Hashmap_Free(&ss->class_item_hashmap);
    Hashmap_Free(&ss->id_item_hashmap);
    Hashmap_Free(&ss->tag_item_hashmap);
    Hashmap_Free(&ss->tag_pseudo_item_hashmap);
    Hashmap_Free(&ss->class_pseudo_item_hashmap);
    Hashmap_Free(&ss->id_pseudo_item_hashmap);
    LinearStringmap_Free(&ss->fontNameIndexMap);
    Container_Free(&ss->fonts);
}

int Stylesheet_Create(Stylesheet* stylesheet, const char* filepath, ImageResourceLoader* imageResourceLoader)
{
    Stylesheet_Init(stylesheet);

    // Load CSS file into memory
    String src = FileReadUTF8(filepath);
    if (src == NULL) return 0;

    // Init token and text ref vectors and reserve
    TokenArray tokens = TokenArray_Create(8000);
    struct Array textRefs; Array_Init(&textRefs, sizeof(struct Style_Text_Ref), 2000);

    // Tokenise and generate stylesheet
    NU_Style_Tokenise(src, &tokens, &textRefs);
    if (!Stylesheet_Parse(StringCstr(src), &tokens, &textRefs, stylesheet, imageResourceLoader)) {
        TokenArray_Free(&tokens);
        Array_Free(&textRefs);
        StringFree(src);
        printf("CSS parsing failed!"); return 0;
    }

    // Free memory
    TokenArray_Free(&tokens);
    Array_Free(&textRefs);
    StringFree(src);
    return 1; // Success
}

int NU_Internal_Apply_Stylesheet(Stylesheet* stylesheet)
{
    // Traverse tree using DFS
    BreadthFirstSearch_Reset(&GUI.bfs, GUI.tree.root);
    NodeP* node;
    while (BreadthFirstSearch_Next(&GUI.bfs, &node)) {
        NU_Apply_Stylesheet_To_Node(node, stylesheet);
    }
    return 1; // success
}

inline NU_Font* Stylesheet_Get_Font(Stylesheet* ss, u8 fontID) {

    return Container_Get(&ss->fonts, fontID);
}