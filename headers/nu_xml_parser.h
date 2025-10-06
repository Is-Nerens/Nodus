#pragma once
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "datastructures/vector.h"
#include "datastructures/string_set.h"
#include "datastructures/hashmap.h"
#include "nu_convert.h"
#include "nu_image.h"
#include "nu_font.h"
#include "nu_draw.h"
#include "nu_stylesheet.h"
#include "nodus.h"
#include "nu_window.h"

#define NU_XML_PROPERTY_COUNT 36
#define NU_XML_KEYWORD_COUNT 45
static const char* nu_xml_keywords[] = {
    "id", "class", "dir", "grow", "overflowV", "overflowH", "gap",
    "width", "minWidth", "maxWidth", "height", "minHeight", "maxHeight", 
    "alignH", "alignV", "textAlignH", "textAlignV",
    "background", "borderColour", "textColour",
    "border", "borderTop", "borderBottom", "borderLeft", "borderRight",
    "borderRadius", "borderRadiusTopLeft", "borderRadiusTopRight", "borderRadiusBottomLeft", "borderRadiusBottomRight",
    "pad", "padTop", "padBottom", "padLeft", "padRight",
    "imageSrc",
    "window",
    "rect",
    "button",
    "grid",
    "text",
    "image",
    "table",
    "thead",
    "row",
};
static const uint8_t keyword_lengths[] = { 
    2, 5, 3, 4, 9, 9, 3,
    5, 8, 8, 6, 9, 9,          // width height
    6, 6, 10, 10,              // alignment
    10, 12, 10,                // background, border, text colour
    6, 9, 12, 10, 11,          // border width
    12, 19, 20, 22, 23,        // border radius
    3, 6, 9, 7, 8,             // padding
    8,                         // image src
    6, 4, 6, 4, 4, 5, 5, 5, 3  // tags
};
enum NU_XML_Token
{
    // --- Property Tokens ---
    ID_PROPERTY, CLASS_PROPERTY, 
    LAYOUT_DIRECTION_PROPERTY, GROW_PROPERTY,
    OVERFLOW_V_PROPERTY, OVERFLOW_H_PROPERTY,
    GAP_PROPERTY,
    WIDTH_PROPERTY, MIN_WIDTH_PROPERTY, MAX_WIDTH_PROPERTY,
    HEIGHT_PROPERTY, MIN_HEIGHT_PROPERTY, MAX_HEIGHT_PROPERTY,
    ALIGN_H_PROPERTY, ALIGN_V_PROPERTY, TEXT_ALIGN_H_PROPERTY, TEXT_ALIGN_V_PROPERTY,
    BACKGROUND_COLOUR_PROPERTY, BORDER_COLOUR_PROPERTY, TEXT_COLOUR_PROPERTY,
    BORDER_WIDTH_PROPERTY, BORDER_TOP_WIDTH_PROPERTY, BORDER_BOTTOM_WIDTH_PROPERTY, BORDER_LEFT_WIDTH_PROPERTY, BORDER_RIGHT_WIDTH_PROPERTY,
    BORDER_RADIUS_PROPERTY, BORDER_TOP_LEFT_RADIUS_PROPERTY, BORDER_TOP_RIGHT_RADIUS_PROPERTY, BORDER_BOTTOM_LEFT_RADIUS_PROPERTY, BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY,
    PADDING_PROPERTY, PADDING_TOP_PROPERTY, PADDING_BOTTOM_PROPERTY, PADDING_LEFT_PROPERTY, PADDING_RIGHT_PROPERTY,
    IMAGE_SOURCE_PROPERTY,

    // --- Tag Tokens ---
    WINDOW_TAG,
    RECT_TAG,
    BUTTON_TAG,
    GRID_TAG,
    TEXT_TAG,
    IMAGE_TAG,
    TABLE_TAG,
    TABLE_HEAD_TAG,
    ROW_TAG,
    OPEN_TAG,
    CLOSE_TAG,
    OPEN_END_TAG,
    CLOSE_END_TAG,
    PROPERTY_ASSIGNMENT,
    PROPERTY_VALUE,
    TEXT_CONTENT,
    UNDEFINED
};




// ------------------------------ 
// Structs ---------------------- 
// ------------------------------ 
struct Text_Ref
{
    uint32_t NU_Token_index;
    uint32_t src_index;
    uint8_t char_count;
};


// ------------------------------
// --- Internal Functions -------
// ------------------------------
static enum NU_XML_Token NU_Word_To_Token(char word[], uint8_t word_char_count)
{
    for (uint8_t i=0; i<NU_XML_KEYWORD_COUNT; i++) {
        size_t len = keyword_lengths[i];
        if (len == word_char_count && memcmp(word, nu_xml_keywords[i], len) == 0) {
            return i;
        }
    }
    return UNDEFINED;
}

