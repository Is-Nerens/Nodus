#pragma once
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX_TREE_DEPTH 32

// Layout flag bits
#define LAYOUT_HORIZONTAL            0x00        // 0b00000000
#define LAYOUT_VERTICAL              0x01        // 0b00000001
#define GROW_HORIZONTAL              0x02        // 0b00000010
#define GROW_VERTICAL                0x04        // 0b00000100
#define OVERFLOW_VERTICAL_SCROLL     0x08        // 0b00001000
#define OVERFLOW_HORIZONTAL_SCROLL   0x10        // 0b00010000

#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "performance.h"
#include "vector.h"

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>

#define PROPERTY_COUNT 31
#define KEYWORD_COUNT 44

static const char* keywords[] = {
    "id", "dir", "grow", "overflowV", "overflowH", "gap",
    "width", "minWidth", "maxWidth", "height", "minHeight", "maxHeight", 
    "alignH", "alignV",
    "background", "borderColour",
    "border", "borderTop", "borderBottom", "borderLeft", "borderRight",
    "borderRadius", "borderRadiusTopLeft", "borderRadiusTopRight", "borderRadiusBottomLeft", "borderRadiusBottomRight",
    "pad", "padTop", "padBottom", "padLeft", "padRight",
    "window",
    "rect",
    "button",
    "grid",
    "text",
    "image"
};
static const uint8_t keyword_lengths[] = { 
    2, 3, 4, 9, 9, 3,
    5, 8, 8, 6, 9, 9,      // width height
    6, 6,                  // alignment
    10, 12,                // background, border colour
    6, 9, 12, 10, 11,      // border width
    12, 19, 20, 22, 23,    // border radius
    3, 6, 9, 7, 11,        // padding
    6, 4, 6, 4, 4, 5       // selectors
};
enum NU_Token
{
    ID_PROPERTY,
    LAYOUT_DIRECTION_PROPERTY,
    GROW_PROPERTY,
    OVERFLOW_V_PROPERTY,
    OVERFLOW_H_PROPERTY,
    GAP_PROPERTY,
    WIDTH_PROPERTY,
    MIN_WIDTH_PROPERTY,
    MAX_WIDTH_PROPERTY,
    HEIGHT_PROPERTY,
    MIN_HEIGHT_PROPERTY,
    MAX_HEIGHT_PROPERTY,
    ALIGN_H_PROPERTY,
    ALIGN_V_PROPERTY,
    BACKGROUND_COLOUR_PROPERTY,
    BORDER_COLOUR_PROPERTY,
    BORDER_WIDTH_PROPERTY,
    BORDER_TOP_WIDTH_PROPERTY,
    BORDER_BOTTOM_WIDTH_PROPERTY,
    BORDER_LEFT_WIDTH_PROPERTY,
    BORDER_RIGHT_WIDTH_PROPERTY,
    BORDER_RADIUS_PROPERTY,
    BORDER_TOP_LEFT_RADIUS_PROPERTY,
    BORDER_TOP_RIGHT_RADIUS_PROPERTY,
    BORDER_BOTTOM_LEFT_RADIUS_PROPERTY,
    BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY,
    PADDING_PROPERTY,
    PADDING_TOP_PROPERTY,
    PADDING_BOTTOM_PROPERTY,
    PADDING_LEFT_PROPERTY,
    PADDING_RIGHT_PROPERTY,
    WINDOW_TAG,
    RECT_TAG,
    BUTTON_TAG,
    GRID_TAG,
    TEXT_TAG,
    IMAGE_TAG,
    OPEN_TAG,
    CLOSE_TAG,
    OPEN_END_TAG,
    CLOSE_END_TAG,
    PROPERTY_ASSIGNMENT,
    PROPERTY_VALUE,
    TEXT_CONTENT,
    UNDEFINED
};
enum Tag
{
    WINDOW,
    RECT,
    BUTTON,
    GRID,
    TEXT,
    NAT,
};





// Structs ---------------------- //
struct Text_Ref
{
    uint32_t node_ID;
    uint32_t buffer_index;
    uint32_t char_count;
    uint32_t char_capacity; // excludes the null terminator
};

