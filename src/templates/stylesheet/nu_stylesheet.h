#pragma once
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <utils/nu_convert.h>
#include <filesystem/nu_file.h>
#include <datastructures/string.h>
#include <datastructures/linear_stringset.h>
#include <datastructures/container.h>
#include "nu_stylesheet_tokens.h"
#include "nu_stylesheet_structs.h"
#include "nu_stylesheet_tokeniser.h"
#include "nu_stylesheet_parser.h"
#include "nu_stylesheet_apply.h"

void NU_Stylesheet_Init(NU_Stylesheet* ss)
{
    Vector_Reserve(&ss->items, sizeof(NU_Stylesheet_Item), 512);
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

    // zero base
    memset(item, 0, sizeof(NU_Stylesheet_Item));

    // apply all properties
    item->propertyFlags = ~(uint64_t)0;

    // do not apply certain properties
    item->propertyFlags &= ~PROPERTY_FLAG_IMAGE;

    // sizing
    item->maxWidth  = UINT16_MAX;
    item->maxHeight = UINT16_MAX;

    // positioning
    item->left = -1;
    item->right = -1;
    item->top = -1;
    item->bottom = -1;

    // colors
    item->backgroundR = 50;
    item->backgroundG = 50;
    item->backgroundB = 50;

    item->borderR = 100;
    item->borderG = 100;
    item->borderB = 100;

    item->textR = 255;
    item->textG = 255;
    item->textB = 255;

    // alignment
    item->horizontalTextAlignment = 1;
    item->verticalTextAlignment = 1;
}

void NU_Stylesheet_Free(NU_Stylesheet* ss)
{
    Vector_Free(&ss->items);
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

int NU_Stylesheet_Create(NU_Stylesheet* stylesheet, const char* filepath)
{
    NU_Stylesheet_Init(stylesheet);

    // Load CSS file into memory
    String src = FileReadUTF8(filepath);
    if (src == NULL) return 0;

    // Init token and text ref vectors and reserve
    TokenArray tokens = TokenArray_Create(8000);
    struct Vector textRefs; Vector_Reserve(&textRefs, sizeof(struct Style_Text_Ref), 2000);

    // Tokenise and generate stylesheet
    NU_Style_Tokenise(src, &tokens, &textRefs);
    if (!NU_Stylesheet_Parse(StringCstr(src), &tokens, stylesheet, &textRefs)) {
        TokenArray_Free(&tokens);
        Vector_Free(&textRefs);
        StringFree(src);
        printf(" CSS parsing failed!"); return 0;
    }

    // Free memory
    TokenArray_Free(&tokens);
    Vector_Free(&textRefs);
    StringFree(src);
    return 1; // Success
}

u32 NU_Internal_Load_Stylesheet(const char* filepath)
{
    NU_Stylesheet* stylesheet = Vector_Create_Uninitialised(&GUI.stylesheets);
    if (!NU_Stylesheet_Create(stylesheet, filepath)) return 0; // Failure
    u32 stylesheet_handle = GUI.stylesheets.size;
    return stylesheet_handle;
}

int NU_Internal_Apply_Stylesheet(u32 stylesheet_handle)
{
    NU_Stylesheet* stylesheet = Vector_Get_Safe(&GUI.stylesheets, stylesheet_handle - 1);   
    if (stylesheet == NULL) return 0;

    GUI.stylesheet = stylesheet;

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