static enum Tag NU_Token_To_Tag(enum NU_XML_Token NU_XML_Token)
{
    int tag_candidate = NU_XML_Token - NU_XML_PROPERTY_COUNT;
    if (tag_candidate < 0) return NAT;
    return NU_XML_Token - NU_XML_PROPERTY_COUNT;
}

static void NU_Tokenise(char* src_buffer, uint32_t src_length, struct Vector* NU_Token_vector, struct Vector* text_ref_vector) 
{
    // Store current NU_XML_Token word
    uint8_t word_char_index = 0;
    char word[32];

    // Store global text char indexes
    uint32_t text_char_count = 0; // Number of chars in text

    // Context
    uint8_t ctx = 0; // 0 == globalspace, 1 == commentspace, 2 == tagspace, 3 == propertyspace

    // Iterate over src file 
    uint32_t i = 0;
    while (i < src_length)
    {
        char c = src_buffer[i];

        // Comment begins
        if (ctx == 0 && i < src_length - 3 && c == '<' && src_buffer[i+1] == '!' && src_buffer[i+2] == '-' && src_buffer[i+3] == '-')
        {
            ctx = 1;
            i+=4;
            continue;
        }

        // Comment ends
        if (ctx == 1 && i < src_length - 2 && c == '-' && src_buffer[i+1] == '-' && src_buffer[i+2] == '>')
        {
            ctx = 0;
            i+=3;
            continue;
        }

        // In comment -> skip rest
        if (ctx == 1)
        {
            i+=1;
            continue;
        }

        // Open end tag
        if (ctx == 0 && i < src_length - 1 && c == '<' && src_buffer[i+1] == '/')
        {
            // Reset global text character count
            if (text_char_count > 0)
            {
                // Add text content token
                enum NU_XML_Token t = TEXT_CONTENT;
                Vector_Push(NU_Token_vector, &t);

                // Add text reference
                struct Text_Ref ref;
                ref.NU_Token_index = NU_Token_vector->size - 1;
                ref.src_index = i - text_char_count;
                ref.char_count = text_char_count;
                Vector_Push(text_ref_vector, &ref);
            }
            text_char_count = 0;

            enum NU_XML_Token t = OPEN_END_TAG;
            Vector_Push(NU_Token_vector, &t);
            word_char_index = 0;
            ctx = 2;
            i+=2;
            continue;
        }

        // Tag begins
        if (ctx == 0 && c == '<')
        {
            // Reset global text character count
            if (text_char_count > 0)
            {
                // Add text content token
                enum NU_XML_Token t = TEXT_CONTENT;
                Vector_Push(NU_Token_vector, &t);

                // Add text reference
                struct Text_Ref ref;
                ref.NU_Token_index = NU_Token_vector->size - 1;
                ref.src_index = i - text_char_count;
                ref.char_count = text_char_count;
                Vector_Push(text_ref_vector, &ref);
            }
            text_char_count = 0;

            enum NU_XML_Token t = OPEN_TAG;
            Vector_Push(NU_Token_vector, &t);
            word_char_index = 0;
            ctx = 2;
            i+=1;
            continue;
        }

        // Self closing end tag
        if (ctx == 2 && i < src_length - 1 && c == '/' && src_buffer[i+1] == '>')
        {
            if (word_char_index > 0) {
                enum NU_XML_Token t = NU_Word_To_Token(word, word_char_index);
                Vector_Push(NU_Token_vector, &t);
            }
            enum NU_XML_Token t = CLOSE_END_TAG;
            Vector_Push(NU_Token_vector, &t);
            word_char_index = 0;
            ctx = 0;
            i+=2;
            continue;
        }

        // Tag ends
        if (ctx == 2 && c == '>')
        {
            if (word_char_index > 0) {
                enum NU_XML_Token t = NU_Word_To_Token(word, word_char_index);
                Vector_Push(NU_Token_vector, &t);
            }
            enum NU_XML_Token t = CLOSE_TAG;
            Vector_Push(NU_Token_vector, &t);
            word_char_index = 0;
            ctx = 0;
            i+=1;
            continue;
        }

        // Property assignment
        if (ctx == 2 && c == '=')
        {
            if (word_char_index > 0) {
                enum NU_XML_Token t = NU_Word_To_Token(word, word_char_index);
                Vector_Push(NU_Token_vector, &t);
            }
            word_char_index = 0;
            enum NU_XML_Token t = PROPERTY_ASSIGNMENT;
            Vector_Push(NU_Token_vector, &t);
            i+=1;
            continue;
        }

        // Property value begins
        if (ctx == 2 && c == '"')
        {
            ctx = 3;
            i+=1;
            continue;
        }

        // Property value ends
        if (ctx == 3 && c == '"')
        {
            enum NU_XML_Token t = PROPERTY_VALUE;
            Vector_Push(NU_Token_vector, &t);
            if (word_char_index > 0)
            {
                struct Text_Ref ref;
                ref.NU_Token_index = NU_Token_vector->size - 1;
                ref.src_index = i - word_char_index;
                ref.char_count = word_char_index;
                Vector_Push(text_ref_vector, &ref);
            }
            word_char_index = 0;
            ctx = 2;
            i+=1;
            continue;
        }

        // If global space and NOT split character
        if (ctx == 0 && c != '\t' && c != '\n')
        {
            // text continues
            if (text_char_count > 0)
            {
                text_char_count += 1;
            }

            // text starts
            if (text_char_count == 0 && c != ' ')
            {
                text_char_count = 1;
            }

            i+=1;
            continue;
        }

        // If split character
        if (c == ' ' || c == '\t' || c == '\n')
        {
            if (word_char_index > 0) {
                enum NU_XML_Token t = NU_Word_To_Token(word, word_char_index);
                Vector_Push(NU_Token_vector, &t);
            }
            word_char_index = 0;
            i+=1;
            continue;
        }

        // If tag space or property space
        if (ctx == 2 || ctx == 3)
        {
            word[word_char_index] = c;
            word_char_index+=1;
        }

        i+=1;
    }

    if (word_char_index > 0) {
        enum NU_XML_Token t = NU_Word_To_Token(word, word_char_index);
        Vector_Push(NU_Token_vector, &t);
    }
}

