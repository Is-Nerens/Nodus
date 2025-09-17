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
#include "string_set.h"
#include "hashmap.h"
#include "nu_convert.h"
#include "nu_image.h"
#include "nu_font.h"
#include "nu_draw.h"

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>

#define PROPERTY_COUNT 33
#define KEYWORD_COUNT 46

static const char* keywords[] = {
    "id", "class", "dir", "grow", "overflowV", "overflowH", "gap",
    "width", "minWidth", "maxWidth", "height", "minHeight", "maxHeight", 
    "alignH", "alignV",
    "background", "borderColour",
    "border", "borderTop", "borderBottom", "borderLeft", "borderRight",
    "borderRadius", "borderRadiusTopLeft", "borderRadiusTopRight", "borderRadiusBottomLeft", "borderRadiusBottomRight",
    "pad", "padTop", "padBottom", "padLeft", "padRight",
    "imageSrc",
    "window",
    "rect",
    "button",
    "grid",
    "text",
    "image"
};
static const uint8_t keyword_lengths[] = { 
    2, 5, 3, 4, 9, 9, 3,
    5, 8, 8, 6, 9, 9,      // width height
    6, 6,                  // alignment
    10, 12,                // background, border colour
    6, 9, 12, 10, 11,      // border width
    12, 19, 20, 22, 23,    // border radius
    3, 6, 9, 7, 11,        // padding
    8,                     // image src
    6, 4, 6, 4, 4, 5       // selectors
};
enum NU_Token
{
    ID_PROPERTY,
    CLASS_PROPERTY,
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
    IMAGE_SOURCE_PROPERTY,
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
    IMAGE,
    NAT,
};




// ------------------------------ 
// Structs ---------------------- 
// ------------------------------ 
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
    uint64_t inline_style_flags;
    char* class;
    char* id;
    enum Tag tag;
    uint32_t ID;
    GLuint gl_image_handle;
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

struct UI_Tree
{
    struct Vector tree_stack[MAX_TREE_DEPTH];
    struct Vector windows;
    struct Vector nano_vg_contexts;
    struct Text_Arena text_arena;
    struct Vector font_resources;
    struct Vector font_registries;
    String_Set class_string_set;
    String_Set id_string_set;
    struct Vector hovered_nodes;
    struct Node* hovered_node;
    struct Node* mouse_down_node;
    int16_t deepest_layer;
};


// ------------------------------
//--- Internal Functions --------
// ------------------------------
static enum NU_Token NU_Word_To_Token(char word[], uint8_t word_char_count)
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
    char word[32];

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
                enum NU_Token t = NU_Word_To_Token(word, word_char_index);
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
                enum NU_Token t = NU_Word_To_Token(word, word_char_index);
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
                enum NU_Token t = NU_Word_To_Token(word, word_char_index);
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
                enum NU_Token t = NU_Word_To_Token(word, word_char_index);
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
        enum NU_Token t = NU_Word_To_Token(word, word_char_index);
        Vector_Push(NU_Token_vector, &t);
    }
}

