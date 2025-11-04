// -----------------------------------------------------------------------------------------
// --- | PURPOSE:      a collection of structs and methods regarding individual nodes
// --- | USED IN:      nodeus.h (and used just about everywhere)
// --- | DEPENDENCIES: <SDL3/SDL.h> <GL/glew.h>
// -----------------------------------------------------------------------------------------


#pragma once

// --- LAYOUT FLAGS ---
#define LAYOUT_VERTICAL              0x01  // 00000001
#define GROW_HORIZONTAL              0x02  // 00000010
#define GROW_VERTICAL                0x04  // 00000100
#define OVERFLOW_VERTICAL_SCROLL     0x08  // 00001000
#define OVERFLOW_HORIZONTAL_SCROLL   0x10  // 00010000
#define HIDE_BACKGROUND              0x20  // 00100000
#define POSITION_ABSOLUTE            0x40  // 01000000
#define HIDDEN                       0x80  // 10000000

enum Tag
{
    WINDOW,
    REC,
    BUTTON,
    GRID,
    CANVAS,
    IMAGE,
    TABLE,
    THEAD,
    ROW,
    NAT,
};

struct Node
{
    SDL_Window* window;
    char* class;
    char* id;
    char* text_content;

    // --- Tracks which styles were applied in xml ---
    uint64_t inline_style_flags;

    // --- Tree information ---
    uint32_t handle;
    uint32_t clipping_root_handle;
    uint16_t index;
    uint16_t parent_index;
    uint16_t first_child_index;
    uint16_t child_capacity;
    uint16_t child_count;
    uint8_t node_present;
    uint8_t layer; 
    uint8_t position_absolute;

    // --- Styling ---
    enum Tag tag;
    GLuint gl_image_handle;
    float x, y, width, height, preferred_width, preferred_height;
    float min_width, max_width, min_height, max_height;
    float gap, content_width, content_height, scroll_x, scroll_v;
    float left, right, top, bottom;
    uint8_t pad_top, pad_bottom, pad_left, pad_right;
    uint8_t border_top, border_bottom, border_left, border_right;
    uint8_t border_radius_tl, border_radius_tr, border_radius_bl, border_radius_br;
    uint8_t background_r, background_g, background_b;
    uint8_t border_r, border_g, border_b;
    uint8_t text_r, text_g, text_b;
    uint8_t font_id;
    uint8_t layout_flags;
    char horizontal_alignment;
    char vertical_alignment;
    char horizontal_text_alignment;
    char vertical_text_alignment;
    bool hide_background;

    // --- Event Information
    char event_flags;
};

typedef struct NU_Node_Dimensions
{
    float width, height;
} NU_Node_Dimensions;

typedef struct NU_Clip_Bounds 
{
    float clip_top;
    float clip_bottom;
    float clip_left;
    float clip_right;
    float tl_radius_x, tl_radius_y;
    float tr_radius_x, tr_radius_y;
    float bl_radius_x, bl_radius_y;
    float br_radius_x, br_radius_y;
} NU_Clip_Bounds;

static void NU_Apply_Node_Defaults(struct Node* node)
{
    node->window = NULL;
    node->class = NULL;
    node->id = NULL;
    node->text_content = NULL;
    node->inline_style_flags = 0;
    node->clipping_root_handle = 0;
    node->position_absolute = 0;
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
    node->scroll_x = 0.0f;
    node->scroll_v = 0.0f;
    node->left = -1.0f;
    node->right = -1.0f;
    node->top = -1.0f;
    node->bottom = -1.0f;
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
    node->font_id = 0;
    node->layout_flags = 0;
    node->horizontal_alignment = 0;
    node->vertical_alignment = 0;
    node->horizontal_text_alignment = 0;
    node->vertical_text_alignment = 1;
    node->hide_background = false;
    node->event_flags = 0;
}