static int NU_Is_Token_Property(enum NU_XML_Token NU_XML_Token)
{
    return NU_XML_Token < NU_XML_PROPERTY_COUNT;
}

static int AssertRootGrammar(struct Vector* token_vector)
{
    // ENFORCE RULE: FIRST TOKEN MUST BE OPEN TAG
    // ENFORCE RULE: SECOND TOKEN MUST BE TAG NAME
    // ENFORCE RULE: THIRD TOKEN MUST BE CLOSE_TAG | PROPERTY
    int root_open = token_vector->size > 2 && 
        *((enum NU_XML_Token*) Vector_Get(token_vector, 0)) == OPEN_TAG &&
        *((enum NU_XML_Token*) Vector_Get(token_vector, 1)) == WINDOW_TAG;

    if (token_vector->size > 2 &&
        *((enum NU_XML_Token*) Vector_Get(token_vector, 0)) == OPEN_TAG &&
        *((enum NU_XML_Token*) Vector_Get(token_vector, 1)) == WINDOW_TAG) 
    {
        if (token_vector->size > 5 && 
            *((enum NU_XML_Token*) Vector_Get(token_vector, token_vector->size-3)) == OPEN_END_TAG &&
            *((enum NU_XML_Token*) Vector_Get(token_vector, token_vector->size-2)) == WINDOW_TAG && 
            *((enum NU_XML_Token*) Vector_Get(token_vector, token_vector->size-1)) == CLOSE_TAG)
        {
            return 0; // Success
        }
        else // Closing tag is not window
        {
            printf("%s\n", "[Generate_Tree] Error! XML tree root not closed.");
            return -1;
        }
    }
    else // Root is not a window tag
    {
        printf("%s\n", "[Generate_Tree] Error! XML tree has no root. XML documents must begin with a <window> tag.");
        return -1;
    }
}

static int AssertNewTagGrammar(struct Vector* token_vector, int i)
{
    // ENFORCE RULE: NEXT TOKEN SHOULD BE TAG NAME 
    // ENFORCE RULE: THIRD TOKEN MUST BE CLOSE CLOSE_END OR PROPERTY
    if (i < token_vector->size - 2 && NU_Token_To_Tag(*((enum NU_XML_Token*) Vector_Get(token_vector, i+1))) != NAT)
    {
        enum NU_XML_Token third_token = *((enum NU_XML_Token*) Vector_Get(token_vector, i+2));
        if (third_token == CLOSE_TAG || third_token == CLOSE_END_TAG || NU_Is_Token_Property(third_token)) return 0; // Success
    }
    return -1; // Failure
}

static int AssertPropertyGrammar(struct Vector* token_vector, int i)
{
    // ENFORCE RULE: NEXT TOKEN SHOULD BE PROPERTY ASSIGN
    // ENFORCE RULE: THIRD TOKEN SHOULD BE PROPERTY TEXT
    if (i < token_vector->size - 2 && *((enum NU_XML_Token*) Vector_Get(token_vector, i+1)) == PROPERTY_ASSIGNMENT)
    {
        if (*((enum NU_XML_Token*) Vector_Get(token_vector, i+2)) == PROPERTY_VALUE) return 0; // Success
        printf("%s\n", "[Generate_Tree] Error! Expected property value after assignment.");
        return -1; // Failure
    }
    printf("%s\n", "[Generate_Tree] Error! Expected '=' after property.");
    return -1; // Failure
}