static int NU_Is_Token_Property(enum NU_Token NU_Token)
{
    return NU_Token < PROPERTY_COUNT;
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

static void NU_Tree_Init(struct UI_Tree* ui_tree)
{
    Vector_Reserve(&ui_tree->windows, sizeof(SDL_Window*), 8);
    Vector_Reserve(&ui_tree->nano_vg_contexts, sizeof(NVGcontext*), 8);
    Vector_Reserve(&ui_tree->font_resources, sizeof(struct Font_Resource), 4);
    Vector_Reserve(&ui_tree->text_arena.free_list, sizeof(struct Arena_Free_Element), 100);
    Vector_Reserve(&ui_tree->text_arena.text_refs, sizeof(struct Text_Ref), 100);
    Vector_Reserve(&ui_tree->text_arena.char_buffer, sizeof(char), 25000); 
    Vector_Reserve(&ui_tree->font_registries, sizeof(struct Vector), 8);
    Vector_Reserve(&ui_tree->hovered_nodes, sizeof(struct Node*), 32);
    String_Set_Init(&ui_tree->class_string_set, 1024, 100);
    String_Set_Init(&ui_tree->id_string_set, 1024, 100);
    ui_tree->hovered_node = NULL;
    ui_tree->mouse_down_node = NULL;
    ui_tree->deepest_layer = 0;
}

void NU_Tree_Cleanup(struct UI_Tree* ui_tree)
{
    Vector_Free(&ui_tree->windows);
    Vector_Free(&ui_tree->nano_vg_contexts);
    Vector_Free(&ui_tree->text_arena.free_list);
    Vector_Free(&ui_tree->text_arena.text_refs);
    Vector_Free(&ui_tree->text_arena.char_buffer);
    Vector_Free(&ui_tree->font_resources);
    Vector_Free(&ui_tree->font_registries);
    Vector_Free(&ui_tree->hovered_nodes);
    String_Set_Free(&ui_tree->class_string_set);
    String_Set_Free(&ui_tree->id_string_set);
    ui_tree->hovered_node = NULL;
    ui_tree->mouse_down_node = NULL;
    ui_tree->deepest_layer = 0;
}

static void NU_Create_Main_Window(struct UI_Tree* ui_tree, struct Node* window_node) 
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_Window* main_window = SDL_CreateWindow("window", 500, 400, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    // Create OpenGL context for the main window
    SDL_GLContext context = SDL_GL_CreateContext(main_window);
    SDL_GL_MakeCurrent(main_window, context);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glewInit();


    // NanoVG context (per-window)
    NVGcontext* new_nano_vg_context = nvgCreateGL3(NVG_STENCIL_STROKES);
    

    // Fonts for NanoVG
    struct Vector font_registry;
    Vector_Reserve(&font_registry, sizeof(int), 8);
    for (int i=0; i<ui_tree->font_resources.size; i++) {
        struct Font_Resource* font = Vector_Get(&ui_tree->font_resources, i);
        int fontID = nvgCreateFontMem(new_nano_vg_context, font->name, font->data, font->size, 0);
        Vector_Push(&font_registry, &fontID);
    }

    window_node->window = main_window;
    window_node->vg = new_nano_vg_context;
    Vector_Push(&ui_tree->windows, &main_window);
    Vector_Push(&ui_tree->nano_vg_contexts, &new_nano_vg_context);
    Vector_Push(&ui_tree->font_registries, &font_registry);
    NU_Draw_Init();
}

static void NU_Apply_Node_Defaults(struct Node* node)
{
    node->window = NULL;
    node->vg = NULL;
    node->inline_style_flags = 0;
    node->class = NULL;
    node->id = NULL;
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
    node->parent_index = -1;
    node->first_child_index = -1;
    node->text_ref_index = -1;
    node->child_capacity = 0;
    node->child_count = 0;
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
    node->layout_flags = 0;
    node->horizontal_alignment = 0;
    node->vertical_alignment = 0;
}

static int NU_Generate_Tree(char* src_buffer, uint32_t src_length, struct UI_Tree* ui_tree, struct Vector* NU_Token_vector, struct Vector* ptext_ref_vector)
{
    // Enforce root grammar
    if (AssertRootGrammar(NU_Token_vector) != 0) {
        return -1; // Failure 
    }

    // -----------------------
    // Create root window node
    // -----------------------
    struct Node root_node;
    struct Vector* root_layer = &ui_tree->tree_stack[0];
    Vector_Push(root_layer, &root_node);
    struct Node* root_window_node = (struct Node*) Vector_Get(root_layer, root_layer->size-1);
    NU_Apply_Node_Defaults(root_window_node); // Default styles
    // Set the ID: upper 8 bits = depth (layer), lower 24 bits = index in layer
    root_window_node->ID = ((uint32_t) (0) << 24) | (ui_tree->tree_stack[1].size & 0xFFFFFF); 
    root_window_node->tag = WINDOW;
    root_window_node->parent_index = -1; 
    root_window_node->child_count = 0;
    NU_Create_Main_Window(ui_tree, root_window_node);

    // ---------------------------------
    // Get first property text reference
    // ---------------------------------
    struct Property_Text_Ref* current_property_text;
    if (ptext_ref_vector->size > 0) current_property_text = Vector_Get(ptext_ref_vector, 0);
    uint32_t text_content_ref_index = 0;
    uint32_t property_text_index = 0;

    // --------------------------
    // (string -> int) ----------
    // --------------------------
    struct sHashmap image_filepath_to_handle_hmap;
    sHashmap_Init(&image_filepath_to_handle_hmap, sizeof(GLuint), 20);

    // ------------------------------------
    // Iterate over all NU_Tokens ---------
    // ------------------------------------
    int i = 2; 
    int current_layer = 0; 
    struct Node* current_node = root_window_node;
    while (i < NU_Token_vector->size - 3)
    {
        const enum NU_Token NU_Token = *((enum NU_Token*) Vector_Get(NU_Token_vector, i));

        // -------------------------
        // New tag -> Add a new node
        // -------------------------
        if (NU_Token == OPEN_TAG)
        {
            if (AssertNewTagGrammar(NU_Token_vector, i) == 0)
            {
                // ----------------------
                // Enforce max tree depth
                // ----------------------
                if (current_layer+1 == MAX_TREE_DEPTH)
                {
                    printf("%s", "[Generate Tree] Error! Exceeded max tree depth of 32");
                    sHashmap_Free(&image_filepath_to_handle_hmap);
                    return -1; // Failure
                }   

                // ----------------------------------------
                // Create a new node and add it to the tree
                // ----------------------------------------
                struct Node new_node;
                NU_Apply_Node_Defaults(&new_node); // Apply Default style
                new_node.tag = NU_Token_To_Tag(*((enum NU_Token*) Vector_Get(NU_Token_vector, i+1)));
                // Set the ID: upper 8 bits = depth (layer), lower 24 bits = index in layer
                struct Vector* node_layer = &ui_tree->tree_stack[current_layer+1];
                uint32_t node_index_in_layer = node_layer->size; 
                new_node.ID = ((uint32_t)(current_layer + 1) << 24) | (node_index_in_layer & 0xFFFFFF);
                new_node.parent_index = ui_tree->tree_stack[current_layer].size-1; ;
                Vector_Push(node_layer, &new_node);
                current_node = (struct Node*) Vector_Get(node_layer, node_layer->size-1);

                // -------------------------------
                // Add node to parent's child list
                // -------------------------------
                struct Node* parentNode = Vector_Get(&ui_tree->tree_stack[current_layer], new_node.parent_index);
                if (parentNode->child_count == 0) parentNode->first_child_index = ui_tree->tree_stack[current_layer+1].size-1;
                parentNode->child_count += 1;
                parentNode->child_capacity += 1;

                current_layer++; // Move one layer deeper
                ui_tree->deepest_layer = MAX(ui_tree->deepest_layer, current_layer);
                i+=2; // Increment token index
                continue;
            }
            else
            {
                sHashmap_Free(&image_filepath_to_handle_hmap);
                return -1;
            }
        }

        // -----------------------------------------------
        // Open end tag -> tag closes -> move up one layer
        // -----------------------------------------------
        if (NU_Token == OPEN_END_TAG)
        {
            if (current_layer < 0) break;
            
            // Check close grammar
            enum Tag openTag = ((struct Node*) Vector_Get(&ui_tree->tree_stack[current_layer], ui_tree->tree_stack[current_layer].size - 1))->tag;
            if (AssertTagCloseStartGrammar(NU_Token_vector, i, openTag) != 0)
            {
                sHashmap_Free(&image_filepath_to_handle_hmap);
                return -1; // Failure
            }

            current_layer--; // Move one layer up (towards root)
            i+=3; // Increment token index
            continue;
        }

        // -----------------------------------------------------
        // Close end tag -> tag self closes -> move up one layer
        // -----------------------------------------------------
        if (NU_Token == CLOSE_END_TAG)
        {
            current_layer--; // Move one layer up (towards root)
            i+=1; // Increment token index
            continue;
        }

        // --------------------
        // Text content -------
        // --------------------
        if (NU_Token == TEXT_CONTENT)
        {
            struct Text_Ref* text_ref = (struct Text_Ref*) Vector_Get(&ui_tree->text_arena.text_refs, text_content_ref_index);
            text_ref->node_ID = current_node->ID;
            current_node->text_ref_index = text_content_ref_index;
            text_content_ref_index += 1;

            i+=1; // Increment token index
            continue;
        }

        // ---------------------------------------
        // Property tag -> assign property to node
        // ---------------------------------------
        if (NU_Is_Token_Property(NU_Token))
        {
            if (AssertPropertyGrammar(NU_Token_vector, i) == 0)
            {
                current_property_text = Vector_Get(ptext_ref_vector, property_text_index);
                property_text_index += 1;
                char c = src_buffer[current_property_text->src_index];
                char* ptext = &src_buffer[current_property_text->src_index];
                src_buffer[current_property_text->src_index + current_property_text->char_count] = '\0';

                // Get the property value text
                switch (NU_Token)
                {
                    // Set id
                    case ID_PROPERTY:
                        current_node->id = String_Set_Add(&ui_tree->id_string_set, ptext);
                        break;

                    // Set class
                    case CLASS_PROPERTY:
                        current_node->class = String_Set_Add(&ui_tree->class_string_set, ptext);
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
                        if (current_property_text->char_count == 4 && memcmp(ptext, "left", 4)) {
                            current_node->inline_style_flags |= 1 << 11;
                            current_node->horizontal_alignment = 0;
                        } else if (current_property_text->char_count == 6 && memcmp(ptext, "center", 6)) {
                            current_node->inline_style_flags |= 1 << 11;
                            current_node->horizontal_alignment = 1;
                        } else if (current_property_text->char_count == 5 && memcmp(ptext, "right", 5)) {
                            current_node->inline_style_flags |= 1 << 11;
                            current_node->horizontal_alignment = 2;
                        }
                        break;

                    // Set vertical alignment
                    case ALIGN_V_PROPERTY:
                        if (current_property_text->char_count == 3 && memcmp(ptext, "top", 3)) {
                            current_node->inline_style_flags |= 1 << 12;
                            current_node->vertical_alignment = 0;
                        } else if (current_property_text->char_count == 6 && memcmp(ptext, "center", 6)) {
                            current_node->inline_style_flags |= 1 << 12;
                            current_node->vertical_alignment = 2;
                        } else if (current_property_text->char_count == 6 && memcmp(ptext, "bottom", 6)) {
                            current_node->inline_style_flags |= 1 << 12;
                            current_node->vertical_alignment = 1;
                        }
                        break;

                    // Set background colour
                    case BACKGROUND_COLOUR_PROPERTY:
                        struct RGB rgb;
                        if (Parse_Hexcode(ptext, current_property_text->char_count, &rgb)) {
                            current_node->inline_style_flags |= 1 << 13;
                            current_node->background_r = rgb.r;
                            current_node->background_g = rgb.g;
                            current_node->background_b = rgb.b;
                        }
                        break;

                    // Set border colour
                    case BORDER_COLOUR_PROPERTY:
                        if (Parse_Hexcode(ptext, current_property_text->char_count, &rgb)) {
                            current_node->inline_style_flags |= 1 << 14;
                            current_node->border_r = rgb.r;
                            current_node->border_g = rgb.g;
                            current_node->border_b = rgb.b;
                        }
                        break;

                    // Set border width
                    case BORDER_WIDTH_PROPERTY:
                        uint8_t property_uint8;
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 15) | (1 << 16) | (1 << 17) | (1 << 18);
                            current_node->border_top = property_uint8;
                            current_node->border_bottom = property_uint8;
                            current_node->border_left = property_uint8;
                            current_node->border_right = property_uint8;
                        }
                        break;
                    case BORDER_TOP_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= 1 << 15;
                            current_node->border_top = property_uint8;
                        }
                        break;
                    case BORDER_BOTTOM_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= 1 << 16;
                            current_node->border_bottom = property_uint8;
                        }
                        break;
                    case BORDER_LEFT_WIDTH_PROPERTY:

                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= 1 << 17;
                            current_node->border_left = property_uint8;
                        }
                        break;
                    case BORDER_RIGHT_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= 1 << 18;
                            current_node->border_right = property_uint8;
                        }
                        break;

                    // Set border radii
                    case BORDER_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 19) | (1 << 20) | (1 << 21) | (1 << 22);
                            current_node->border_radius_tl = property_uint8;
                            current_node->border_radius_tr = property_uint8;
                            current_node->border_radius_bl = property_uint8;
                            current_node->border_radius_br = property_uint8;
                        }
                        break;
                    case BORDER_TOP_LEFT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 19);
                            current_node->border_radius_tl = property_uint8;
                        }
                        break;
                    case BORDER_TOP_RIGHT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 20);
                            current_node->border_radius_tr = property_uint8;
                        }
                        break;
                    case BORDER_BOTTOM_LEFT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 21);
                            current_node->border_radius_bl = property_uint8;
                        }
                        break;
                    case BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 22);
                            current_node->border_radius_br = property_uint8;
                        }
                        break;

                    // Set padding
                    case PADDING_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 23) | (1 << 24) | (1 << 25) | (1 << 26);
                            current_node->pad_top    = property_uint8;
                            current_node->pad_bottom = property_uint8;
                            current_node->pad_left   = property_uint8;
                            current_node->pad_right  = property_uint8;
                        }
                        break;
                    case PADDING_TOP_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 23);
                            current_node->pad_top = property_uint8;
                        }
                        break;
                    case PADDING_BOTTOM_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 24);
                            current_node->pad_bottom = property_uint8;
                        }
                        break;
                    case PADDING_LEFT_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 25);
                            current_node->pad_left = property_uint8;
                        }
                        break;
                    case PADDING_RIGHT_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inline_style_flags |= (1 << 26);
                            current_node->pad_right = property_uint8;
                        }
                        break;

                    case IMAGE_SOURCE_PROPERTY:
                        void* found = sHashmap_Find(&image_filepath_to_handle_hmap, ptext);
                        if (found == NULL) {
                            GLuint image_handle = Image_Load(ptext);
                            if (image_handle) {
                                current_node->gl_image_handle = image_handle;
                                sHashmap_Add(&image_filepath_to_handle_hmap, ptext, &image_handle);
                            }
                        } 
                        else {
                            current_node->gl_image_handle = *(GLuint*)found;
                        }
                        break;

                    default:
                        break;
                }

                // Increment NU_Token
                i+=3;
                continue;
            }
            else {
                sHashmap_Free(&image_filepath_to_handle_hmap);
                return -1;
            }
        }
        i+=1; // Increment token index
    }
    sHashmap_Free(&image_filepath_to_handle_hmap);
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
    Vector_Reserve(&NU_Token_vector, sizeof(enum NU_Token), 25000); // reserve ~100KB
    Vector_Reserve(&ptext_ref_vector, sizeof(struct Property_Text_Ref), 25000); // reserve ~225KB

    // Init UI tree layers -> reserve 100 nodes per stack layer = 384KB
    Vector_Reserve(&ui_tree->tree_stack[0], sizeof(struct Node), 1); // 1 root element
    for (int i=1; i<MAX_TREE_DEPTH; i++) {
        Vector_Reserve(&ui_tree->tree_stack[i], sizeof(struct Node), 100);
    }

    // Tokenise the file source
    NU_Tokenise(src_buffer, src_length, &NU_Token_vector, &ptext_ref_vector, &ui_tree->text_arena);

    // Generate UI tree
    timer_start();
    if (NU_Generate_Tree(src_buffer, src_length, ui_tree, &NU_Token_vector, &ptext_ref_vector) != 0) return -1; // Failure
    timer_stop();

    // Free token and property text reference memory
    Vector_Free(&NU_Token_vector);
    Vector_Free(&ptext_ref_vector);
    return 0; // Success
}
// Public Functions ------------- //