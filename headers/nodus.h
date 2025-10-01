#pragma once

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>
#include <SDL3/SDL.h>
#include <GL/glew.h>

#include "datastructures/vector.h"
#include "datastructures/string_set.h"
#include "datastructures/hashmap.h"
#include "datastructures/string_arena.h"
#include "datastructures/hashmap.h"
#include "nu_font.h"
#include "nu_resources.h"


// Definitions
#define LAYOUT_HORIZONTAL            0x00        // 0b00000000
#define LAYOUT_VERTICAL              0x01        // 0b00000001
#define GROW_HORIZONTAL              0x02        // 0b00000010
#define GROW_VERTICAL                0x04        // 0b00000100
#define OVERFLOW_VERTICAL_SCROLL     0x08        // 0b00001000
#define OVERFLOW_HORIZONTAL_SCROLL   0x16        // 0b00010000
#define HIDE_BACKGROUND              0x32        // 0b00100000
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
    char* class;
    char* id;
    char* text_content;

    // -----------------------------------------------------
    // --- Tracks which styles were applied in xml ---------
    // -----------------------------------------------------
    uint64_t inline_style_flags;

    // ------------------------------
    // --- Tree information ---------
    // ------------------------------
    uint32_t handle;
    uint16_t index;
    uint16_t parent_index;
    uint16_t first_child_index;
    uint16_t child_capacity;
    uint16_t child_count;
    uint8_t node_present;
    uint8_t layer; 


    // ------------------------------
    // --- Styling ------------------
    // ------------------------------
    enum Tag tag;
    GLuint gl_image_handle;
    float x, y, width, height, preferred_width, preferred_height;
    float min_width, max_width, min_height, max_height;
    float gap, content_width, content_height;
    uint8_t pad_top, pad_bottom, pad_left, pad_right;
    uint8_t border_top, border_bottom, border_left, border_right;
    uint8_t border_radius_tl, border_radius_tr, border_radius_bl, border_radius_br;
    uint8_t background_r, background_g, background_b;
    uint8_t border_r, border_g, border_b;
    uint8_t text_r, text_g, text_b;
    uint8_t layout_flags;
    char horizontal_alignment;
    char vertical_alignment;
    char horizontal_text_alignment;
    char vertical_text_alignment;
    bool hide_background;

    // ------------------------------
    // --- Event Information --------
    // ------------------------------
    char event_flags;
};

#include "nu_node_table.h"
#include "nu_layer.h"

struct NU_GUI
{
    NU_Tree tree;
    struct Vector windows;
    struct Vector window_nodes;
    StringArena node_text_arena;
    struct Vector font_resources;
    struct Vector font_registry;
    String_Set class_string_set;
    String_Set id_string_set;
    struct Node* hovered_node;
    struct Node* mouse_down_node;
    uint16_t deepest_layer;

    // Status
    bool awaiting_draw;
    bool running;

    // Styles
    struct NU_Stylesheet* stylesheet;
    SDL_GLContext gl_ctx;
    NVGcontext* vg_ctx;
    SDL_Window* hovered_window;

    // Id -> node mapping
    String_Map id_node_map;

    // Event callbacks
    struct Hashmap on_click_events;
    struct Hashmap on_changed_events;
    struct Hashmap on_drag_events;
    struct Hashmap on_released_events;
};

struct NU_GUI __nu_global_gui;

enum NU_Event
{
    NU_EVENT_ON_CLICK,
    NU_EVENT_ON_CHANGED,
    NU_EVENT_ON_DRAG,
    NU_EVENT_ON_RELEASED
};

typedef void (*NU_Callback)(uint32_t handle, void* args);

struct NU_Callback_Info
{
    uint32_t handle;
    void* args;
    NU_Callback callback;
};