static int AssertTagCloseStartGrammar(struct Vector* token_vector, int i, enum Tag openTag)
{
    // ENFORCE RULE: NEXT TOKEN SHOULD BE TAG AND MUST MATCH OPENING TAG
    // ENDORCE RULE: THIRD TOKEN MUST BE A TAG END
    if (i < token_vector->size - 2 && 
        NU_Token_To_Tag(*((enum NU_XML_Token*) Vector_Get(token_vector, i+1))) == openTag && 
        *((enum NU_XML_Token*) Vector_Get(token_vector, i+2)) == CLOSE_TAG) return 0; // Success
    else {
        printf("%s", "[Generate Tree] Error! Closing tag does not match.");
        printf("%s %d %s %d", "close tag:", NU_Token_To_Tag(*((enum NU_XML_Token*) Vector_Get(token_vector, i+1))), "open tag:", openTag);
    }
    return -1; // Failure
}

static void NU_Apply_Node_Defaults(struct Node* node)
{
    node->window = NULL;
    node->class = NULL;
    node->id = NULL;
    node->text_content = NULL;
    node->inline_style_flags = 0;
    node->tag = NAT;
    node->gl_image_handle = 0;
    node->preferred_width = 0.0f;
    node->preferred_height = 0.0f;
    node->min_width = 0.0f;
    node->max_width = 10e20f;
    node->min_height = 0.0f;
    node->max_height = 10e20f;
    node->gap = 0.0f;
    node->content_width = 0.0f;
    node->content_height = 0.0f;
    node->index = UINT16_MAX;
    node->parent_index = UINT16_MAX;
    node->first_child_index = UINT16_MAX;
    node->child_capacity = 0;
    node->child_count = 0;
    node->node_present = 1;
    node->pad_top = 0;
    node->pad_bottom = 0;
    node->pad_left = 0;
    node->pad_right = 0;
    node->border_top = 0;
    node->border_bottom = 0;
    node->border_left = 0;
    node->border_right = 0;
    node->border_radius_tl = 0;
    node->border_radius_tr = 0;
    node->border_radius_bl = 0;
    node->border_radius_br = 0;
    node->background_r = 2;
    node->background_g = 2;
    node->background_b = 2;
    node->border_r = 156;
    node->border_g = 100;
    node->border_b = 5;
    node->text_r = 255;
    node->text_g = 255;
    node->text_b = 255;
    node->layout_flags = 0;
    node->horizontal_alignment = 0;
    node->vertical_alignment = 0;
    node->horizontal_text_alignment = 0;
    node->vertical_text_alignment = 1;
    node->hide_background = false;
    node->event_flags = 0;
}

