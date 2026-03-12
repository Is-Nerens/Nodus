#pragma once
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <utils/nu_convert.h>
#include <filesystem/nu_file.h>
#include <datastructures/string.h>
#include <datastructures/linear_stringset.h>
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
    HashmapInit(&ss->class_item_hashmap, sizeof(char*), sizeof(uint32_t), 128);
    HashmapInit(&ss->id_item_hashmap, sizeof(char*), sizeof(uint32_t), 128);
    HashmapInit(&ss->tag_item_hashmap, sizeof(int), sizeof(uint32_t), 16);
    HashmapInit(&ss->tag_pseudo_item_hashmap, sizeof(NU_Stylesheet_Tag_Pseudo_Pair), sizeof(uint32_t), 16);
    HashmapInit(&ss->class_pseudo_item_hashmap, sizeof(NU_Stylesheet_String_Pseudo_Pair), sizeof(uint32_t), 16);
    HashmapInit(&ss->id_pseudo_item_hashmap, sizeof(NU_Stylesheet_String_Pseudo_Pair), sizeof(uint32_t), 16);
    LinearStringmapInit(&ss->fontNameIndexMap, sizeof(int), 12, 128);
    Vector_Reserve(&ss->fonts, sizeof(NU_Font), 4);

    // Create default style item
    NU_Stylesheet_Item* item = &ss->defaultStyleItem;
    memset(item, 0, sizeof(NU_Stylesheet_Item)); // Default struct to 0
    item->propertyFlags = ~(uint64_t)0; // Set all bits to 1
    item->propertyFlags &= ~PROPERTY_FLAG_IMAGE; // Clear
    item->maxWidth = 10e20f;
    item->maxHeight = 10e20f;
    item->left = item->right = item->top = item->bottom = -1;
    item->backgroundR = item->backgroundG = item->backgroundB = 50;
    item->borderR = item->borderG = item->borderB = 100;
    item->textR = item->textG = item->textB = 255;
    item->horizontalAlignment = 1;
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
    Vector_Free(&ss->fonts);
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

uint32_t NU_Internal_Load_Stylesheet(const char* filepath)
{
    NU_Stylesheet* stylesheet = Vector_Create_Uninitialised(&__NGUI.stylesheets);
    if (!NU_Stylesheet_Create(stylesheet, filepath)) return 0; // Failure
    uint32_t stylesheet_handle = __NGUI.stylesheets.size;
    return stylesheet_handle;
}

int NU_Internal_Apply_Stylesheet(uint32_t stylesheet_handle)
{
    NU_Stylesheet* stylesheet = Vector_Get_Safe(&__NGUI.stylesheets, stylesheet_handle - 1);   
    if (stylesheet == NULL) return 0;

    // Traverse tree using DFS
    DepthFirstSearch dfs = DepthFirstSearch_Create(__NGUI.tree.root);
    NodeP* node;
    while (DepthFirstSearch_Next(&dfs, &node)) 
    {
        NU_Apply_Stylesheet_To_Node(node, stylesheet);
    }
    DepthFirstSearch_Free(&dfs);

    __NGUI.stylesheet = stylesheet;
    return 1; // success
}