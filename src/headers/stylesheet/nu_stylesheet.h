#pragma once

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "nu_convert.h"
#include "nu_stylesheet_tokens.h"
#include "nu_stylesheet_structs.h"
#include "nu_stylesheet_tokeniser.h"
#include "nu_stylesheet_parser.h"
#include <image/nu_image.h>
#include <filesystem/file.h>
#include <datastructures/string.h>
#include <performance.h>



// -------------------------------------------------
// --- STYLESHEET INIT AND FREE ====================
// -------------------------------------------------
void NU_Stylesheet_Init(NU_Stylesheet* ss)
{
    Vector_Reserve(&ss->items, sizeof(NU_Stylesheet_Item), 500);
    String_Set_Init(&ss->class_string_set, 1024, 100);
    String_Set_Init(&ss->id_string_set, 1024, 100);
    HashmapInit(&ss->class_item_hashmap, sizeof(char*), sizeof(uint32_t), 100);
    HashmapInit(&ss->id_item_hashmap, sizeof(char*), sizeof(uint32_t), 100);
    HashmapInit(&ss->tag_item_hashmap, sizeof(int), sizeof(uint32_t), 20);
    HashmapInit(&ss->tag_pseudo_item_hashmap, sizeof(NU_Stylesheet_Tag_Pseudo_Pair), sizeof(uint32_t), 20);
    HashmapInit(&ss->class_pseudo_item_hashmap, sizeof(NU_Stylesheet_String_Pseudo_Pair), sizeof(uint32_t), 20);
    HashmapInit(&ss->id_pseudo_item_hashmap, sizeof(NU_Stylesheet_String_Pseudo_Pair), sizeof(uint32_t), 20);
    NU_Stringmap_Init(&ss->image_filepath_to_handle_hmap, sizeof(GLuint), 512, 20);
    NU_Stringmap_Init(&ss->font_name_index_map, sizeof(uint8_t), 128, 12);
    Vector_Reserve(&ss->fonts, sizeof(NU_Font), 4);
}

void NU_Stylesheet_Free(NU_Stylesheet* ss)
{
    Vector_Free(&ss->items);
    String_Set_Free(&ss->class_string_set);
    String_Set_Free(&ss->id_string_set);
    HashmapFree(&ss->class_item_hashmap);
    HashmapFree(&ss->id_item_hashmap);
    HashmapFree(&ss->tag_item_hashmap);
    HashmapFree(&ss->tag_pseudo_item_hashmap);
    HashmapFree(&ss->class_pseudo_item_hashmap);
    HashmapFree(&ss->id_pseudo_item_hashmap);
    NU_Stringmap_Free(&ss->font_name_index_map);
    Vector_Free(&ss->fonts);
}


// -------------------------------------------------
// --- STYLESHEET PARSING AND CREATION =============
// -------------------------------------------------

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




// -------------------------------------------------
// --- STYLESHEET APPLICATION TO NODES =============
// -------------------------------------------------
static void NU_Stylesheet_Find_Match(Node* node, NU_Stylesheet* ss, int* match_index_list)
{
    int count = 0;

    // Tag match first (lowest priority)
    void* tag_found = HashmapGet(&ss->tag_item_hashmap, &node->tag); 
    if (tag_found != NULL) { 
        match_index_list[count++] = (int)*(uint32_t*)tag_found;
    }

    // Class match second
    if (node->class != NULL) { 
        char* stored_class = String_Set_Get(&ss->class_string_set, node->class);
        if (stored_class != NULL) {
            void* class_found = HashmapGet(&ss->class_item_hashmap, &stored_class);
            if (class_found != NULL) {
                match_index_list[count++] = (int)*(uint32_t*)class_found;
            }
        }
    }

    // ID match last (highest priority)
    if (node->id != NULL) { 
        char* stored_id = String_Set_Get(&ss->id_string_set, node->id);
        if (stored_id != NULL) {
            void* id_found = HashmapGet(&ss->id_item_hashmap, &stored_id);
            if (id_found != NULL) {
                match_index_list[count++] = (int)*(uint32_t*)id_found;
            }
        }
    }
}