struct Node
{
    SDL_Window* window;
    NVGcontext* vg;
    uint32_t ID;
    enum Tag tag;
    float x, y, width, height, preferred_width, preferred_height;
    float min_width, max_width, min_height, max_height;
    float gap, content_width, content_height;
    int parent_index;
    int first_child_index;
    int text_ref_index;
    uint16_t child_capacity;
    uint16_t child_count;
    uint8_t pad_top, pad_bottom, pad_left, pad_right;
    uint8_t border_top, border_bottom, border_left, border_right;
    uint8_t border_radius_tl, border_radius_tr, border_radius_bl, border_radius_br;
    uint8_t background_r, background_g, background_b, background_a;
    uint8_t border_r, border_g, border_b, border_a;
    char layout_flags;
    char horizontal_alignment;
    char vertical_alignment;
};

struct Property_Text_Ref
{
    uint32_t NU_Token_index;
    uint32_t src_index;
    uint8_t char_count;
};

struct Arena_Free_Element
{
    uint32_t index;
    uint32_t length;
};

struct Text_Arena
{
    struct Vector free_list;
    struct Vector text_refs;
    struct Vector char_buffer;
};

struct Font_Resource
{
    char* name;
    uint8_t* data; 
    int size;  
};

struct Font_Registry {
    int* font_ids;  
    int count;      
};

struct RGB {
    uint8_t r, g, b;
};

struct UI_Tree
{
    struct Vector tree_stack[MAX_TREE_DEPTH];
    struct Text_Arena text_arena;
    uint16_t deepest_layer;
    struct Vector font_resources;
    struct Vector font_registries;
};

// Structs ---------------------- //



// Internal Functions ----------- //

static enum NU_Token NU_Word_To_NU_Token(char word[], uint8_t word_char_count)
{
    for (uint8_t i=0; i<KEYWORD_COUNT; i++) {
        size_t len = keyword_lengths[i];
        if (len == word_char_count && memcmp(word, keywords[i], len) == 0) {
            return i;
        }
    }
    return UNDEFINED;
}

static enum Tag NU_Token_To_Tag(enum NU_Token NU_Token)
{
    int tag_candidate = NU_Token - PROPERTY_COUNT;
    if (tag_candidate < 0) return NAT;
    return NU_Token - PROPERTY_COUNT;
}