static int NU_Generate_Tree(char* src_buffer, uint32_t src_length, struct Vector* NU_Token_vector, struct Vector* text_ref_vector)
{
    // Enforce root grammar
    if (AssertRootGrammar(NU_Token_vector) != 0) {
        return -1; // Failure 
    }

    // -----------------------
    // Create root window node
    // -----------------------
    struct Node root_node;
    NU_Apply_Node_Defaults(&root_node); // Default styles 
    root_node.tag = WINDOW;
    root_node.window = *(SDL_Window**) Vector_Get(&__nu_global_gui.windows, 0);
    struct Node* root_window_node = NU_Tree_Append(&__nu_global_gui.tree, &root_node, 0);
    Vector_Push(&__nu_global_gui.window_nodes, &root_window_node);

    // ---------------------------------
    // Get first property text reference
    // ---------------------------------
    struct Text_Ref* current_text_ref;
    if (text_ref_vector->size > 0) current_text_ref = Vector_Get(text_ref_vector, 0);
    uint32_t text_content_ref_index = 0;
    uint32_t text_ref_index = 0;

    // --------------------------
    // (string -> int) ----------
    // --------------------------
    String_Map image_filepath_to_handle_hmap;
    String_Map_Init(&image_filepath_to_handle_hmap, sizeof(GLuint), 512, 20);

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
    while (i < NU_Token_vector->size - 3)
    {
        const enum NU_XML_Token NU_XML_Token = *((enum NU_XML_Token*) Vector_Get(NU_Token_vector, i));

        // -------------------------
        // New tag -> Add a new node
        // -------------------------
        if (NU_XML_Token == OPEN_TAG)
        {
            if (AssertNewTagGrammar(NU_Token_vector, i) == 0)
            {
                // ----------------------
                // Enforce max tree depth
                // ----------------------
                if (current_layer+1 == MAX_TREE_DEPTH)
                {
                    printf("%s", "[Generate Tree] Error! Exceeded max tree depth of 32");
                    String_Map_Free(&image_filepath_to_handle_hmap);
                    return -1; // Failure
                }   
                

                // -----------------
                // Enforce tag rules
                // -----------------
                enum Tag tag = NU_Token_To_Tag(*((enum NU_XML_Token*) Vector_Get(NU_Token_vector, i+1)));

                if (ctx == 1 && tag != ROW && tag != THEAD) { 
                    printf("%s\n", "[Generate_Tree] Error! first child of <table> must be <row> or <thead>."); return -1; // Failure
                }
                else if (ctx == 2 && tag == THEAD) {
                    printf("%s\n", "[Generate_Tree] Error! <table> cannot have more than ONE <thead>."); return -1; // Failure
                }
                else if ((ctx == 2 || ctx == 3) && tag != ROW) {
                    printf("%s\n", "[Generate_Tree] Error! All children of <table> (except optional first child <thead>) must be <row>."); return -1; // Failure
                }
                else if (!(ctx == 1 || ctx == 2 || ctx == 3) && tag == ROW) {
                    printf("%s\n", "[Generate_Tree] Error! <row> must be the child of <table>."); return -1; // Failure
                } 
                else if (ctx != 1 && tag == THEAD) {
                    printf("%s\n", "[Generate_Tree] Error! <thead> can only be the FIRST child of <table>."); return -1; // Failure
                }

                // ----------------------------------------
                // Create a new node and add it to the tree
                // ----------------------------------------
                struct Node new_node;
                NU_Apply_Node_Defaults(&new_node); // Apply Default style
                new_node.tag = tag;
                new_node.parent_index = __nu_global_gui.tree.layers[current_layer].size - 1;
                current_node = NU_Tree_Append(&__nu_global_gui.tree, &new_node, current_layer+1);

                // ----------------------------------------
                // --- Handle scenarios for different tags
                // ----------------------------------------
                if (current_node->tag == WINDOW) // If node is a window -> create SDL window
                {
                    NU_Create_Subwindow(current_node);
                    Vector_Push(&__nu_global_gui.window_nodes, &current_node);
                }
                else if (current_node->tag == TABLE) {
                    current_node->inline_style_flags |= 1 << 0; // Enforce vertical direction
                    current_node->layout_flags |= LAYOUT_VERTICAL;
                    ctx = 1;
                } 
                else if (current_node->tag == THEAD) {
                    current_node->inline_style_flags |= 1 << 1; // Enforce horizontal growth
                    current_node->layout_flags |= GROW_HORIZONTAL;
                    ctx = 4;
                }
                else if (current_node->tag == ROW)
                {
                    current_node->inline_style_flags |= 1 << 1; // Enforce horizontal growth
                    current_node->layout_flags |= GROW_HORIZONTAL;
                    if (ctx == 1 || ctx == 3) ctx = 6;
                    else ctx = 5;
                }
                


                // -------------------------------
                // Add node to parent's child list
                // -------------------------------
                struct Node* parent_node = NU_Tree_Get(&__nu_global_gui.tree, current_layer, current_node->parent_index);
                if (current_node->tag != WINDOW) { // Inherit window from parent
                    current_node->window = parent_node->window;
                } 
                if (parent_node->child_count == 0) {
                    parent_node->first_child_index = current_node->index;
                }
                if (parent_node->tag == ROW || parent_node->tag == THEAD) {
                    current_node->inline_style_flags |= 1 << 1;
                    current_node->layout_flags |= GROW_VERTICAL;
                }
                parent_node->child_count += 1;
                parent_node->child_capacity += 1;

                current_layer++; // Move one layer deeper
                __nu_global_gui.deepest_layer = MAX(__nu_global_gui.deepest_layer, current_layer);
                i+=2; // Increment token index
                continue;
            }
            else
            {
                String_Map_Free(&image_filepath_to_handle_hmap);
                return -1;
            }
        }

        // -----------------------------------------------
        // Open end tag -> tag closes -> move up one layer
        // -----------------------------------------------
        if (NU_XML_Token == OPEN_END_TAG)
        {
         
            // Check close grammar
            struct Node* open_node = NU_Tree_Get(&__nu_global_gui.tree, current_layer, __nu_global_gui.tree.layers[current_layer].size - 1);
            enum Tag openTag = open_node->tag;
            if (AssertTagCloseStartGrammar(NU_Token_vector, i, openTag) != 0)
            {
                String_Map_Free(&image_filepath_to_handle_hmap);
                return -1; // Failure
            }

            // Multi node structure context switch
            if (open_node->tag == TABLE) { // <table> closes
                ctx = 0;
            } 
            else if (open_node->tag == THEAD) { // <thead> closes
                ctx = 2;
            }
            else if (open_node->tag == ROW) { // <row> closes
                if (ctx == 5) ctx = 2;
                else ctx = 3;
            } 

            current_layer--; // Move one layer up (towards root)
            i+=3; // Increment token index
            continue;
        }

        // -----------------------------------------------------
        // Close end tag -> tag self closes -> move up one layer
        // -----------------------------------------------------
        if (NU_XML_Token == CLOSE_END_TAG)
        {

            // Multi node structure context switch
            if (current_node->tag == TABLE) { // <table> closes
                ctx = 0;
            } 
            else if (current_node->tag == THEAD) { // <thead> closes
                ctx = 2;
            }
            else if (current_node->tag == ROW) { // <row> closes
                if (ctx == 5) ctx = 2;
                else ctx = 3;
            } 

            current_layer--; // Move one layer up (towards root)
            i+=1; // Increment token index
            continue;
        }

        // --------------------
        // Text content -------
        // --------------------
        if (NU_XML_Token == TEXT_CONTENT)
        {
            current_text_ref = Vector_Get(text_ref_vector, text_ref_index);
            text_ref_index += 1;
            char c = src_buffer[current_text_ref->src_index];
            char* text = &src_buffer[current_text_ref->src_index];
            src_buffer[current_text_ref->src_index + current_text_ref->char_count] = '\0';
            
            current_node->text_content = StringArena_Add(&__nu_global_gui.node_text_arena, text);

            i+=1; // Increment token index
            continue;
        }

        // ---------------------------------------
        // Property tag -> assign property to node
        // ---------------------------------------
        if (NU_Is_Token_Property(NU_XML_Token))
        {
            if (AssertPropertyGrammar(NU_Token_vector, i) == 0)
            {
                current_text_ref = Vector_Get(text_ref_vector, text_ref_index);
                text_ref_index += 1;
                char c = src_buffer[current_text_ref->src_index];
                char* ptext = &src_buffer[current_text_ref->src_index];
                src_buffer[current_text_ref->src_index + current_text_ref->char_count] = '\0';

                // Get the property value text
                switch (NU_XML_Token)
                {
                    // Set id
                    case ID_PROPERTY:
                        char* id_get = String_Set_Get(&__nu_global_gui.id_string_set, ptext);
                        if (id_get == NULL) {
                            current_node->id = String_Set_Add(&__nu_global_gui.id_string_set, ptext);
                            String_Map_Set(&__nu_global_gui.id_node_map, ptext, &current_node->handle);
                        }
                        break;

                    // Set class
                    case CLASS_PROPERTY:
                        current_node->class = String_Set_Add(&__nu_global_gui.class_string_set, ptext);
                        break;

                    // Set layout direction
                    case LAYOUT_DIRECTION_PROPERTY:
                        if (c == 'h') {
                            current_node->inline_style_flags |= 1 << 0;
                            current_node->layout_flags |= LAYOUT_HORIZONTAL;
                        }  
                        else {
                            current_node->inline_style_flags |= 1 << 0;
                            current_node->layout_flags |= LAYOUT_VERTICAL;
                        }
                        break;

                    // Set growth
                    case GROW_PROPERTY:
                        switch(c)
                        {
                            case 'v':
                                current_node->inline_style_flags |= 1 << 1;
                                current_node->layout_flags |= GROW_VERTICAL;
                                break;
                            case 'h':
                                current_node->inline_style_flags |= 1 << 1;
                                current_node->layout_flags |= GROW_HORIZONTAL;
                                break;
                            case 'b':
                                current_node->inline_style_flags |= 1 << 1;
                                current_node->layout_flags |= (GROW_HORIZONTAL | GROW_VERTICAL);
                                break;
                        }

                        break;
                    
                    // Set overflow behaviour
                    case OVERFLOW_V_PROPERTY:
                        if (c == 's') {
                            current_node->inline_style_flags |= 1 << 2;
                            current_node->layout_flags |= OVERFLOW_VERTICAL_SCROLL;
                        }
                        
                        break;
                    
                    case OVERFLOW_H_PROPERTY:
                        if (c == 's') {
                            current_node->inline_style_flags |= 1 << 3;
                            current_node->layout_flags |= OVERFLOW_HORIZONTAL_SCROLL;
                        }                
                        break;
                    
                    // Set gap 
                    case GAP_PROPERTY:
                        float property_float;
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inline_style_flags |= 1 << 4;
                            current_node->gap = property_float;
                        }
                        break;
                    
                    // Set preferred width
                    case WIDTH_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inline_style_flags |= 1 << 5;
                            current_node->preferred_width = property_float;
                        }
                        break;

                    // Set min width
                    case MIN_WIDTH_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inline_style_flags |= 1 << 6;
                            current_node->min_width = property_float;
                        }
                        break;

                    // Set max width
                    case MAX_WIDTH_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inline_style_flags |= 1 << 7;
                           current_node->max_width = property_float; 
                        }
                        break;

                    // Set preferred height
                    case HEIGHT_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inline_style_flags |= 1 << 8;
                            current_node->preferred_height = property_float;
                        }
                        break;

                    // Set min height
                    case MIN_HEIGHT_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inline_style_flags |= 1 << 9;
                            current_node->min_height = property_float;
                        }
                        break;

                    // Set max height
                    case MAX_HEIGHT_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inline_style_flags |= 1 << 10;
                            current_node->min_height = property_float;
                        }
                        break;

                    // Set horizontal alignment
                    case ALIGN_H_PROPERTY:
                        if (current_text_ref->char_count == 4 && memcmp(ptext, "left", 4) == 0) {
                            current_node->inline_style_flags |= 1 << 11;
                            current_node->horizontal_alignment = 0;
                        } else if (current_text_ref->char_count == 6 && memcmp(ptext, "center", 6) == 0) {
                            current_node->inline_style_flags |= 1 << 11;
                            current_node->horizontal_alignment = 1;
                        } else if (current_text_ref->char_count == 5 && memcmp(ptext, "right", 5) == 0) {
                            current_node->inline_style_flags |= 1 << 11;
                            current_node->horizontal_alignment = 2;
                        }
                        break;

                    // Set vertical alignment
                    case ALIGN_V_PROPERTY:
                        if (current_text_ref->char_count == 3 && memcmp(ptext, "top", 3) == 0) {
                            current_node->inline_style_flags |= 1 << 12;
                            current_node->vertical_alignment = 0;
                        } else if (current_text_ref->char_count == 6 && memcmp(ptext, "center", 6) == 0) {
                            current_node->inline_style_flags |= 1 << 12;
                            current_node->vertical_alignment = 2;
                        } else if (current_text_ref->char_count == 6 && memcmp(ptext, "bottom", 6) == 0) {
                            current_node->inline_style_flags |= 1 << 12;
                            current_node->vertical_alignment = 1;
                        }
                        break;

                    // Set horizontal text alignment
                    case TEXT_ALIGN_H_PROPERTY:
                        if (current_text_ref->char_count == 4 && memcmp(ptext, "left", 4) == 0) {
                            current_node->inline_style_flags |= 1 << 13;
                            current_node->horizontal_text_alignment = 0;
                        } else if (current_text_ref->char_count == 6 && memcmp(ptext, "center", 6) == 0) {
                            current_node->inline_style_flags |= 1 << 13;
                            current_node->horizontal_text_alignment = 2;
                        } else if (current_text_ref->char_count == 5 && memcmp(ptext, "right", 5) == 0) {
                            current_node->inline_style_flags |= 1 << 13;
                            current_node->horizontal_text_alignment = 1;
                        }
                        break;

                    // Set vertical text alignment
                    case TEXT_ALIGN_V_PROPERTY:
                        if (current_text_ref->char_count == 3 && memcmp(ptext, "top", 3) == 0) {
                            current_node->inline_style_flags |= 1 << 14;
                            current_node->vertical_text_alignment = 0;
                        } else if (current_text_ref->char_count == 6 && memcmp(ptext, "center", 6) == 0) {
                            current_node->inline_style_flags |= 1 << 14;
                            current_node->vertical_text_alignment = 2;
                        } else if (current_text_ref->char_count == 6 && memcmp(ptext, "bottom", 6) == 0) {
                            current_node->inline_style_flags |= 1 << 14;
                            current_node->vertical_text_alignment = 1;
                        }
                        break;

                    // Set background colour
                    case BACKGROUND_COLOUR_PROPERTY:
                        struct RGB rgb;
                        if (Parse_Hexcode(ptext, current_text_ref->char_count, &rgb)) {
                            current_node->inline_style_flags |= 1 << 15;
                            current_node->background_r = rgb.r;
                            current_node->background_g = rgb.g;
                            current_node->background_b = rgb.b;
                        } else if (current_text_ref->char_count == 4 && memcmp(ptext, "none", 4) == 0) {
                            current_node->inline_style_flags |= 1 << 16;
                            current_node->hide_background = true;
                        }
                        break;

                    // Set border colour
                    case BORDER_COLOUR_PROPERTY:
                        if (Parse_Hexcode(ptext, current_text_ref->char_count, &rgb)) {
                            current_node->inline_style_flags |= 1 << 17;
                            current_node->border_r = rgb.r;
                            current_node->border_g = rgb.g;
                            current_node->border_b = rgb.b;
                        }
                        break;

                    // Set text colour
                    case TEXT_COLOUR_PROPERTY:
                        if (Parse_Hexcode(ptext, current_text_ref->char_count, &rgb)) {
                            current_node->inline_style_flags |= 1 << 18;
                            current_node->text_r = rgb.r;
                            current_node->text_g = rgb.g;
                            current_node->text_b = rgb.b;
                        }
                        break;

                    // Set border width
                    case BORDER_WIDTH_PROPERTY:
                        uint8_t property_uint8;
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 19) | (1 << 20) | (1 << 21) | (1 << 22);
                            current_node->border_top = property_uint8;
                            current_node->border_bottom = property_uint8;
                            current_node->border_left = property_uint8;
                            current_node->border_right = property_uint8;
                        }
                        break;
                    case BORDER_TOP_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= 1 << 19;
                            current_node->border_top = property_uint8;
                        }
                        break;
                    case BORDER_BOTTOM_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= 1 << 20;
                            current_node->border_bottom = property_uint8;
                        }
                        break;
                    case BORDER_LEFT_WIDTH_PROPERTY:

                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= 1 << 21;
                            current_node->border_left = property_uint8;
                        }
                        break;
                    case BORDER_RIGHT_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= 1 << 22;
                            current_node->border_right = property_uint8;
                        }
                        break;

                    // Set border radii
                    case BORDER_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 23) | (1 << 24) | (1 << 25) | (1 << 26);
                            current_node->border_radius_tl = property_uint8;
                            current_node->border_radius_tr = property_uint8;
                            current_node->border_radius_bl = property_uint8;
                            current_node->border_radius_br = property_uint8;
                        }
                        break;
                    case BORDER_TOP_LEFT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 23);
                            current_node->border_radius_tl = property_uint8;
                        }
                        break;
                    case BORDER_TOP_RIGHT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 24);
                            current_node->border_radius_tr = property_uint8;
                        }
                        break;
                    case BORDER_BOTTOM_LEFT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 25);
                            current_node->border_radius_bl = property_uint8;
                        }
                        break;
                    case BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 26);
                            current_node->border_radius_br = property_uint8;
                        }
                        break;

                    // Set padding
                    case PADDING_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 27) | (1 << 28) | (1 << 29) | (1 << 30);
                            current_node->pad_top    = property_uint8;
                            current_node->pad_bottom = property_uint8;
                            current_node->pad_left   = property_uint8;
                            current_node->pad_right  = property_uint8;
                        }
                        break;
                    case PADDING_TOP_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 27);
                            current_node->pad_top = property_uint8;
                        }
                        break;
                    case PADDING_BOTTOM_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 28);
                            current_node->pad_bottom = property_uint8;
                        }
                        break;
                    case PADDING_LEFT_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 29);
                            current_node->pad_left = property_uint8;
                        }
                        break;
                    case PADDING_RIGHT_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 30);
                            current_node->pad_right = property_uint8;
                        }
                        break;

                    case IMAGE_SOURCE_PROPERTY:
                        void* found = String_Map_Get(&image_filepath_to_handle_hmap, ptext);
                        if (found == NULL) {
                            GLuint image_handle = Image_Load(ptext);
                            if (image_handle) {
                                current_node->gl_image_handle = image_handle;
                                String_Map_Set(&image_filepath_to_handle_hmap, ptext, &image_handle);
                            }
                        } 
                        else {
                            current_node->gl_image_handle = *(GLuint*)found;
                        }
                        break;

                    default:
                        break;
                }

                // Increment NU_XML_Token
                i+=3;
                continue;
            }
            else {
                String_Map_Free(&image_filepath_to_handle_hmap);
                return -1;
            }
        }
        i+=1; // Increment token index
    }
    String_Map_Free(&image_filepath_to_handle_hmap);
    return 0; // Success
}



