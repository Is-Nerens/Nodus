#pragma once
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "nu_convert.h"
#include "image/nu_image.h"
#include "stylesheet/nu_stylesheet.h"
#include "nodus.h"
#include "nu_window.h"


#include "nu_xml_tokens.h"
#include "nu_xml_grammar_assertions.h"
#include "nu_xml_tokeniser.h"

int NU_Generate_Tree(char* src_buffer, uint32_t src_length, struct Vector* tokens, struct Vector* textRefs)
{
    // Enforce root grammar
    if (AssertRootGrammar(tokens) != 0) {
        return 0; // Failure 
    }

    // -----------------------
    // Create root window node
    // -----------------------
    struct Node root_node;
    NU_Apply_Node_Defaults(&root_node); // Default styles 
    root_node.tag = WINDOW;
    root_node.window = *(SDL_Window**) Vector_Get(&__NGUI.windows, 0);
    struct Node* root_window_node = NU_Tree_Append(&__NGUI.tree, &root_node, 0);
    Vector_Push(&__NGUI.windowNodes, &root_window_node->handle);

    // ---------------------------------
    // Get first property text reference
    // ---------------------------------
    struct Text_Ref* current_text_ref;
    if (textRefs->size > 0) current_text_ref = Vector_Get(textRefs, 0);
    uint32_t text_content_ref_index = 0;
    uint32_t text_ref_index = 0;

    // --------------------------
    // (string -> int) ----------
    // --------------------------
    NU_Stringmap image_filepath_to_handle_hmap;
    NU_Stringmap_Init(&image_filepath_to_handle_hmap, sizeof(GLuint), 512, 20);

    // ------------------------------------
    // Iterate over all NU_Tokens ---------
    // ------------------------------------
    int i = 2; 
    int ctx = 0; 
    // 0 = default
    // 1 = <table> just opened
    // 2 = in <table> with <thead>
    // 3 = in table without <thead>
    // 4 = in <thead>
    // 5 = in <row> in <table> with <thead>
    // 6 = in <row> in <table> without <thead>
    uint8_t current_layer = 0; 
    struct Node* current_node = root_window_node;
    while (i < tokens->size - 3)
    {
        const enum NU_XML_TOKEN NU_XML_TOKEN = *((enum NU_XML_TOKEN*) Vector_Get(tokens, i));
        
        // -------------------------
        // New tag -> Add a new node
        // -------------------------
        if (NU_XML_TOKEN == OPEN_TAG)
        {
            if (AssertNewTagGrammar(tokens, i))
            {
                // -----------------
                // Enforce tag rules
                // -----------------
                enum Tag tag = NU_Token_To_Tag(*((enum NU_XML_TOKEN*) Vector_Get(tokens, i+1)));

                if (ctx == 1 && tag != ROW && tag != THEAD) { 
                    printf("%s\n", "[Generate_Tree] Error! first child of <table> must be <row> or <thead>."); return 0; // Failure
                }
                else if (ctx == 2 && tag == THEAD) {
                    printf("%s\n", "[Generate_Tree] Error! <table> cannot have more than ONE <thead>."); return 0; // Failure
                }
                else if ((ctx == 2 || ctx == 3) && tag != ROW) {
                    printf("%s\n", "[Generate_Tree] Error! All children of <table> (except optional first child <thead>) must be <row>."); return 0; // Failure
                }
                else if (!(ctx == 1 || ctx == 2 || ctx == 3) && tag == ROW) {
                    printf("%s\n", "[Generate_Tree] Error! <row> must be the child of <table>."); return 0; // Failure
                } 
                else if (ctx != 1 && tag == THEAD) {
                    printf("%s\n", "[Generate_Tree] Error! <thead> can only be the FIRST child of <table>."); return 0; // Failure
                }

                // ----------------------------------------
                // Create a new node and add it to the tree
                // ----------------------------------------
                struct Node new_node;
                NU_Apply_Node_Defaults(&new_node); // Apply Default style
                new_node.tag = tag;
                new_node.parentIndex = __NGUI.tree.layers[current_layer].size - 1;
                current_node = NU_Tree_Append(&__NGUI.tree, &new_node, current_layer+1);

                // ----------------------------------------
                // --- Handle scenarios for different tags
                // ----------------------------------------
                if (current_node->tag == WINDOW) // If node is a window -> create SDL window
                {
                    NU_Create_Subwindow(current_node);
                    Vector_Push(&__NGUI.windowNodes, &current_node->handle);
                }
                else if (current_node->tag == TABLE) {
                    current_node->inlineStyleFlags |= 1ULL << 0; // Enforce vertical direction
                    current_node->layoutFlags |= LAYOUT_VERTICAL;
                    ctx = 1;
                } 
                else if (current_node->tag == THEAD) {
                    current_node->inlineStyleFlags |= 1ULL << 1; // Enforce horizontal growth
                    current_node->layoutFlags |= GROW_HORIZONTAL;
                    ctx = 4;
                }
                else if (current_node->tag == ROW)
                {
                    current_node->inlineStyleFlags |= 1ULL << 1; // Enforce horizontal growth
                    current_node->layoutFlags |= GROW_HORIZONTAL;
                    if (ctx == 1 || ctx == 3) ctx = 6;
                    else ctx = 5;
                }
                else if (current_node->tag == CANVAS)
                {
                    NU_Add_Canvas_Context(current_node->handle);
                }

                // -------------------------------
                // Add node to parent's child list
                // -------------------------------
                struct Node* parent_node = NU_Tree_Get(&__NGUI.tree, current_layer, current_node->parentIndex);
                if (current_node->tag != WINDOW) { // Inherit window from parent
                    current_node->window = parent_node->window;
                } 
                if (parent_node->childCount == 0) {
                    parent_node->firstChildIndex = current_node->index;
                }
                if (parent_node->tag == ROW || parent_node->tag == THEAD) {
                    current_node->inlineStyleFlags |= 1ULL << 1;
                    current_node->layoutFlags |= GROW_VERTICAL;
                }
                parent_node->childCount += 1;
                parent_node->childCapacity += 1;

                current_layer++; // Move one layer deeper
                __NGUI.deepest_layer = MAX(__NGUI.deepest_layer, current_layer);
                i+=2; // Increment token index
                continue;
            }
            else
            {
                NU_Stringmap_Free(&image_filepath_to_handle_hmap);
                printf("fail\n");
                return 0;
            }
        }

        // -----------------------------------------------
        // Open end tag -> tag closes -> move up one layer
        // -----------------------------------------------
        if (NU_XML_TOKEN == OPEN_END_TAG)
        {
         
            // Check close grammar
            struct Node* open_node = NU_Tree_Get(&__NGUI.tree, current_layer, __NGUI.tree.layers[current_layer].size - 1);
            enum Tag openTag = open_node->tag;
            if (AssertTagCloseStartGrammar(tokens, i, openTag) != 0) {
                NU_Stringmap_Free(&image_filepath_to_handle_hmap);
                return 0; // Failure
            }

            // Multi node structure context switch
            if (open_node->tag == TABLE) ctx = 0;      // <table> closes
            else if (open_node->tag == THEAD) ctx = 2; // <thead> closes
            else if (open_node->tag == ROW) {          // <row> closes
                if (ctx == 5) ctx = 2;
                else ctx = 3;
            } 

            current_layer--; // Move one layer up (towards root)
            i+=3;            // Increment token index
            continue;
        }

        // -----------------------------------------------------
        // Close end tag -> tag self closes -> move up one layer
        // -----------------------------------------------------
        if (NU_XML_TOKEN == CLOSE_END_TAG)
        {

            // Multi node structure context switch
            if (current_node->tag == TABLE) ctx = 0;      // <table> closes 
            else if (current_node->tag == THEAD) ctx = 2; // <thead> closes
            else if (current_node->tag == ROW) {          // <row> closes
                if (ctx == 5) ctx = 2;
                else ctx = 3;
            } 

            current_layer--; // Move one layer up (towards root)
            i+=1;            // Increment token index
            continue;
        }

        // --------------------
        // Text content -------
        // --------------------
        if (NU_XML_TOKEN == TEXT_CONTENT)
        {
            current_text_ref = Vector_Get(textRefs, text_ref_index);
            text_ref_index += 1;
            char c = src_buffer[current_text_ref->src_index];
            char* text = &src_buffer[current_text_ref->src_index];
            src_buffer[current_text_ref->src_index + current_text_ref->char_count] = '\0';
            current_node->textContent = StringArena_Add(&__NGUI.node_text_arena, text);
            i+=1; // Increment token index
            continue;
        }

        // ---------------------------------------
        // Property tag -> assign property to node
        // ---------------------------------------
        if (NU_Is_Token_Property(NU_XML_TOKEN))
        {
            if (AssertPropertyGrammar(tokens, i) == 0)
            {
                current_text_ref = Vector_Get(textRefs, text_ref_index);
                text_ref_index += 1;
                char c = src_buffer[current_text_ref->src_index];
                char* ptext = &src_buffer[current_text_ref->src_index];
                src_buffer[current_text_ref->src_index + current_text_ref->char_count] = '\0';

                // Get the property value text
                switch (NU_XML_TOKEN)
                {
                    // Set id
                    case ID_PROPERTY:
                        char* id_get = String_Set_Get(&__NGUI.id_string_set, ptext);
                        if (id_get == NULL) {
                            current_node->id = String_Set_Add(&__NGUI.id_string_set, ptext);
                            NU_Stringmap_Set(&__NGUI.id_node_map, ptext, &current_node->handle);
                        }
                        break;

                    // Set class
                    case CLASS_PROPERTY:
                        current_node->class = String_Set_Add(&__NGUI.class_string_set, ptext);
                        break;

                    // Set layout direction
                    case LAYOUT_DIRECTION_PROPERTY:
                        if (c == 'h') {
                            current_node->inlineStyleFlags |= 1ULL << 0;
                        }  
                        else {
                            current_node->inlineStyleFlags |= 1ULL << 0;
                            current_node->layoutFlags |= LAYOUT_VERTICAL;
                        }
                        break;

                    // Set growth
                    case GROW_PROPERTY:
                        switch(c)
                        {
                            case 'v':
                                current_node->inlineStyleFlags |= 1ULL << 1;
                                current_node->layoutFlags |= GROW_VERTICAL;
                                break;
                            case 'h':
                                current_node->inlineStyleFlags |= 1ULL << 1;
                                current_node->layoutFlags |= GROW_HORIZONTAL;
                                break;
                            case 'b':
                                current_node->inlineStyleFlags |= 1ULL << 1;
                                current_node->layoutFlags |= (GROW_HORIZONTAL | GROW_VERTICAL);
                                break;
                        }

                        break;
                    
                    // Set overflow behaviour
                    case OVERFLOW_V_PROPERTY:
                        if (c == 's') {
                            current_node->inlineStyleFlags |= 1ULL << 2;
                            current_node->layoutFlags |= OVERFLOW_VERTICAL_SCROLL;
                        }
                        
                        break;
                    
                    case OVERFLOW_H_PROPERTY:
                        if (c == 's') {
                            current_node->inlineStyleFlags |= 1ULL << 3;
                            current_node->layoutFlags |= OVERFLOW_HORIZONTAL_SCROLL;
                        }                
                        break;

                    // Relative/Absolute positiong
                    case POSITION_PROPERTY:
                        if (current_text_ref->char_count == 8 && memcmp(ptext, "absolute", 8) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 4;
                            current_node->layoutFlags |= POSITION_ABSOLUTE;
                        }
                        break;

                    // Show/hide
                    case HIDE_PROPERTY:
                        if (current_text_ref->char_count == 4 && memcmp(ptext, "true", 4) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 5;
                            current_node->layoutFlags |= HIDDEN;
                        }
                        break;
                    
                    // Set gap 
                    case GAP_PROPERTY:
                        float property_float;
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 6;
                            current_node->gap = property_float;
                        }
                        break;
                    
                    // Set preferred width
                    case WIDTH_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 7;
                            current_node->preferred_width = property_float;
                        }
                        break;

                    // Set min width
                    case MIN_WIDTH_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 8;
                            current_node->minWidth = property_float;
                        }
                        break;

                    // Set max width
                    case MAX_WIDTH_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 9;
                           current_node->maxWidth = property_float; 
                        }
                        break;

                    // Set preferred height
                    case HEIGHT_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 10;
                            current_node->preferred_height = property_float;
                        }
                        break;

                    // Set min height
                    case MIN_HEIGHT_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 11;
                            current_node->minHeight = property_float;
                        }
                        break;

                    // Set max height
                    case MAX_HEIGHT_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 12;
                            current_node->minHeight = property_float;
                        }
                        break;

                    // Set horizontal alignment
                    case ALIGN_H_PROPERTY:
                        if (current_text_ref->char_count == 4 && memcmp(ptext, "left", 4) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 13;
                            current_node->horizontalAlignment = 0;
                        } else if (current_text_ref->char_count == 6 && memcmp(ptext, "center", 6) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 13;
                            current_node->horizontalAlignment = 1;
                        } else if (current_text_ref->char_count == 5 && memcmp(ptext, "right", 5) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 13;
                            current_node->horizontalAlignment = 2;
                        }
                        break;

                    // Set vertical alignment
                    case ALIGN_V_PROPERTY:
                        if (current_text_ref->char_count == 3 && memcmp(ptext, "top", 3) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 14;
                            current_node->verticalAlignment = 0;
                        } else if (current_text_ref->char_count == 6 && memcmp(ptext, "center", 6) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 14;
                            current_node->verticalAlignment = 1;
                        } else if (current_text_ref->char_count == 6 && memcmp(ptext, "bottom", 6) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 14;
                            current_node->verticalAlignment = 2;
                        }
                        break;

                    // Set horizontal text alignment
                    case TEXT_ALIGN_H_PROPERTY:
                        if (current_text_ref->char_count == 4 && memcmp(ptext, "left", 4) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 15;
                            current_node->horizontalTextAlignment = 0;
                        } else if (current_text_ref->char_count == 6 && memcmp(ptext, "center", 6) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 15;
                            current_node->horizontalTextAlignment = 2;
                        } else if (current_text_ref->char_count == 5 && memcmp(ptext, "right", 5) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 15;
                            current_node->horizontalTextAlignment = 1;
                        }
                        break;

                    // Set vertical text alignment
                    case TEXT_ALIGN_V_PROPERTY:
                        if (current_text_ref->char_count == 3 && memcmp(ptext, "top", 3) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 16;
                            current_node->verticalTextAlignment = 0;
                        } else if (current_text_ref->char_count == 6 && memcmp(ptext, "center", 6) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 16;
                            current_node->verticalTextAlignment = 2;
                        } else if (current_text_ref->char_count == 6 && memcmp(ptext, "bottom", 6) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 16;
                            current_node->verticalTextAlignment = 1;
                        }
                        break;

                    // Set absolute positioning
                    case LEFT_PROPERTY:
                        float abs_position; 
                        if (String_To_Float(&abs_position, ptext)) {
                            current_node->left = abs_position;
                            current_node->inlineStyleFlags |= 1ULL << 17;
                        }
                        break;
                    
                    case RIGHT_PROPERTY:
                        if (String_To_Float(&abs_position, ptext)) {
                            current_node->right = abs_position;
                            current_node->inlineStyleFlags |= 1ULL << 18;
                        }
                        break;

                    case TOP_PROPERTY:
                        if (String_To_Float(&abs_position, ptext)) {
                            current_node->top = abs_position;
                            current_node->inlineStyleFlags |= 1ULL << 19;
                        }
                        break;
                    
                    case BOTTOM_PROPERTY:
                        if (String_To_Float(&abs_position, ptext)) {
                            current_node->bottom = abs_position;
                            current_node->inlineStyleFlags |= 1ULL << 20;
                        }
                        break;

                    // Set background colour
                    case BACKGROUND_COLOUR_PROPERTY:
                        struct RGB rgb;
                        if (Parse_Hexcode(ptext, current_text_ref->char_count, &rgb)) {
                            current_node->inlineStyleFlags |= 1ULL << 21;
                            current_node->backgroundR = rgb.r;
                            current_node->backgroundG = rgb.g;
                            current_node->backgroundB = rgb.b;
                        } else if (current_text_ref->char_count == 4 && memcmp(ptext, "none", 4) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 22;
                            current_node->hideBackground = true;
                        }
                        break;

                    // Set border colour
                    case BORDER_COLOUR_PROPERTY:
                        if (Parse_Hexcode(ptext, current_text_ref->char_count, &rgb)) {
                            current_node->inlineStyleFlags |= 1ULL << 23;
                            current_node->borderR = rgb.r;
                            current_node->borderG = rgb.g;
                            current_node->borderB = rgb.b;
                        }
                        break;

                    // Set text colour
                    case TEXT_COLOUR_PROPERTY:
                        if (Parse_Hexcode(ptext, current_text_ref->char_count, &rgb)) {
                            current_node->inlineStyleFlags |= 1ULL << 24;
                            current_node->textR = rgb.r;
                            current_node->textG = rgb.g;
                            current_node->textB = rgb.b;
                        }
                        break;

                    // Set border width
                    case BORDER_WIDTH_PROPERTY:
                        uint8_t property_uint8;
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= (1ULL << 25) | (1ULL << 26) | (1ULL << 27) | (1ULL << 28);
                            current_node->borderTop = property_uint8;
                            current_node->borderBottom = property_uint8;
                            current_node->borderLeft = property_uint8;
                            current_node->borderRight = property_uint8;
                        }
                        break;
                    case BORDER_TOP_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 25;
                            current_node->borderTop = property_uint8;
                        }
                        break;
                    case BORDER_BOTTOM_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 26;
                            current_node->borderBottom = property_uint8;
                        }
                        break;
                    case BORDER_LEFT_WIDTH_PROPERTY:

                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 27;
                            current_node->borderLeft = property_uint8;
                        }
                        break;
                    case BORDER_RIGHT_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 28;
                            current_node->borderRight = property_uint8;
                        }
                        break;

                    // Set border radii
                    case BORDER_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= (1ULL << 29) | (1ULL << 30) | (1ULL << 31) | ((uint64_t)1ULL << 32);
                            current_node->borderRadiusTl = property_uint8;
                            current_node->borderRadiusTr = property_uint8;
                            current_node->borderRadiusBl = property_uint8;
                            current_node->borderRadiusBr = property_uint8;
                        }
                        break;
                    case BORDER_TOP_LEFT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= (1ULL << 29);
                            current_node->borderRadiusTl = property_uint8;
                        }
                        break;
                    case BORDER_TOP_RIGHT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= (1ULL << 30);
                            current_node->borderRadiusTr = property_uint8;
                        }
                        break;
                    case BORDER_BOTTOM_LEFT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= (1ULL << 31);
                            current_node->borderRadiusBl = property_uint8;
                        }
                        break;
                    case BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= ((uint64_t)1ULL << 32);
                            current_node->borderRadiusBr = property_uint8;
                        }
                        break;

                    // Set padding
                    case PADDING_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= ((uint64_t)1ULL << 33) | ((uint64_t)1ULL << 34) | ((uint64_t)1ULL << 35) | ((uint64_t)1ULL << 36);
                            current_node->padTop    = property_uint8;
                            current_node->padBottom = property_uint8;
                            current_node->padLeft   = property_uint8;
                            current_node->padRight  = property_uint8;
                        }
                        break;
                    case PADDING_TOP_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= ((uint64_t)1ULL << 33);
                            current_node->padTop = property_uint8;
                        }
                        break;
                    case PADDING_BOTTOM_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= ((uint64_t)1ULL << 34);
                            current_node->padBottom = property_uint8;
                        }
                        break;
                    case PADDING_LEFT_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= ((uint64_t)1ULL << 35);
                            current_node->padLeft = property_uint8;
                        }
                        break;
                    case PADDING_RIGHT_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= ((uint64_t)1ULL << 36);
                            current_node->padRight = property_uint8;
                        }
                        break;

                    // Image source
                    case IMAGE_SOURCE_PROPERTY:
                        void* found = NU_Stringmap_Get(&image_filepath_to_handle_hmap, ptext);
                        if (found == NULL) {
                            GLuint image_handle = Image_Load(ptext);
                            if (image_handle) {
                                current_node->glImageHandle = image_handle;
                                NU_Stringmap_Set(&image_filepath_to_handle_hmap, ptext, &image_handle);
                            }
                        } 
                        else {
                            current_node->glImageHandle = *(GLuint*)found;
                        }
                        current_node->inlineStyleFlags |= ((uint64_t)1ULL << 37);
                        break;

                    default:
                        break;
                }

                // Increment NU_XML_TOKEN
                i+=3;
                continue;
            }
            else 
            {
                NU_Stringmap_Free(&image_filepath_to_handle_hmap);
                return 0;
            }
        }
        i+=1; // Increment token index
    }
    NU_Stringmap_Free(&image_filepath_to_handle_hmap);
    return 1; // Success
}