static void NU_Apply_Style_Item_To_Node(Node* node, NU_Stylesheet_Item* item)
{
    if (item->propertyFlags & (1ULL << 0) && !(node->inlineStyleFlags & (1ULL << 0))) node->layoutFlags = (node->layoutFlags & ~(1ULL << 0)) | (item->layoutFlags & (1ULL << 0)); // Layout direction
    if (item->propertyFlags & (1ULL << 1) && !(node->inlineStyleFlags & (1ULL << 1))) {
        node->layoutFlags = (node->layoutFlags & ~(1ULL << 1)) | (item->layoutFlags & (1ULL << 1)); // Grow horizontal
        node->layoutFlags = (node->layoutFlags & ~(1ULL << 2)) | (item->layoutFlags & (1ULL << 2)); // Grow vertical
    }
    if (item->propertyFlags & (1ULL << 2) && !(node->inlineStyleFlags & (1ULL << 2))) node->layoutFlags = (node->layoutFlags & ~(1ULL << 3)) | (item->layoutFlags & (1ULL << 3)); // Overflow vertical scroll (or not)
    if (item->propertyFlags & (1ULL << 3) && !(node->inlineStyleFlags & (1ULL << 3))) node->layoutFlags = (node->layoutFlags & ~(1ULL << 4)) | (item->layoutFlags & (1ULL << 4)); // Overflow horizontal scroll (or not)
    if (item->propertyFlags & (1ULL << 4) && !(node->inlineStyleFlags & (1ULL << 4))) node->layoutFlags = (node->layoutFlags & ~(1ULL << 5)) | (item->layoutFlags & (1ULL << 5)); // Absolute positioning (or not)
    if (item->propertyFlags & (1ULL << 5) && !(node->inlineStyleFlags & (1ULL << 5))) node->layoutFlags = (node->layoutFlags & ~(1ULL << 6)) | (item->layoutFlags & (1ULL << 6)); // Hidden or not
    if (item->propertyFlags & (1ULL << 6) && !(node->inlineStyleFlags & (1ULL << 6))) node->gap = item->gap;
    if (item->propertyFlags & (1ULL << 7) && !(node->inlineStyleFlags & (1ULL << 7))) node->preferred_width = item->preferred_width;
    if (item->propertyFlags & (1ULL << 8) && !(node->inlineStyleFlags & (1ULL << 8))) node->minWidth = item->minWidth;
    if (item->propertyFlags & (1ULL << 9) && !(node->inlineStyleFlags & (1ULL << 9))) node->maxWidth = item->maxWidth;
    if (item->propertyFlags & (1ULL << 10) && !(node->inlineStyleFlags & (1ULL << 10))) node->preferred_height = item->preferred_height;
    if (item->propertyFlags & (1ULL << 11) && !(node->inlineStyleFlags & (1ULL << 11))) node->minHeight = item->minHeight;
    if (item->propertyFlags & (1ULL << 12) && !(node->inlineStyleFlags & (1ULL << 12))) node->maxHeight = item->maxHeight;
    if (item->propertyFlags & (1ULL << 13) && !(node->inlineStyleFlags & (1ULL << 13))) node->horizontalAlignment = item->horizontalAlignment;
    if (item->propertyFlags & (1ULL << 14) && !(node->inlineStyleFlags & (1ULL << 14))) node->verticalAlignment = item->verticalAlignment;
    if (item->propertyFlags & (1ULL << 15) && !(node->inlineStyleFlags & (1ULL << 15))) node->horizontalTextAlignment = item->horizontalTextAlignment;
    if (item->propertyFlags & (1ULL << 16) && !(node->inlineStyleFlags & (1ULL << 16))) node->verticalTextAlignment = item->verticalTextAlignment;
    if (item->propertyFlags & (1ULL << 17) && !(node->inlineStyleFlags & (1ULL << 17))) node->left = item->left;
    if (item->propertyFlags & (1ULL << 18) && !(node->inlineStyleFlags & (1ULL << 18))) node->right = item->right;
    if (item->propertyFlags & (1ULL << 19) && !(node->inlineStyleFlags & (1ULL << 19))) node->top = item->top;
    if (item->propertyFlags & (1ULL << 20) && !(node->inlineStyleFlags & (1ULL << 20))) node->bottom = item->bottom;
    if (item->propertyFlags & (1ULL << 21) && !(node->inlineStyleFlags & (1ULL << 21))) {
        node->backgroundR = item->backgroundR;
        node->backgroundG = item->backgroundG;
        node->backgroundB = item->backgroundB;
    }
    if (item->propertyFlags & (1ULL << 22) && !(node->inlineStyleFlags & (1ULL << 22))) node->hideBackground = item->hideBackground;
    if (item->propertyFlags & (1ULL << 23) && !(node->inlineStyleFlags & (1ULL << 23))) {
        node->borderR = item->borderR;
        node->borderG = item->borderG;
        node->borderB = item->borderB;
    }
    if (item->propertyFlags & (1ULL << 24) && !(node->inlineStyleFlags & (1ULL << 24))) {
        node->textR = item->textR;
        node->textG = item->textG;
        node->textB = item->textB;
    }
    if (item->propertyFlags & (1ULL << 25) && !(node->inlineStyleFlags & (1ULL << 25))) node->borderTop = item->borderTop;
    if (item->propertyFlags & (1ULL << 26) && !(node->inlineStyleFlags & (1ULL << 26))) node->borderBottom = item->borderBottom;
    if (item->propertyFlags & (1ULL << 27) && !(node->inlineStyleFlags & (1ULL << 27))) node->borderLeft = item->borderLeft;
    if (item->propertyFlags & (1ULL << 28) && !(node->inlineStyleFlags & (1ULL << 28))) node->borderRight = item->borderRight;
    if (item->propertyFlags & (1ULL << 29) && !(node->inlineStyleFlags & (1ULL << 29))) node->borderRadiusTl = item->borderRadiusTl;
    if (item->propertyFlags & (1ULL << 30) && !(node->inlineStyleFlags & (1ULL << 30))) node->borderRadiusTr = item->borderRadiusTr;
    if (item->propertyFlags & (1ULL << 31) && !(node->inlineStyleFlags & (1ULL << 31))) node->borderRadiusBl = item->borderRadiusBl;
    if (item->propertyFlags & (1ULL << 32) && !(node->inlineStyleFlags & (1ULL << 32))) node->borderRadiusBr = item->borderRadiusBr;
    if (item->propertyFlags & (1ULL << 33) && !(node->inlineStyleFlags & (1ULL << 33))) node->padTop = item->padTop;
    if (item->propertyFlags & (1ULL << 34) && !(node->inlineStyleFlags & (1ULL << 34))) node->padBottom = item->padBottom;
    if (item->propertyFlags & (1ULL << 35) && !(node->inlineStyleFlags & (1ULL << 35))) node->padLeft = item->padLeft;
    if (item->propertyFlags & (1ULL << 36) && !(node->inlineStyleFlags & (1ULL << 36))) node->padRight = item->padRight;
    if (item->propertyFlags & (1ULL << 37) && !(node->inlineStyleFlags & (1ULL << 37))) node->glImageHandle = item->glImageHandle;
    node->fontId = item->fontId; // set font 
}