static void NU_Tokenise(char* src_buffer, uint32_t src_length, struct Vector* NU_Token_vector, struct Vector* ptext_ref_vector, struct Text_Arena* text_arena) 
{
    // Store current NU_Token word
    uint8_t word_char_index = 0;
    char word[24];

    // Store global text char indexes
    uint32_t text_arena_buffer_index = 0;
    uint32_t text_char_count = 0;

    // Context
    uint8_t ctx = 0; // 0 == globalspace, 1 == commentspace, 2 == tagspace, 3 == propertyspace

    // Iterate over src file 
    uint32_t i = 0;
    while (i < src_length)
    {
        char c = src_buffer[i];

        // Comment begins
        if (i < src_length - 3 && c == '<' && src_buffer[i+1] == '!' && src_buffer[i+2] == '-' && src_buffer[i+3] == '-' && ctx == 0)
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

        // In comment
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
                char null_terminator = '\0';
                Vector_Push(&text_arena->char_buffer, &null_terminator); // add null terminator
                struct Text_Ref new_ref;
                new_ref.char_count = text_char_count;
                new_ref.char_capacity = text_char_count;
                new_ref.buffer_index = text_arena_buffer_index;
                Vector_Push(&text_arena->text_refs, &new_ref);

                // Add text content token
                enum NU_Token t = TEXT_CONTENT;
                Vector_Push(NU_Token_vector, &t);
            }
            text_char_count = 0;

            enum NU_Token t = OPEN_END_TAG;
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
                char null_terminator = '\0';
                Vector_Push(&text_arena->char_buffer, &null_terminator); // add null terminator
                struct Text_Ref new_ref;
                new_ref.char_count = text_char_count;
                new_ref.char_capacity = text_char_count;
                new_ref.buffer_index = text_arena_buffer_index;
                Vector_Push(&text_arena->text_refs, &new_ref);

                // Add text content token
                enum NU_Token t = TEXT_CONTENT;
                Vector_Push(NU_Token_vector, &t);
            }
            text_char_count = 0;

            enum NU_Token t = OPEN_TAG;
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
                enum NU_Token t = NU_Word_To_NU_Token(word, word_char_index);
                Vector_Push(NU_Token_vector, &t);
            }
            enum NU_Token t = CLOSE_END_TAG;
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
                enum NU_Token t = NU_Word_To_NU_Token(word, word_char_index);
                Vector_Push(NU_Token_vector, &t);
            }
            enum NU_Token t = CLOSE_TAG;
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
                enum NU_Token t = NU_Word_To_NU_Token(word, word_char_index);
                Vector_Push(NU_Token_vector, &t);
            }
            word_char_index = 0;
            enum NU_Token t = PROPERTY_ASSIGNMENT;
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
            enum NU_Token t = PROPERTY_VALUE;
            Vector_Push(NU_Token_vector, &t);
            if (word_char_index > 0)
            {
                struct Property_Text_Ref ref;
                ref.NU_Token_index = NU_Token_vector->size - 1;
                ref.src_index = i - word_char_index;
                ref.char_count = word_char_index;
                Vector_Push(ptext_ref_vector, &ref);
            }
            word_char_index = 0;
            ctx = 2;
            i+=1;
            continue;
        }

        // If global space and split character
        if (ctx == 0 && c != '\t' && c != '\n')
        {
            // text continues
            if (text_char_count > 0)
            {
                Vector_Push(&text_arena->char_buffer, &c);
                text_char_count += 1;
            }

            // text starts
            if (text_char_count == 0 && c != ' ')
            {
                text_arena_buffer_index = text_arena->char_buffer.size;
                Vector_Push(&text_arena->char_buffer, &c);
                text_char_count = 1;
            }

            i+=1;
            continue;
        }

        // If split character
        if (c == ' ' || c == '\t' || c == '\n')
        {
            if (word_char_index > 0) {
                enum NU_Token t = NU_Word_To_NU_Token(word, word_char_index);
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
        enum NU_Token t = NU_Word_To_NU_Token(word, word_char_index);
        Vector_Push(NU_Token_vector, &t);
    }
}

static int NU_Is_Token_Property(enum NU_Token NU_Token)
{
    return NU_Token < PROPERTY_COUNT;
}

static int Property_Text_To_Float(float* result, char* src_buffer, struct Property_Text_Ref* ptext_ref)
{
    *result = 0.0f;
    float fraction_divider = 1.0f;
    int decimal_found = 0;

    for (uint8_t i = 0; i < ptext_ref->char_count; i++)
    {
        char c = src_buffer[ptext_ref->src_index + i];

        if (c == '.')
        {
            if (decimal_found) 
            {
                *result = 0.0f;
                return -1;
            }
            decimal_found = 1;
            continue;
        }

        if (c < '0' || c > '9')
        {
            *result = 0.0f;
            return -1;
        }

        int digit = c - '0';

        if (!decimal_found)
        {
            *result = (*result * 10.0f) + digit;
        }
        else
        {
            fraction_divider *= 10.0f;
            *result += digit / fraction_divider;
        }
    }

    return 0;   
}

static int Property_Text_To_uint8_t(uint8_t* result, char* src_buffer, struct Property_Text_Ref* ptext_ref)
{
    *result = 0;
    for (uint8_t i = 0; i < ptext_ref->char_count; i++) {
        char c = src_buffer[ptext_ref->src_index + i];
        if (c < '0' || c > '9') {
            *result = 0;
            return -1;
        }
        int digit = c - '0';
        if (*result > (UINT8_MAX - digit) / 10) {
            *result = 255; // limit to max uint8_t value
        }
        *result = (uint8_t)(*result * 10 + digit);
    }
    return 0;
}


