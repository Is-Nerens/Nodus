#pragma once

#include <math.h>
#include <SDL3/SDL.h>
#include <GL/glew.h>

#include "datastructures/vector.h"
#include "datastructures/string_set.h"
#include "datastructures/hashmap.h"
#include "datastructures/string_arena.h"
#include "datastructures/hashmap.h"
#include "draw/nu_draw_structures.h"
#include "text/nu_font.h"


#include "node_datastructures/nu_node.h"
#include "node_datastructures/nu_nodelist.h"
#include "node_datastructures/nu_tree.h"


typedef struct NU_Window_Draw_Lists
{
    Vector relative_node_list;
    Vector clipped_relative_node_list;
    Vector absolute_node_list;
    Vector clipped_absolute_node_list;
    Vector canvas_node_list;
    Vector clipped_canvas_node_list;
} NU_Window_Draw_Lists;


struct NU_GUI
{
    NU_Tree tree;
    Vector windows;
    Vector window_nodes;
    StringArena node_text_arena;
    String_Set class_string_set;
    String_Set id_string_set;


    Vector windows_draw_lists; // Vector of NU_Window_Draw_Lists (a meta vector?)
    Vector absolute_root_nodes;
    Hashmap node_clip_map;



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
    struct Hashmap on_mouse_down_events;
    struct Hashmap on_mouse_up_events;
    struct Hashmap on_mouse_move_events;

    // Canvas drawing contexts
    struct Hashmap canvas_contexts; // maps (canvas node handle -> context)

    Uint32 SDL_CUSTOM_RENDER_EVENT;
    Uint32 SDL_CUSTOM_UNBLOCK_LOOP_EVENT;
};

struct NU_GUI __nu_global_gui;


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


#include "./xml/nu_xml_parser.h"
#include "./stylesheet/nu_stylesheet.h"
#include "./draw/nu_draw.h"
#include "./draw/nu_canvas_draw.h"
#include "nu_window.h"
#include "nu_layout.h"
#include "nu_events.h"
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
    Vector_Reserve(&__nu_global_gui.window_nodes, sizeof(uint32_t), 8);
    StringArena_Init(&__nu_global_gui.node_text_arena, 512);
    String_Map_Init(&__nu_global_gui.id_node_map, sizeof(uint32_t), 512, 25);
    String_Set_Init(&__nu_global_gui.class_string_set, 1024, 100);
    String_Set_Init(&__nu_global_gui.id_string_set, 1024, 100);

    // Draw lists and clipping 
    Vector_Reserve(&__nu_global_gui.windows_draw_lists, sizeof(NU_Window_Draw_Lists), 8);
    Vector_Reserve(&__nu_global_gui.absolute_root_nodes, sizeof(struct Node*), 8);
    Hashmap_Init(&__nu_global_gui.node_clip_map, sizeof(uint32_t), sizeof(NU_Clip_Bounds), 16);

    // Events
    Hashmap_Init(&__nu_global_gui.on_click_events,      sizeof(uint32_t), sizeof(struct NU_Callback_Info), 25);
    Hashmap_Init(&__nu_global_gui.on_changed_events,    sizeof(uint32_t), sizeof(struct NU_Callback_Info), 25);
    Hashmap_Init(&__nu_global_gui.on_drag_events,       sizeof(uint32_t), sizeof(struct NU_Callback_Info), 25);
    Hashmap_Init(&__nu_global_gui.on_released_events,   sizeof(uint32_t), sizeof(struct NU_Callback_Info), 25);
    Hashmap_Init(&__nu_global_gui.on_resize_events,     sizeof(uint32_t), sizeof(struct NU_Callback_Info), 25);
    Hashmap_Init(&__nu_global_gui.node_resize_tracking, sizeof(uint32_t), sizeof(NU_Node_Dimensions)     , 25);
    Hashmap_Init(&__nu_global_gui.on_mouse_down_events, sizeof(uint32_t), sizeof(struct NU_Callback_Info), 10);
    Hashmap_Init(&__nu_global_gui.on_mouse_up_events,   sizeof(uint32_t), sizeof(struct NU_Callback_Info), 10);
    Hashmap_Init(&__nu_global_gui.on_mouse_move_events,  sizeof(uint32_t), sizeof(struct NU_Callback_Info), 10);

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
    Vector_Free(&__nu_global_gui.windows_draw_lists);
    Vector_Free(&__nu_global_gui.absolute_root_nodes);
    Hashmap_Free(&__nu_global_gui.node_clip_map);
    Hashmap_Free(&__nu_global_gui.on_click_events);
    Hashmap_Free(&__nu_global_gui.on_changed_events);
    Hashmap_Free(&__nu_global_gui.on_drag_events);
    Hashmap_Free(&__nu_global_gui.on_released_events);
    Hashmap_Free(&__nu_global_gui.on_resize_events);
    Hashmap_Free(&__nu_global_gui.node_resize_tracking);
    Hashmap_Free(&__nu_global_gui.on_mouse_down_events);
    Hashmap_Free(&__nu_global_gui.on_mouse_up_events);
    Hashmap_Free(&__nu_global_gui.on_mouse_move_events);
    Hashmap_Free(&__nu_global_gui.canvas_contexts);
    SDL_Quit();
}
