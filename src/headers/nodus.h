#pragma once
#include <math.h>
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// === NODUS INCLUDES ===
#include <datastructures/vector.h>
#include <datastructures/string.h>
#include <datastructures/stringset.h>
#include <datastructures/hashmap.h>
#include <datastructures/stringmap.h>
#include <datastructures/linear_stringmap.h>
#include <datastructures/string_arena.h>
#include <datastructures/hashmap.h>
#include <rendering/text/nu_font.h>
#include <rendering/nu_renderer_structures.h>
#include <tree/nu_node.h>
#include <tree/nu_tree.h>
#include <tree/nu_nodelist.h>
#include <window/nu_window_manager_structs.h>

struct NU_GUI
{
    Tree tree;
    NU_WindowManager winManager;
    StringArena node_text_arena;
    Stringset class_string_set;
    Stringset id_string_set;
    Stringmap id_node_map;
    Hashmap canvas_contexts; 

    // state
    NodeP* hovered_node;
    NodeP* mouse_down_node;
    NodeP* scroll_hovered_node;
    NodeP* scroll_mouse_down_node;
    NodeP* focused_node;
    float mouse_down_global_x;
    float mouse_down_global_y;
    float v_scroll_thumb_grab_offset;
    bool running;
    bool awaiting_redraw;

    // styles
    Vector stylesheets;
    struct NU_Stylesheet* stylesheet;
    SDL_GLContext gl_ctx;

    // events
    Hashmap on_click_events;
    Hashmap on_input_changed_events;
    Hashmap on_drag_events;
    Hashmap on_released_events;
    Hashmap on_resize_events;
    Hashmap node_resize_tracking; 
    Hashmap on_mouse_down_events;
    Hashmap on_mouse_up_events;
    Hashmap on_mouse_move_events;
    Hashmap on_mouse_in_events;
    Hashmap on_mouse_out_events;

    Uint32 SDL_CUSTOM_RENDER_EVENT;
    SDL_Mutex* unblock_mutex;
    bool unblock;
};

// global gui instance
struct NU_GUI __NGUI;
#include <rendering/nu_renderer.h>
#include <rendering/canvas/nu_canvas_api.h>
#include <window/nu_window_manager.h>
#include <rendering/image/nu_image.h>
#include <parser/stylesheet/nu_stylesheet.h>
#include <parser/xml/nu_xml_parser.h>
#include "nu_layout.h"
#include <tree/nu_input_text.h>
#include "nu_draw.h"
#include "nu_event_defs.h"
#include "nu_dom.h"
#include "nu_events.h"

void NU_Internal_Set_Class(Node* node, char* class)
{
    node->class = NULL;
    NodeP* nodeP = NODEP_OF(node, NodeP, node);

    // Look for class in gui class string set
    char* gui_class_get = StringsetGet(&__NGUI.class_string_set, class);
    if (gui_class_get == NULL) { // Not found? Look in the stylesheet
        char* style_class_get = LinearStringsetGet(&__NGUI.stylesheet->class_string_set, class);

        // If found in the stylesheet -> add it to the gui class set
        if (style_class_get) {
            node->class = StringsetAdd(&__NGUI.class_string_set, class);
        }
    } 
    else {
        node->class = gui_class_get; 
    }

    // Update styling
    NU_Apply_Stylesheet_To_Node(nodeP, __NGUI.stylesheet);
    if (nodeP == __NGUI.scroll_mouse_down_node) {
        NU_Apply_Pseudo_Style_To_Node(nodeP, __NGUI.stylesheet, PSEUDO_PRESS);
    } else if (nodeP == __NGUI.hovered_node) {
        NU_Apply_Pseudo_Style_To_Node(nodeP, __NGUI.stylesheet, PSEUDO_HOVER);
    }

    __NGUI.awaiting_redraw = true;
}