static int AssertRootGrammar(struct Vector* token_vector)
{
    // ENFORCE RULE: FIRST TOKEN MUST BE OPEN TAG
    // ENFORCE RULE: SECOND TOKEN MUST BE TAG NAME
    // ENFORCE RULE: THIRD TOKEN MUST BE CLOSE_TAG | PROPERTY
    int root_open = token_vector->size > 2 && 
        *((enum NU_Token*) Vector_Get(token_vector, 0)) == OPEN_TAG &&
        *((enum NU_Token*) Vector_Get(token_vector, 1)) == WINDOW_TAG;

    if (token_vector->size > 2 &&
        *((enum NU_Token*) Vector_Get(token_vector, 0)) == OPEN_TAG &&
        *((enum NU_Token*) Vector_Get(token_vector, 1)) == WINDOW_TAG) 
    {
        if (token_vector->size > 5 && 
            *((enum NU_Token*) Vector_Get(token_vector, token_vector->size-3)) == OPEN_END_TAG &&
            *((enum NU_Token*) Vector_Get(token_vector, token_vector->size-2)) == WINDOW_TAG && 
            *((enum NU_Token*) Vector_Get(token_vector, token_vector->size-1)) == CLOSE_TAG)
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
    if (i < token_vector->size - 2 && NU_Token_To_Tag(*((enum NU_Token*) Vector_Get(token_vector, i+1))) != NAT)
    {
        enum NU_Token third_token = *((enum NU_Token*) Vector_Get(token_vector, i+2));
        if (third_token == CLOSE_TAG || third_token == CLOSE_END_TAG || NU_Is_Token_Property(third_token)) return 0; // Success
    }
    return -1; // Failure
}

static int AssertPropertyGrammar(struct Vector* token_vector, int i)
{
    // ENFORCE RULE: NEXT TOKEN SHOULD BE PROPERTY ASSIGN
    // ENFORCE RULE: THIRD TOKEN SHOULD BE PROPERTY TEXT
    if (i < token_vector->size - 2 && *((enum NU_Token*) Vector_Get(token_vector, i+1)) == PROPERTY_ASSIGNMENT)
    {
        if (*((enum NU_Token*) Vector_Get(token_vector, i+2)) == PROPERTY_VALUE) return 0; // Success
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
        NU_Token_To_Tag(*((enum NU_Token*) Vector_Get(token_vector, i+1))) == openTag && 
        *((enum NU_Token*) Vector_Get(token_vector, i+2)) == CLOSE_TAG) return 0; // Success
    else {
        printf("%s", "[Generate Tree] Error! Closing tag does not match.");
        printf("%s %d %s %d", "close tag:", NU_Token_To_Tag(*((enum NU_Token*) Vector_Get(token_vector, i+1))), "open tag:", openTag);
    }
    return -1; // Failure
}

static int Hex_To_Int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1; 
}

bool Parse_Hexcode(const char* string, int char_count, struct RGB* rgb) {
    if ((char_count != 7 && char_count != 9) || string[0] != '#') return false;
    int r1 = Hex_To_Int(string[1]);
    int r2 = Hex_To_Int(string[2]);
    int g1 = Hex_To_Int(string[3]);
    int g2 = Hex_To_Int(string[4]);
    int b1 = Hex_To_Int(string[5]);
    int b2 = Hex_To_Int(string[6]);
    if (r1 < 0 || r2 < 0 || g1 < 0 || g2 < 0 || b1 < 0 || b2 < 0) return false;
    rgb->r = (uint8_t)((r1 << 4) | r2);
    rgb->g = (uint8_t)((g1 << 4) | g2);
    rgb->b = (uint8_t)((b1 << 4) | b2);
    return true;
}

