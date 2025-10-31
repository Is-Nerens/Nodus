#pragma once

#include <math.h>
#include <SDL3/SDL.h>
#include <GL/glew.h>

#include "datastructures/vector.h"
#include "datastructures/string_set.h"
#include "datastructures/hashmap.h"
#include "datastructures/string_arena.h"
#include "datastructures/hashmap.h"
#include "nu_draw_structures.h"
#include "nu_font.h"
#include "nu_resources.h"
#include "nu_nodelist.h"


// Definitions
#define LAYOUT_VERTICAL              0x01  // 00000001
#define GROW_HORIZONTAL              0x02  // 00000010
#define GROW_VERTICAL                0x04  // 00000100
#define OVERFLOW_VERTICAL_SCROLL     0x08  // 00001000
#define OVERFLOW_HORIZONTAL_SCROLL   0x10  // 00010000
#define HIDE_BACKGROUND              0x20  // 00100000
#define POSITION_ABSOLUTE            0x40  // 01000000
#define HIDDEN                       0x80  // 10000000
#define MAX_TREE_DEPTH 32


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

    // -----------------------------------------------------
    // --- Tracks which styles were applied in xml ---------
    // -----------------------------------------------------
    uint64_t inline_style_flags;

    // ------------------------------
    // --- Tree information ---------
    // ------------------------------
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


    // ------------------------------
    // --- Styling ------------------
    // ------------------------------
    enum Tag tag;
    GLuint gl_image_handle;
    float x, y, width, height, preferred_width, preferred_height;
    float min_width, max_width, min_height, max_height;
    float gap, content_width, content_height, scroll_x, scroll_v;
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

    // ------------------------------
    // --- Event Information --------
    // ------------------------------
    char event_flags;
};

#include "nu_node_table.h"
#include "nu_layer.h"

typedef struct NU_Node_Dimensions
{
    float width, height;
} NU_Node_Dimensions;

struct NU_GUI
{
    NU_Tree tree;
    struct Vector windows;
    struct Vector window_nodes;
    StringArena node_text_arena;

    String_Set class_string_set;
    String_Set id_string_set;
    uint32_t hovered_node;
    uint32_t mouse_down_node;
    uint32_t scroll_hovered_node;
    uint32_t scroll_mouse_down_node;
    float mouse_down_global_x;
    float mouse_down_global_y;
    float v_scroll_thumb_grab_offset;
    uint16_t deepest_layer;

    // Status
    bool running;
    bool awaiting_redraw;

    // Styles
    struct NU_Stylesheet* stylesheet;
    SDL_GLContext gl_ctx;
    SDL_Window* hovered_window;

    // Id -> node mapping
    String_Map id_node_map;

    // Event callbacks
    struct Hashmap on_click_events;
    struct Hashmap on_changed_events;
    struct Hashmap on_drag_events;
    struct Hashmap on_released_events;
    struct Hashmap on_resize_events;

    struct Hashmap node_resize_tracking; // Stores the current dimensions of nodes with resize events

    // Canvas drawing contexts
    struct Hashmap canvas_contexts; // maps (canvas node handle -> context)

    Uint32 SDL_CUSTOM_RENDER_EVENT;
    Uint32 SDL_CUSTOM_UNBLOCK_LOOP_EVENT;
};

struct NU_GUI __nu_global_gui;