void NU_Apply_Stylesheet_To_Node(Node* node, NU_Stylesheet* ss)
{
    int match_index_list[3] = {-1, -1, -1};
    NU_Stylesheet_Find_Match(node, ss, &match_index_list[0]);

    int i = 0;
    while (match_index_list[i] != -1) {
        NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, (uint32_t)match_index_list[i]);
        i += 1;

        // --- Apply style ---
        NU_Apply_Style_Item_To_Node(node, item);
    }
}

void NU_Apply_Tag_Style_To_Node(Node* node, NU_Stylesheet* ss)
{
    // Tag match first (lowest priority)
    void* tag_found = HashmapGet(&ss->tag_item_hashmap, &node->tag); 
    if (tag_found != NULL) { 
        uint32_t index = *(uint32_t*)tag_found;
        NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, index);
        NU_Apply_Style_Item_To_Node(node, item);
    }
}

void NU_Apply_Pseudo_Style_To_Node(Node* node, NU_Stylesheet* ss, enum NU_Pseudo_Class pseudo)
{
    // Tag pseudo style match and apply
    NU_Stylesheet_Tag_Pseudo_Pair key = { node->tag, pseudo };
    void* tag_pseudo_found = HashmapGet(&ss->tag_pseudo_item_hashmap, &key);
    if (tag_pseudo_found != NULL) {
        uint32_t index = *(uint32_t*)tag_pseudo_found;
        NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, index);
        NU_Apply_Style_Item_To_Node(node, item);
    }

    // Class pseudo style match and apply
    if (node->class != NULL) {
        char* stored_class = String_Set_Get(&ss->class_string_set, node->class);
        if (stored_class != NULL) {
            NU_Stylesheet_String_Pseudo_Pair key = { stored_class, pseudo };
            void* class_pseudo_found = HashmapGet(&ss->class_pseudo_item_hashmap, &key);
            if (class_pseudo_found != NULL) {
                uint32_t index = *(uint32_t*)class_pseudo_found;
                NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, index);
                NU_Apply_Style_Item_To_Node(node, item);
            }
        }
    }

    // Id pseudo style match and apply
    if (node->id != NULL) {
        char* stored_id = String_Set_Get(&ss->id_string_set, node->id);
        if (stored_id != NULL) {
            NU_Stylesheet_String_Pseudo_Pair key = { stored_id, pseudo };
            void* id_pseudo_found = HashmapGet(&ss->id_pseudo_item_hashmap, &key);
            if (id_pseudo_found != NULL) {
                uint32_t index = *(uint32_t*)id_pseudo_found;
                NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, index);
                NU_Apply_Style_Item_To_Node(node, item);
            }
        }
    }
}

int NU_Internal_Apply_Stylesheet(uint32_t stylesheet_handle)
{
    // Passing in a corrupted stylesheet
    if (stylesheet_handle == -1) return 0;

    NU_Stylesheet* stylesheet = Vector_Get(&__NGUI.stylesheets, stylesheet_handle - 1);   

    Node* root_window = NU_Tree_Get(&__NGUI.tree, 0, 0);
    NU_Apply_Stylesheet_To_Node(root_window, stylesheet);

    // For each layer
    for (uint16_t l=0; l<=__NGUI.deepest_layer; l++)
    {
        NU_Layer* layer = &__NGUI.tree.layers[l];
        for (uint32_t i=0; i<layer->size; i++)
        {
            Node* node = NU_Layer_Get(layer, i); if (!node->nodeState) continue;
            NU_Apply_Stylesheet_To_Node(node, stylesheet);
        }
    }

    __NGUI.stylesheet = stylesheet;
    return 1; // success
}