int NU_Internal_Load_XML(char* filepath)
{
    // Open XML source file and load into buffer
    FILE* f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "Cannot open file '%s': %s\n", filepath, strerror(errno)); return 0;
    }
    fseek(f, 0, SEEK_END);
    long file_size_long = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (file_size_long > UINT32_MAX) {
        printf("%s", "Src file is too large! It must be < 4 294 967 295 Bytes"); return 0;
    }
    uint32_t src_length = (uint32_t)file_size_long;
    char* src_buffer = malloc(src_length + 1);
    src_length = fread(src_buffer, 1, file_size_long, f);
    src_buffer[src_length] = '\0'; 
    fclose(f);

    // Init token and text ref vectors
    struct Vector tokens;
    struct Vector textRefs;
    Vector_Reserve(&tokens, sizeof(enum NU_XML_TOKEN), 25000); // reserve ~100KB
    Vector_Reserve(&textRefs, sizeof(struct Text_Ref), 25000); // reserve ~225KB

    // Init UI tree 
    NU_Tree_Init(&__NGUI.tree, 100, 10);

    // Tokenise and generate
    NU_Tokenise(src_buffer, src_length, &tokens, &textRefs); 
    if (!NU_Generate_Tree(src_buffer, src_length, &tokens, &textRefs)) return 0; 

    // Free token and property text reference memory
    Vector_Free(&tokens);
    Vector_Free(&textRefs);
    return 1; // Success
}