static int NU_Generate_Tree(char* src_buffer, uint32_t src_length, struct UI_Tree* ui_tree, struct Vector* NU_Token_vector, struct Vector* ptext_ref_vector)
{
    // Enforce root grammar
    if (AssertRootGrammar(NU_Token_vector) != 0) 
    {
        return -1; // Failure 
    }

    // Get first property text reference
    uint32_t property_text_index = 0;
    struct Property_Text_Ref* current_property_text;
    if (ptext_ref_vector->size > 0) current_property_text = Vector_Get(ptext_ref_vector, property_text_index);

    // Get first text content reference
    uint32_t text_content_ref_index = 0;

    // Iterate over all NU_Tokens
    int i = 0;
    int current_layer = -1;
    ui_tree->deepest_layer = 0;
    struct Node* current_node = NULL;
    uint32_t current_node_ID = 0;
    while (i < NU_Token_vector->size - 3)
    {
        const enum NU_Token NU_Token = *((enum NU_Token*) Vector_Get(NU_Token_vector, i));

        // New tag -> Add a new node
        if (NU_Token == OPEN_TAG)
        {
            if (AssertNewTagGrammar(NU_Token_vector, i) == 0)
            {
                // Enforce max tree depth
                if (current_layer+1 == MAX_TREE_DEPTH)
                {
                    printf("%s", "[Generate Tree] Error! Exceeded max tree depth of 64");
                    return -1; // Failure
                }

                // Create a new node
                struct Node new_node;
                current_node_ID = ((uint32_t) (current_layer + 1) << 24) | (ui_tree->tree_stack[current_layer+1].size & 0xFFFFFF); // Max depth = 256, Max node index = 16,777,215
                new_node.ID = current_node_ID;
                new_node.tag = NU_Token_To_Tag(*((enum NU_Token*) Vector_Get(NU_Token_vector, i+1)));
                new_node.window = NULL; 
                new_node.vg = NULL;
                new_node.preferred_width = 0.0f;
                new_node.preferred_height = 0.0f;
                new_node.gap = 5.0f;
                new_node.max_width = 10e20f;
                new_node.min_width = 0.0f;
                new_node.pad_top = 3;
                new_node.pad_bottom = 3;
                new_node.pad_left = 3;
                new_node.pad_right = 3;
                new_node.border_top = 0;
                new_node.border_bottom = 0;
                new_node.border_left = 0;
                new_node.border_right = 0;
                new_node.border_radius_tl = 5;
                new_node.border_radius_tr = 5;
                new_node.border_radius_bl = 5;
                new_node.border_radius_br = 5;
                if (new_node.tag == BUTTON) {
                    new_node.background_r = 25;
                    new_node.background_g = 25;
                    new_node.background_b = 25;

                    new_node.border_r = 69;
                    new_node.border_g = 101;
                    new_node.border_b = 153;

                    new_node.border_top = 2;
                    new_node.border_bottom = 2;
                    new_node.border_left = 2;
                    new_node.border_right = 2;
                }
                else if (new_node.tag == RECT)
                {
                    new_node.border_r = 50;
                    new_node.border_g = 50;
                    new_node.border_b = 50;

                    new_node.background_r = 2;
                    new_node.background_g = 2;
                    new_node.background_b = 2;

                    new_node.border_top = 1;
                    new_node.border_bottom = 1;
                    new_node.border_left = 1;
                    new_node.border_right = 1;
                }
                else {
                    new_node.background_r = 2;
                    new_node.background_g = 2;
                    new_node.background_b = 2;

                    new_node.border_r = 156;
                    new_node.border_g = 100;
                    new_node.border_b = 5;
                }
                new_node.child_count = 0;
                new_node.first_child_index = -1;
                new_node.text_ref_index = -1;
                new_node.layout_flags = 0;
                new_node.parent_index = ui_tree->tree_stack[current_layer].size - 1; 

                // Add node to tree
                struct Vector* node_layer = &ui_tree->tree_stack[current_layer+1];
                Vector_Push(node_layer, &new_node);
                current_node = (struct Node*) Vector_Get(node_layer, node_layer->size -1);
                if (current_layer != -1) // Only equals -1 for the root window node (optimisation would be to remove this check and append root node at beggining of function)
                {
                    // Inform parent that parent has new child
                    struct Node* parentNode = (struct Node*) Vector_Get(&ui_tree->tree_stack[current_layer], new_node.parent_index);
                    if (parentNode->child_count == 0)
                    {
                        parentNode->first_child_index = ui_tree->tree_stack[current_layer+1].size - 1;
                    }
                    parentNode->child_count += 1;
                    parentNode->child_capacity += 1;
                }

                // Move one layer deeper
                current_layer++;

                ui_tree->deepest_layer = MAX(ui_tree->deepest_layer, current_layer);

                // Increment NU_Token
                i+=2;
                continue;
            }
            else
            {
                return -1;
            }
        }

        // Open end tag -> tag closes -> move up one layer
        if (NU_Token == OPEN_END_TAG)
        {
            if (current_layer < 0) break;
            
            // Check close grammar
            enum Tag openTag = ((struct Node*) Vector_Get(&ui_tree->tree_stack[current_layer], ui_tree->tree_stack[current_layer].size - 1))->tag;
            if (AssertTagCloseStartGrammar(NU_Token_vector, i, openTag) != 0)
            {
                return -1; // Failure
            }

            // Move one layer higher
            current_layer--;

            // Increment NU_Token
            i+=3;
            continue;
        }

        // Close end tag -> tag self closes -> move up one layer
        if (NU_Token == CLOSE_END_TAG)
        {
            // Move one layer higher
            current_layer--;

            // Increment NU_Token
            i+=1;
            continue;
        }

        // Text content
        if (NU_Token == TEXT_CONTENT)
        {
            struct Text_Ref* text_ref = (struct Text_Ref*) Vector_Get(&ui_tree->text_arena.text_refs, text_content_ref_index);
            text_ref->node_ID = current_node_ID;
            current_node->text_ref_index = text_content_ref_index;
            text_content_ref_index += 1;

            // Increment NU_Token
            i+=1;
            continue;
        }

        // Property tag -> assign property to node
        if (NU_Is_Token_Property(NU_Token))
        {
            if (AssertPropertyGrammar(NU_Token_vector, i) == 0)
            {
                current_property_text = Vector_Get(ptext_ref_vector, property_text_index);
                property_text_index += 1;
                char c = src_buffer[current_property_text->src_index];
                char* ptext = &src_buffer[current_property_text->src_index];

                // Get the property value text
                switch (NU_Token)
                {
                    // Set layout direction
                    case LAYOUT_DIRECTION_PROPERTY:
                        if (c == 'h') 
                            current_node->layout_flags |= LAYOUT_HORIZONTAL;
                        else 
                            current_node->layout_flags |= LAYOUT_VERTICAL;
                        break;

                    // Set growth
                    case GROW_PROPERTY:
                        switch(c)
                        {
                            case 'v':
                                current_node->layout_flags |= GROW_VERTICAL;
                                break;
                            case 'h':
                                current_node->layout_flags |= GROW_HORIZONTAL;
                                break;
                            case 'b':
                                current_node->layout_flags |= (GROW_HORIZONTAL | GROW_VERTICAL);
                                break;
                        }

                        break;
                    
                    // Set overflow behaviour
                    case OVERFLOW_V_PROPERTY:
                        if (c == 's') 
                            current_node->layout_flags |= OVERFLOW_VERTICAL_SCROLL;
                        break;
                    
                    case OVERFLOW_H_PROPERTY:
                        if (c == 's') 
                            current_node->layout_flags |= OVERFLOW_HORIZONTAL_SCROLL;
                        break;
                    
                    // Set gap 
                    case GAP_PROPERTY:
                        float gap;
                        if (Property_Text_To_Float(&gap, src_buffer, current_property_text) == 0)
                            current_node->gap = gap;
                        break;
                    
                    // Set preferred width
                    case WIDTH_PROPERTY:
                        float width; 
                        if (Property_Text_To_Float(&width, src_buffer, current_property_text) == 0) 
                            current_node->preferred_width = width;
                        break;

                    // Set min width
                    case MIN_WIDTH_PROPERTY:
                        float min_width;
                        if (Property_Text_To_Float(&min_width, src_buffer, current_property_text) == 0) 
                            current_node->min_width = min_width;
                        break;

                    // Set max width
                    case MAX_WIDTH_PROPERTY:
                        float max_width;
                        if (Property_Text_To_Float(&max_width, src_buffer, current_property_text) == 0) 
                            current_node->max_width = max_width;
                        break;

                    // Set preferred height
                    case HEIGHT_PROPERTY:
                        float height;
                        if (Property_Text_To_Float(&height, src_buffer, current_property_text) == 0) 
                            current_node->preferred_height = height;
                        break;

                    // Set min height
                    case MIN_HEIGHT_PROPERTY:
                        float min_height;
                        if (Property_Text_To_Float(&min_height, src_buffer, current_property_text) == 0) 
                            current_node->min_height = min_height;
                        break;

                    // Set max height
                    case MAX_HEIGHT_PROPERTY:
                        float max_height;
                        if (Property_Text_To_Float(&max_height, src_buffer, current_property_text) == 0) {
                            current_node->min_height = max_height;
                        }
                        break;

                    // Set horizontal alignment
                    case ALIGN_H_PROPERTY:
                        if (current_property_text->char_count < 4) break;
                        if (memcmp(ptext, "left", 4) == 0) {
                            current_node->horizontal_alignment = 0;
                        } else if (memcmp(ptext, "center", 6) == 0) {
                            current_node->horizontal_alignment = 1;
                        } else if (memcmp(ptext, "right", 5) == 0) {
                            current_node->horizontal_alignment = 2;
                        }
                        break;

                    // Set vertical alignment
                    case ALIGN_V_PROPERTY:
                        if (current_property_text->char_count < 4) break;
                        if (memcmp(ptext, "left", 4) == 0) {
                            current_node->vertical_alignment = 0;
                        } else if (memcmp(ptext, "center", 6) == 0) {
                            current_node->vertical_alignment = 1;
                        } else if (memcmp(ptext, "right", 5) == 0) {
                            current_node->vertical_alignment = 2;
                        }
                        break;

                    // Set background colour
                    case BACKGROUND_COLOUR_PROPERTY:
                        struct RGB rgb;
                        if (Parse_Hexcode(ptext, current_property_text->char_count, &rgb)) {
                            current_node->background_r = rgb.r;
                            current_node->background_g = rgb.g;
                            current_node->background_b = rgb.b;
                        }
                        break;

                    // Set border colour
                    case BORDER_COLOUR_PROPERTY:
                        if (Parse_Hexcode(ptext, current_property_text->char_count, &rgb)) {
                            current_node->border_r = rgb.r;
                            current_node->border_g = rgb.g;
                            current_node->border_b = rgb.b;
                        }
                        break;
                        
                    default:
                        break;

                    // Set border width
                    case BORDER_TOP_WIDTH_PROPERTY:
                        uint8_t border_width;
                        if (Property_Text_To_uint8_t(&border_width, src_buffer, current_property_text) == 0) {
                            current_node->border_top = border_width;
                        }
                        break;
                    case BORDER_BOTTOM_WIDTH_PROPERTY:
                        if (Property_Text_To_uint8_t(&border_width, src_buffer, current_property_text) == 0) {
                            current_node->border_bottom = border_width;
                        }
                        break;
                    case BORDER_LEFT_WIDTH_PROPERTY:

                        if (Property_Text_To_uint8_t(&border_width, src_buffer, current_property_text) == 0) {
                            current_node->border_left = border_width;
                        }
                        break;
                    case BORDER_RIGHT_WIDTH_PROPERTY:
                        if (Property_Text_To_uint8_t(&border_width, src_buffer, current_property_text) == 0) {
                            current_node->border_right = border_width;
                        }
                        break;

                    // Set border radii
                    case BORDER_RADIUS_PROPERTY:
                        uint8_t border_radius;
                        if (Property_Text_To_uint8_t(&border_radius, src_buffer, current_property_text) == 0) {
                            current_node->border_radius_tl = border_radius;
                            current_node->border_radius_tr = border_radius;
                            current_node->border_radius_bl = border_radius;
                            current_node->border_radius_br = border_radius;
                        }
                        break;
                    case BORDER_TOP_LEFT_RADIUS_PROPERTY:
                        if (Property_Text_To_uint8_t(&border_radius, src_buffer, current_property_text) == 0) {
                            current_node->border_radius_tl = border_radius;
                        }
                        break;
                    case BORDER_TOP_RIGHT_RADIUS_PROPERTY:
                        if (Property_Text_To_uint8_t(&border_radius, src_buffer, current_property_text) == 0) {
                            current_node->border_radius_tr = border_radius;
                        }
                        break;
                    case BORDER_BOTTOM_LEFT_RADIUS_PROPERTY:
                        if (Property_Text_To_uint8_t(&border_radius, src_buffer, current_property_text) == 0) {
                            current_node->border_radius_bl = border_radius;
                        }
                        break;
                    case BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY:
                        if (Property_Text_To_uint8_t(&border_radius, src_buffer, current_property_text) == 0) {
                            current_node->border_radius_br = border_radius;
                        }
                        break;

                    // Set padding
                    case PADDING_PROPERTY:
                        uint8_t pad;
                        if (Property_Text_To_uint8_t(&pad, src_buffer, current_property_text) == 0) {
                            current_node->pad_top    = pad;
                            current_node->pad_bottom = pad;
                            current_node->pad_left   = pad;
                            current_node->pad_right  = pad;
                        }
                        break;
                    case PADDING_TOP_PROPERTY:
                        if (Property_Text_To_uint8_t(&pad, src_buffer, current_property_text) == 0) {
                            current_node->pad_top = pad;
                        }
                        break;
                    case PADDING_BOTTOM_PROPERTY:
                        if (Property_Text_To_uint8_t(&pad, src_buffer, current_property_text) == 0) {
                            current_node->pad_bottom = pad;
                        }
                        break;
                    case PADDING_LEFT_PROPERTY:
                        if (Property_Text_To_uint8_t(&pad, src_buffer, current_property_text) == 0) {
                            current_node->pad_left = pad;
                        }
                        break;
                    case PADDING_RIGHT_PROPERTY:
                        if (Property_Text_To_uint8_t(&pad, src_buffer, current_property_text) == 0) {
                            current_node->pad_right = pad;
                        }
                        break;
                }

                // Increment NU_Token
                i+=3;
                continue;
            }
            else {
                return -1;
            }
        }
        i+= 1;
    }
    return 0; // Success
}