enum NU_Event
{
    NU_EVENT_ON_CLICK,
    NU_EVENT_ON_CHANGED,
    NU_EVENT_ON_DRAG,
    NU_EVENT_ON_RELEASED,
    NU_EVENT_ON_RESIZE
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

void NU_Add_Canvas_Context(uint32_t canvas_node_handle)
{
    NU_Canvas_Context ctx;
    Vertex_RGB_List_Init(&ctx.vertices, 512);
    Index_List_Init(&ctx.indices, 1024);
    Hashmap_Set(&__nu_global_gui.canvas_contexts, &canvas_node_handle, &ctx);
}


#include "nu_window.h"
#include "nu_xml_parser.h"
#include "nu_style_parser.h"
#include "nu_layout.h"
#include "nu_events.h"
#include "nu_draw.h"
#include "nu_canvas_draw.h"
#include "nu_dom.h"

int NU_Internal_Init()
{
    // Check if SDL initialised
    if (!SDL_Init(SDL_INIT_VIDEO)) 
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    Vector_Reserve(&__nu_global_gui.windows, sizeof(SDL_Window*), 8);
    Vector_Reserve(&__nu_global_gui.window_nodes, sizeof(struct Node*), 8);
    StringArena_Init(&__nu_global_gui.node_text_arena, 512);
    String_Map_Init(&__nu_global_gui.id_node_map, sizeof(uint32_t), 512, 25);

    // Events
    Hashmap_Init(&__nu_global_gui.on_click_events,    sizeof(uint32_t), sizeof(struct NU_Callback_Info), 25);
    Hashmap_Init(&__nu_global_gui.on_changed_events,  sizeof(uint32_t), sizeof(struct NU_Callback_Info), 25);
    Hashmap_Init(&__nu_global_gui.on_drag_events,     sizeof(uint32_t), sizeof(struct NU_Callback_Info), 25);
    Hashmap_Init(&__nu_global_gui.on_released_events, sizeof(uint32_t), sizeof(struct NU_Callback_Info), 25);
    Hashmap_Init(&__nu_global_gui.on_resize_events,   sizeof(uint32_t), sizeof(struct NU_Callback_Info), 25);

    Hashmap_Init(&__nu_global_gui.node_resize_tracking, sizeof(uint32_t), sizeof(NU_Node_Dimensions), 25);

    String_Set_Init(&__nu_global_gui.class_string_set, 1024, 100);
    String_Set_Init(&__nu_global_gui.id_string_set, 1024, 100);

    // Canvas drawing contexts and flag empty
    Hashmap_Init(&__nu_global_gui.canvas_contexts, sizeof(uint32_t), sizeof(NU_Canvas_Context), 4);

    __nu_global_gui.hovered_node = UINT32_MAX;
    __nu_global_gui.mouse_down_node = UINT32_MAX;
    __nu_global_gui.scroll_hovered_node = UINT32_MAX;
    __nu_global_gui.scroll_mouse_down_node = UINT32_MAX;
    __nu_global_gui.deepest_layer = 0;
    __nu_global_gui.stylesheet = NULL;
    __nu_global_gui.hovered_window = NULL;
    __nu_global_gui.running = true;
    __nu_global_gui.awaiting_redraw = true;


    // Register custom render event type
    __nu_global_gui.SDL_CUSTOM_RENDER_EVENT = SDL_RegisterEvents(1);
    if (__nu_global_gui.SDL_CUSTOM_RENDER_EVENT == (Uint32)-1) {
        printf("Failed to register custom SDL render event! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    // Register unblock event type
    __nu_global_gui.SDL_CUSTOM_UNBLOCK_LOOP_EVENT = SDL_RegisterEvents(1);
    if (__nu_global_gui.SDL_CUSTOM_UNBLOCK_LOOP_EVENT == (Uint32)-1) {
        printf("Failed to register custom SDL unblock event! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    NU_Create_Main_Window();
    NU_Text_Renderer_Init();
    SDL_AddEventWatch(EventWatcher, NULL);
    

    return 1; // Success
}

int NU_Internal_Running()
{
    if (__nu_global_gui.running)
    {
        SDL_Event event;
        if (SDL_WaitEvent(&event)) {
            EventWatcher(NULL, &event);
        }
        return 1;
    }
    else
    {
        return 0;
    }
}

void NU_Internal_Render()
{
    SDL_Event e;
    SDL_zero(e);
    e.type = __nu_global_gui.SDL_CUSTOM_RENDER_EVENT;
    SDL_PushEvent(&e);       
}

void NU_Internal_Unblock()
{
    SDL_Event e;
    SDL_zero(e);
    e.type = __nu_global_gui.SDL_CUSTOM_UNBLOCK_LOOP_EVENT;
    SDL_PushEvent(&e);   
}

void NU_Internal_Quit()
{
    NU_Tree_Free(&__nu_global_gui.tree);
    Vector_Free(&__nu_global_gui.windows);
    Vector_Free(&__nu_global_gui.window_nodes);
    StringArena_Free(&__nu_global_gui.node_text_arena);
    String_Map_Free(&__nu_global_gui.id_node_map);
    String_Set_Free(&__nu_global_gui.class_string_set);
    String_Set_Free(&__nu_global_gui.id_string_set);

    // Events
    Hashmap_Free(&__nu_global_gui.on_click_events);
    Hashmap_Free(&__nu_global_gui.on_changed_events);
    Hashmap_Free(&__nu_global_gui.on_drag_events);
    Hashmap_Free(&__nu_global_gui.on_released_events);
    Hashmap_Free(&__nu_global_gui.on_resize_events);

    Hashmap_Free(&__nu_global_gui.node_resize_tracking);

    // Canvas drawing contexts
    Hashmap_Free(&__nu_global_gui.canvas_contexts);

    // SDL 
    SDL_Quit();
}