// ---------------------------------- //
// --- Public Functions ------------- //
// ---------------------------------- //
int NU_From_XML(char* filepath)
{
    // Open XML source file and load into buffer
    FILE* f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "Cannot open file '%s': %s\n", filepath, strerror(errno));
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long file_size_long = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (file_size_long > UINT32_MAX) {
        printf("%s", "Src file is too large! It must be < 4 294 967 295 Bytes");
        return 0;
    }
    uint32_t src_length = (uint32_t)file_size_long;
    char* src_buffer = malloc(src_length + 1);
    src_length = fread(src_buffer, 1, file_size_long, f);
    src_buffer[src_length] = '\0'; 
    fclose(f);

    // Init Token vector and reserve ~1MB
    struct Vector NU_Token_vector;
    struct Vector text_ref_vector;

    // Init vectors
    Vector_Reserve(&NU_Token_vector, sizeof(enum NU_XML_Token), 25000); // reserve ~100KB
    Vector_Reserve(&text_ref_vector, sizeof(struct Text_Ref), 25000); // reserve ~225KB

    // Init UI tree 
    NU_Tree_Init(&__nu_global_gui.tree, 100, MAX_TREE_DEPTH);

    // Tokenise the file source
    NU_Tokenise(src_buffer, src_length, &NU_Token_vector, &text_ref_vector);

    // Generate UI tree
    timer_start();
    if (NU_Generate_Tree(src_buffer, src_length, &NU_Token_vector, &text_ref_vector) != 0) return 0; // Failure
    timer_stop();

    // Free token and property text reference memory
    Vector_Free(&NU_Token_vector);
    Vector_Free(&text_ref_vector);
    return 1; // Success
}