// Internal Functions ----------- //



// Public Functions ------------- //

int NU_Parse(char* filepath, struct UI_Tree* ui_tree)
{
    // Open XML source file and load into buffer
    FILE* f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "Cannot open file '%s': %s\n", filepath, strerror(errno));
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long file_size_long = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (file_size_long > UINT32_MAX) {
        printf("%s", "Src file is too large! It must be < 4 294 967 295 Bytes");
        return -1;
    }
    uint32_t src_length = (uint32_t)file_size_long;
    char* src_buffer = malloc(src_length + 1);
    src_length = fread(src_buffer, 1, file_size_long, f);
    src_buffer[src_length] = '\0'; 
    fclose(f);

    // Init Token vector and reserve ~1MB
    struct Vector NU_Token_vector;
    struct Vector ptext_ref_vector;

    // Init vectors
    Vector_Reserve(&NU_Token_vector, sizeof(enum NU_Token), 250000); // reserve ~1MB
    Vector_Reserve(&ptext_ref_vector, sizeof(struct Property_Text_Ref), 100000); // reserve ~900KB
    Vector_Reserve(&ui_tree->text_arena.free_list, sizeof(struct Arena_Free_Element), 100000); // reserve ~800KB
    Vector_Reserve(&ui_tree->text_arena.text_refs, sizeof(struct Text_Ref), 100000); // reserve ~800KB
    Vector_Reserve(&ui_tree->text_arena.char_buffer, sizeof(char), 1000000); // reserve ~1MB

    // Init UI tree layers -> reserve 100 nodes per stack layer = 384KB
    Vector_Reserve(&ui_tree->tree_stack[0], sizeof(struct Node), 1); // 1 root element
    for (int i=1; i<MAX_TREE_DEPTH; i++) {
        Vector_Reserve(&ui_tree->tree_stack[i], sizeof(struct Node), 100);
    }

    // Tokenise the file source
    NU_Tokenise(src_buffer, src_length, &NU_Token_vector, &ptext_ref_vector, &ui_tree->text_arena);

    // Generate UI tree
    if (NU_Generate_Tree(src_buffer, src_length, ui_tree, &NU_Token_vector, &ptext_ref_vector) != 0) return -1; // Failure

    // Free token and property text reference memory
    Vector_Free(&NU_Token_vector);
    Vector_Free(&ptext_ref_vector);
    return 0; // Success
}

void NU_Free_UI_Tree_Memory(struct UI_Tree* ui_tree)
{
    Vector_Free(&ui_tree->text_arena.free_list);
    Vector_Free(&ui_tree->text_arena.text_refs);
    Vector_Free(&ui_tree->text_arena.char_buffer);
}

// Public Functions ------------- //