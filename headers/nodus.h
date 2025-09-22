#pragma once

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>
#include <SDL3/SDL.h>
#include <GL/glew.h>

#include "datastructures/vector.h"
#include "datastructures/string_set.h"
#include "datastructures/text_arena.h"
#include "nu_font.h"
#include "nu_resources.h"


// Definitions
#define LAYOUT_HORIZONTAL            0x00        // 0b00000000
#define LAYOUT_VERTICAL              0x01        // 0b00000001
#define GROW_HORIZONTAL              0x02        // 0b00000010
#define GROW_VERTICAL                0x04        // 0b00000100
#define OVERFLOW_VERTICAL_SCROLL     0x08        // 0b00001000
#define OVERFLOW_HORIZONTAL_SCROLL   0x10        // 0b00010000
#define MAX_TREE_DEPTH 32

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

struct Node
{
    SDL_Window* window;
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
    uint8_t text_r, text_g, text_b;
    char layout_flags;
    char horizontal_alignment;
    char vertical_alignment;
};

struct NU_GUI
{
    struct Vector tree_stack[MAX_TREE_DEPTH];
    struct Vector windows;
    struct Vector window_nodes;
    struct Text_Arena text_arena;
    struct Vector font_resources;
    struct Vector font_registry;
    String_Set class_string_set;
    String_Set id_string_set;
    struct Vector hovered_nodes;
    struct Node* hovered_node;
    struct Node* mouse_down_node;
    int16_t deepest_layer;

    // Styles
    struct NU_Stylesheet* stylesheet;
    SDL_GLContext gl_ctx;
    NVGcontext* vg_ctx;
    SDL_Window* hovered_window;
};



#include "nu_window.h"


void NU_Tree_Init(struct NU_GUI* ngui)
{
    Vector_Reserve(&ngui->windows, sizeof(SDL_Window*), 8);
    Vector_Reserve(&ngui->window_nodes, sizeof(struct Node*), 8);
    Vector_Reserve(&ngui->font_resources, sizeof(struct Font_Resource), 4);
    Vector_Reserve(&ngui->text_arena.free_list, sizeof(struct Arena_Free_Element), 100);
    Vector_Reserve(&ngui->text_arena.text_refs, sizeof(struct Text_Ref), 100);
    Vector_Reserve(&ngui->text_arena.char_buffer, sizeof(char), 25000); 
    Vector_Reserve(&ngui->font_registry, sizeof(int), 8);
    Vector_Reserve(&ngui->hovered_nodes, sizeof(struct Node*), 32);
    String_Set_Init(&ngui->class_string_set, 1024, 100);
    String_Set_Init(&ngui->id_string_set, 1024, 100);
    ngui->hovered_node = NULL;
    ngui->mouse_down_node = NULL;
    ngui->deepest_layer = 0;
    ngui->stylesheet = NULL;
    ngui->hovered_window = NULL;
    NU_Create_Main_Window(ngui);
}

void NU_Load_Font(struct NU_GUI* ngui, const char* ttf_path)
{
    struct Font_Resource font;
    Load_Font_Resource(ttf_path, &font);
    Vector_Push(&ngui->font_resources, &font);

    int fontID = nvgCreateFontMem(ngui->vg_ctx, font.name, font.data, font.size, 0);
    Vector_Push(&ngui->font_registry, &fontID);
}

void NU_Tree_Cleanup(struct NU_GUI* ngui)
{
    Vector_Free(&ngui->windows);
    Vector_Free(&ngui->window_nodes);
    Vector_Free(&ngui->text_arena.free_list);
    Vector_Free(&ngui->text_arena.text_refs);
    Vector_Free(&ngui->text_arena.char_buffer);
    Vector_Free(&ngui->font_resources);
    Vector_Free(&ngui->font_registry);
    Vector_Free(&ngui->hovered_nodes);
    String_Set_Free(&ngui->class_string_set);
    String_Set_Free(&ngui->id_string_set);
    ngui->hovered_node = NULL;
    ngui->mouse_down_node = NULL;
    ngui->deepest_layer = 0;
}

#include "nu_xml_parser.h"
#include "nu_style_parser.h"
#include "nu_layout.h"
#include "nu_draw.h"