int NU_Internal_Create_Gui(char* xml_filepath, char* css_filepath)
{
    // init SDL and GLEW
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }
    NU_WindowManagerInit(&__NGUI.winManager);
    StringArena_Init(&__NGUI.node_text_arena, 1024);
    StringsetInit(&__NGUI.class_string_set, 1024, 100);
    StringsetInit(&__NGUI.id_string_set, 1024, 100);
    StringmapInit(&__NGUI.id_node_map, sizeof(NodeP*), 100, 1024);
    HashmapInit(&__NGUI.canvas_contexts, sizeof(Node*), sizeof(NU_Canvas_Context), 4);
    Vector_Reserve(&__NGUI.stylesheets, sizeof(NU_Stylesheet), 2);

    // Events
    HashmapInit(&__NGUI.on_click_events,         sizeof(Node*), sizeof(struct NU_Callback_Info), 50);
    HashmapInit(&__NGUI.on_input_changed_events, sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_drag_events,          sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_released_events,      sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_resize_events,        sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.node_resize_tracking,    sizeof(Node*), sizeof(NU_NodeDimensions)     , 10);
    HashmapInit(&__NGUI.on_mouse_down_events,    sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_mouse_up_events,      sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_mouse_move_events,    sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_mouse_in_events,      sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_mouse_out_events,     sizeof(Node*), sizeof(struct NU_Callback_Info), 10);

    // State
    __NGUI.hovered_node = NULL;
    __NGUI.mouse_down_node = NULL;
    __NGUI.scroll_hovered_node = NULL;
    __NGUI.scroll_mouse_down_node = NULL;
    __NGUI.focused_node = NULL;
    __NGUI.stylesheet = NULL;
    __NGUI.running = false;
    __NGUI.awaiting_redraw = true;
    __NGUI.unblock_mutex = NULL;
    __NGUI.unblock = false;


    // Register custom render event type
    __NGUI.SDL_CUSTOM_RENDER_EVENT = SDL_RegisterEvents(1);
    if (__NGUI.SDL_CUSTOM_RENDER_EVENT == (Uint32)-1) {
        printf("Failed to register custom SDL render event! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    // Event watcher
    SDL_AddEventWatch(EventWatcher, NULL);

    // Load xml
    if (!NU_Internal_Load_XML(xml_filepath)) return 0;

    // Load css
    u32 stylesheetHandle = NU_Internal_Load_Stylesheet(css_filepath);
    if (stylesheetHandle == 0) return 0;
    if (!NU_Internal_Apply_Stylesheet(stylesheetHandle)) return 0;
    NU_Layout(); // Initial layout calculation
    __NGUI.running = true;

    return 1; // Success
}

int NU_Internal_Running()
{
    if (!__NGUI.running) return 0;

    SDL_Event event;

    SDL_LockMutex(__NGUI.unblock_mutex);
    bool unblock = __NGUI.unblock;
    __NGUI.unblock = false;
    SDL_UnlockMutex(__NGUI.unblock_mutex);

    // If unblock is requested, break after draining events
    if (unblock) {
        while (SDL_PollEvent(&event)) {
            EventWatcher(NULL, &event);
        }
    }

    // Wait for next event, with timeout to save CPU
    SDL_WaitEventTimeout(&event, 2);

    return 1;
}

void NU_Internal_Unblock()
{
    SDL_LockMutex(__NGUI.unblock_mutex);
    __NGUI.unblock = true;
    SDL_UnlockMutex(__NGUI.unblock_mutex);
}

void NU_Internal_Render()
{
    SDL_Event e;
    SDL_zero(e);
    e.type = __NGUI.SDL_CUSTOM_RENDER_EVENT;
    SDL_PushEvent(&e);      
}

void NU_Internal_Quit()
{
    TreeFree(&__NGUI.tree);
    NU_WindowManagerFree(&__NGUI.winManager);
    StringmapFree(&__NGUI.id_node_map);
    StringsetFree(&__NGUI.class_string_set);
    StringsetFree(&__NGUI.id_string_set);
    StringArena_Free(&__NGUI.node_text_arena);
    for (u32 i=0; i<__NGUI.stylesheets.size; i++)
    {
        NU_Stylesheet* stylesheet = Vector_Get(&__NGUI.stylesheets, i);
        NU_Stylesheet_Free(stylesheet);
    }
    Vector_Free(&__NGUI.stylesheets);
    HashmapFree(&__NGUI.on_click_events);
    HashmapFree(&__NGUI.on_input_changed_events);
    HashmapFree(&__NGUI.on_drag_events);
    HashmapFree(&__NGUI.on_released_events);
    HashmapFree(&__NGUI.on_resize_events);
    HashmapFree(&__NGUI.node_resize_tracking);
    HashmapFree(&__NGUI.on_mouse_down_events);
    HashmapFree(&__NGUI.on_mouse_up_events);
    HashmapFree(&__NGUI.on_mouse_move_events);
    HashmapFree(&__NGUI.on_mouse_in_events);
    HashmapFree(&__NGUI.on_mouse_out_events);
    HashmapFree(&__NGUI.canvas_contexts);
    SDL_Quit();
}