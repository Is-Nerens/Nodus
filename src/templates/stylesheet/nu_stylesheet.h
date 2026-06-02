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

void NU_Stylesheet_Init(NU_Stylesheet* ss)
{
    ArrayInit(&ss->items, sizeof(NU_Stylesheet_Item), 512);
    LinearStringsetInit(&ss->class_string_set, 1024, 128);
    LinearStringsetInit(&ss->id_string_set, 1024, 128);
    HashmapInit(&ss->class_item_hashmap, sizeof(char*), sizeof(u32), 128);
    HashmapInit(&ss->id_item_hashmap, sizeof(char*), sizeof(u32), 128);
    HashmapInit(&ss->tag_item_hashmap, sizeof(int), sizeof(u32), 16);
    HashmapInit(&ss->tag_pseudo_item_hashmap, sizeof(NU_Stylesheet_Tag_Pseudo_Pair), sizeof(u32), 16);
    HashmapInit(&ss->class_pseudo_item_hashmap, sizeof(NU_Stylesheet_String_Pseudo_Pair), sizeof(u32), 16);
    HashmapInit(&ss->id_pseudo_item_hashmap, sizeof(NU_Stylesheet_String_Pseudo_Pair), sizeof(u32), 16);
    LinearStringmapInit(&ss->fontNameIndexMap, sizeof(int), 12, 128);
    ss->fonts = Container_Create(sizeof(NU_Font));

    // Create default style item
    NU_Stylesheet_Item* item = &ss->defaultStyleItem;
    memset(item, 0, sizeof(NU_Stylesheet_Item)); // zero base
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
    memset(&ss->scrollbarStyle, 0, sizeof(NU_Stylesheet_Scrollbar_Style)); // zero base
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

void NU_Stylesheet_Free(NU_Stylesheet* ss)
{
    ArrayFree(&ss->items);
    LinearStringsetFree(&ss->class_string_set);
    LinearStringsetFree(&ss->id_string_set);
    HashmapFree(&ss->class_item_hashmap);
    HashmapFree(&ss->id_item_hashmap);
    HashmapFree(&ss->tag_item_hashmap);
    HashmapFree(&ss->tag_pseudo_item_hashmap);
    HashmapFree(&ss->class_pseudo_item_hashmap);
    HashmapFree(&ss->id_pseudo_item_hashmap);
    LinearStringmapFree(&ss->fontNameIndexMap);
    Container_Free(&ss->fonts);
}

int NU_Stylesheet_Create(NU_Stylesheet* stylesheet, const char* filepath, ImageResourceLoader* imageResourceLoader)
{
    NU_Stylesheet_Init(stylesheet);

    // Load CSS file into memory
    String src = FileReadUTF8(filepath);
    if (src == NULL) return 0;

    // Init token and text ref vectors and reserve
    TokenArray tokens = TokenArray_Create(8000);
    struct Array textRefs; ArrayInit(&textRefs, sizeof(struct Style_Text_Ref), 2000);

    // Tokenise and generate stylesheet
    NU_Style_Tokenise(src, &tokens, &textRefs);
    if (!NU_Stylesheet_Parse(StringCstr(src), &tokens, &textRefs, stylesheet, imageResourceLoader)) {
        TokenArray_Free(&tokens);
        ArrayFree(&textRefs);
        StringFree(src);
        printf("CSS parsing failed!"); return 0;
    }

    // Free memory
    TokenArray_Free(&tokens);
    ArrayFree(&textRefs);
    StringFree(src);
    return 1; // Success
}

int NU_Internal_Apply_Stylesheet(NU_Stylesheet* stylesheet)
{
    // Traverse tree using DFS
    BreadthFirstSearch_Reset(&GUI.bfs, GUI.tree.root);
    NodeP* node;
    while (BreadthFirstSearch_Next(&GUI.bfs, &node)) {
        NU_Apply_Stylesheet_To_Node(node, stylesheet);
    }
    return 1; // success
}

inline NU_Font* Stylesheet_Get_Font(NU_Stylesheet* ss, u8 fontID) {

    return Container_Get(&ss->fonts, fontID);
}