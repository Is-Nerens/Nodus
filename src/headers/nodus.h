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
    u32 hovered_node;
    u32 mouse_down_node;
    u32 scroll_hovered_node;
    u32 scroll_mouse_down_node;
    float mouse_down_global_x;
    float mouse_down_global_y;
    float v_scroll_thumb_grab_offset;
    bool running;
    bool awaiting_redraw;

    // styles
    Vector stylesheets;
    struct NU_Stylesheet* stylesheet;
    SDL_GLContext gl_ctx;
    SDL_Window* hovered_window;

    // events
    Hashmap on_click_events;
    Hashmap on_changed_events;
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

void NU_Add_Canvas_Context(u32 canvas_node_handle)
{
    NU_Canvas_Context ctx;
    Vertex_RGB_List_Init(&ctx.vertices, 512);
    Index_List_Init(&ctx.indices, 1024);
    HashmapSet(&__NGUI.canvas_contexts, &canvas_node_handle, &ctx);
}

inline NodeP* NODE_P(u32 nodeHandle)
{
    NodeP* nodeP = NodeTableGet(&__NGUI.tree.table, nodeHandle);
    if (nodeP == NULL) return NULL;
    return nodeP; 
}

inline Node* NODE(u32 nodeHandle)
{
    NodeP* nodeP = NodeTableGet(&__NGUI.tree.table, nodeHandle);
    if (nodeP == NULL) return NULL;
    return &nodeP->node; 
}

inline u32 PARENT(u32 nodeHandle)
{
    NodeP* nodeP = NodeTableGet(&__NGUI.tree.table, nodeHandle);
    if (nodeP == NULL) return UINT32_MAX;
    return nodeP->parentHandle;
}

inline u32 CHILD(u32 nodeHandle, u32 childIndex)
{
    NodeP* nodeP = NodeTableGet(&__NGUI.tree.table, nodeHandle);
    if (nodeP == NULL || childIndex >= nodeP->childCount) return UINT32_MAX;
    NodeP* child = &__NGUI.tree.layers[nodeP->layer+1].nodeArray[nodeP->firstChildIndex + childIndex];
    return child->handle;
}

inline u32 CHILD_COUNT(u32 nodeHandle)
{
    NodeP* nodeP = NodeTableGet(&__NGUI.tree.table, nodeHandle);
    if (nodeP == NULL) return UINT32_MAX;
    return nodeP->childCount;
}

inline u32 DEPTH(u32 nodeHandle)
{
    NodeP* nodeP = NodeTableGet(&__NGUI.tree.table, nodeHandle);
    if (nodeP == NULL) return UINT32_MAX;
    return nodeP->layer;
}



#include <rendering/nu_renderer.h>
#include <rendering/canvas/nu_canvas_api.h>
#include <window/nu_window_manager.h>
#include <rendering/image/nu_image.h>
#include <stylesheet/nu_stylesheet.h>
#include <xml/nu_xml_parser.h>
#include "nu_layout.h"
#include "nu_draw.h"
#include "nu_events.h"
#include "nu_dom.h"

inline u32 CREATE_NODE(u32 parentHandle, NodeType type)
{
    if (type == WINDOW) return UINT32_MAX; // Nodus doesn't yet support window creation
    u32 nodeHandle = TreeCreateNode(&__NGUI.tree, parentHandle, type);
    NodeP* node = NodeTableGet(&__NGUI.tree.table, nodeHandle);
    NU_ApplyNodeDefaults(node);
    NU_Apply_Stylesheet_To_Node(node, __NGUI.stylesheet);
    return nodeHandle;
}

inline void DELETE_NODE(u32 nodeHandle)
{
    NodeP* nodeP = NodeTableGet(&__NGUI.tree.table, nodeHandle);
    if (nodeP == NULL) return;
    return TreeDeleteNode(&__NGUI.tree, nodeHandle, NU_DissociateNode);
}

int NU_Internal_Create_Gui(char* xml_filepath, char* css_filepath)
{
    // init SDL and GLEW
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }
    NU_WindowManagerInit(&__NGUI.winManager);
    StringArena_Init(&__NGUI.node_text_arena, 1024);
    StringsetInit(&__NGUI.class_string_set, 1024, 100);
    StringsetInit(&__NGUI.id_string_set, 1024, 100);
    StringmapInit(&__NGUI.id_node_map, sizeof(u32), 100, 1024);
    HashmapInit(&__NGUI.canvas_contexts, sizeof(u32), sizeof(NU_Canvas_Context), 4);
    Vector_Reserve(&__NGUI.stylesheets, sizeof(NU_Stylesheet), 2);

    // Events
    HashmapInit(&__NGUI.on_click_events,      sizeof(u32), sizeof(struct NU_Callback_Info), 50);
    HashmapInit(&__NGUI.on_changed_events,    sizeof(u32), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_drag_events,       sizeof(u32), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_released_events,   sizeof(u32), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_resize_events,     sizeof(u32), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.node_resize_tracking, sizeof(u32), sizeof(NU_NodeDimensions)     , 10);
    HashmapInit(&__NGUI.on_mouse_down_events, sizeof(u32), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_mouse_up_events,   sizeof(u32), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_mouse_move_events, sizeof(u32), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_mouse_in_events,   sizeof(u32), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_mouse_out_events,  sizeof(u32), sizeof(struct NU_Callback_Info), 10);

    // State
    __NGUI.hovered_node = UINT32_MAX;
    __NGUI.mouse_down_node = UINT32_MAX;
    __NGUI.scroll_hovered_node = UINT32_MAX;
    __NGUI.scroll_mouse_down_node = UINT32_MAX;
    __NGUI.stylesheet = NULL;
    __NGUI.hovered_window = NULL;
    __NGUI.running = false;
    __NGUI.awaiting_redraw = false;
    __NGUI.unblock_mutex = NULL;
    __NGUI.unblock = false;

    // Register custom render event type
    __NGUI.SDL_CUSTOM_RENDER_EVENT = SDL_RegisterEvents(1);
    if (__NGUI.SDL_CUSTOM_RENDER_EVENT == (Uint32)-1) {
        printf("Failed to register custom SDL render event! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    // Event watcher
    SDL_AddEventWatch(EventWatcher, NULL);

    // Load xml
    if (!NU_Internal_Load_XML(xml_filepath)) return 0;

    // Load css
    u32 stylesheetHandle = NU_Internal_Load_Stylesheet(css_filepath);
    if (stylesheetHandle == 0) return 0;
    if (!NU_Internal_Apply_Stylesheet(stylesheetHandle)) return 0;

    __NGUI.running = true;

    return 1; // Success
}

int NU_Internal_Running()
{
    if (__NGUI.running)
    {
        SDL_Event event;
        while(__NGUI.running) {
            SDL_LockMutex(__NGUI.unblock_mutex);
            if (__NGUI.unblock) {
                __NGUI.unblock = false;
                SDL_UnlockMutex(__NGUI.unblock_mutex);
                break;
            }
            SDL_UnlockMutex(__NGUI.unblock_mutex);
            if (SDL_WaitEventTimeout(&event, 5)) { 
                EventWatcher(NULL, &event);
            }
        }
        return 1;
    }
    else
    {
        return 0;
    }
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
    HashmapFree(&__NGUI.on_changed_events);
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