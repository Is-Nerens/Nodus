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
    Vector_Reserve(&ss->fonts, sizeof(NU_Font), 4);
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
    Vector_Free(&ss->fonts);
}

int NU_Stylesheet_Create(NU_Stylesheet* stylesheet, char* filepath)
{
    NU_Stylesheet_Init(stylesheet);

    // Load CSS file into memory
    String src = FileReadUTF8(filepath);
    if (src == NULL) return 0;

    // Init token and text ref vectors and reserve
    struct Vector tokens;
    struct Vector textRefs;
    Vector_Reserve(&tokens, sizeof(enum NU_Style_Token), 8000);
    Vector_Reserve(&textRefs, sizeof(struct Style_Text_Ref), 2000);

    // Tokenise and generate stylesheet
    NU_Style_Tokenise(src, &tokens, &textRefs);
    if (!NU_Stylesheet_Parse(StringCstr(src), &tokens, stylesheet, &textRefs)) {
        Vector_Free(&tokens);
        Vector_Free(&textRefs);
        StringFree(src);
        printf(" CSS parsing failed!"); return 0;
    }


    // Free memory
    Vector_Free(&tokens);
    Vector_Free(&textRefs);
    StringFree(src);
    return 1; // Success
}

uint32_t NU_Internal_Load_Stylesheet(char* filepath)
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

    // For each layer
    for (uint32_t l=0; l<__NGUI.tree.depth; l++)
    {
        Layer* layer = &__NGUI.tree.layers[l];
        for (uint32_t i=0; i<layer->size; i++)
        {
            NodeP* node = &layer->nodeArray[i]; if (!node->state) continue;
            NU_Apply_Stylesheet_To_Node(node, stylesheet);
        }
    }

    __NGUI.stylesheet = stylesheet;
    return 1; // success
}