inline struct Node* NODE(uint32_t handle)
{
    if (handle >= __nu_global_gui.tree.node_table.capacity) return NULL;
    uint32_t rem = handle & 7;                                // i % 8
    uint32_t occupancy_index = handle >> 3;                   // i / 8
    if (!(__nu_global_gui.tree.node_table.occupancy[occupancy_index] & (1u << rem))) { // Found empty
        return NULL;
    }
    return __nu_global_gui.tree.node_table.data[handle];
}


#include "nu_window.h"

void NU_Init()
{
    Vector_Reserve(&__nu_global_gui.windows, sizeof(SDL_Window*), 8);
    Vector_Reserve(&__nu_global_gui.window_nodes, sizeof(struct Node*), 8);
    Vector_Reserve(&__nu_global_gui.font_resources, sizeof(struct Font_Resource), 4);
    StringArena_Init(&__nu_global_gui.node_text_arena, 512);
    Vector_Reserve(&__nu_global_gui.font_registry, sizeof(int), 8);
    String_Map_Init(&__nu_global_gui.id_node_map, sizeof(uint32_t), 512, 25);

    // Events
    Hashmap_Init(&__nu_global_gui.on_click_events,    sizeof(uint32_t), sizeof(struct NU_Callback_Info), 25);
    Hashmap_Init(&__nu_global_gui.on_changed_events,  sizeof(uint32_t), sizeof(struct NU_Callback_Info), 25);
    Hashmap_Init(&__nu_global_gui.on_drag_events,     sizeof(uint32_t), sizeof(struct NU_Callback_Info), 25);
    Hashmap_Init(&__nu_global_gui.on_released_events, sizeof(uint32_t), sizeof(struct NU_Callback_Info), 25);

    String_Set_Init(&__nu_global_gui.class_string_set, 1024, 100);
    String_Set_Init(&__nu_global_gui.id_string_set, 1024, 100);
    __nu_global_gui.hovered_node = NULL;
    __nu_global_gui.mouse_down_node = NULL;
    __nu_global_gui.deepest_layer = 0;
    __nu_global_gui.awaiting_draw = true;
    __nu_global_gui.stylesheet = NULL;
    __nu_global_gui.hovered_window = NULL;
    NU_Create_Main_Window();

    __nu_global_gui.running = true;
}

bool NU_Running()
{
    return __nu_global_gui.running;
}

void NU_Load_Font(const char* ttf_path)
{
    struct Font_Resource font;
    Load_Font_Resource(ttf_path, &font);
    Vector_Push(&__nu_global_gui.font_resources, &font);

    int fontID = nvgCreateFontMem(__nu_global_gui.vg_ctx, font.name, font.data, font.size, 0);
    Vector_Push(&__nu_global_gui.font_registry, &fontID);
}

void NU_Quit()
{
    NU_Tree_Free(&__nu_global_gui.tree);
    Vector_Free(&__nu_global_gui.windows);
    Vector_Free(&__nu_global_gui.window_nodes);
    StringArena_Free(&__nu_global_gui.node_text_arena);
    Vector_Free(&__nu_global_gui.font_resources);
    Vector_Free(&__nu_global_gui.font_registry);
    String_Map_Free(&__nu_global_gui.id_node_map);

    // Events
    Hashmap_Free(&__nu_global_gui.on_click_events);
    Hashmap_Free(&__nu_global_gui.on_changed_events);
    Hashmap_Free(&__nu_global_gui.on_drag_events);
    Hashmap_Free(&__nu_global_gui.on_released_events);

    String_Set_Free(&__nu_global_gui.class_string_set);
    String_Set_Free(&__nu_global_gui.id_string_set);
    __nu_global_gui.hovered_node = NULL;
    __nu_global_gui.mouse_down_node = NULL;
    __nu_global_gui.deepest_layer = 0;
}

#include "nu_xml_parser.h"
#include "nu_style_parser.h"
#include "nu_layout.h"
#include "nu_events.h"
#include "nu_draw.h"
#include "nu_dom.h"