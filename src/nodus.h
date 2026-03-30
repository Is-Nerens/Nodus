#pragma once

// -------------------------
// --- External Includes ---
// -------------------------
#include <math.h>
#include <SDL3/SDL.h>
#include <GL/glew.h>

// ----------------------
// --- Nodus Includes ---
// ----------------------
#include <utils/nu_int.h>
#include <datastructures/vector.h>
#include <datastructures/string.h>
#include <datastructures/container.h>
#include <datastructures/stringset.h>
#include <datastructures/hashmap.h>
#include <datastructures/set.h>
#include <datastructures/stringmap.h>
#include <datastructures/linear_stringmap.h>
#include <datastructures/string_arena.h>
#include <datastructures/hashmap.h>
#include <text/nu_font.h>
#include <rendering/nu_renderer_structures.h>
#include <tree/nu_node.h>
#include <tree/nu_tree.h>
#include <tree/nu_nodelist.h>
#include <window/nu_window_manager_structs.h>
#include <rendering/nu_renderer.h>

struct NU_GUI
{
    Tree tree;
    NU_WindowManager winManager;
    StringArena nodeTextArena;
    Stringset class_string_set;
    Stringset id_string_set;
    Stringmap id_node_map;
    Container canvasContexts;

    // Pseudo nodes
    NodeP* hovered_node;
    NodeP* prev_hovered_node;
    NodeP* mouse_down_node;
    NodeP* prev_mouse_down_node;
    NodeP* focused_node;
    NodeP* prev_focused_node;

    // Scroll state
    NodeP* scroll_hovered_node;
    NodeP* scroll_mouse_down_node;
    float v_scroll_thumb_grab_offset;

    // Mouse position state
    float mouseDownGlobalX;
    float mouseDownGlobalY;

    // States
    bool running;
    bool awaiting_redraw;
    bool recalculate_mouse_hover;

    // styles
    Vector stylesheets;
    struct NU_Stylesheet* stylesheet;
    SDL_GLContext gl_ctx;

    // Events
    Hashmap on_click_events;
    Hashmap on_input_changed_events;
    Hashmap on_drag_events;
    Hashmap on_released_events;
    Hashmap on_resize_events;
    Hashmap node_resize_tracking; 
    Hashmap on_mouse_down_events;
    Hashmap on_mouse_up_events;
    Hashmap on_mouse_down_outside_events;
    Hashmap on_mouse_move_events;
    Hashmap on_mouse_in_events;
    Hashmap on_mouse_out_events;
    Hashmap on_mouse_wheel_events;
    Set deletedNodesWithRegisteredEvents;
    Uint32 SDL_CUSTOM_RENDER_EVENT;

    // Cursors
    SDL_Cursor* cursorDefault;
    SDL_Cursor* cursorPointer;
    SDL_Cursor* cursorText;
    SDL_Cursor* cursorWait;
    SDL_Cursor* cursorCrosshair;
    SDL_Cursor* cursorMove;
    SDL_Cursor* cursorNsResize;
    SDL_Cursor* cursorEwResize;
    SDL_Cursor* cursorNwseResize;
    SDL_Cursor* cursorNeswResize;

    // Layout and draw datastructures
    BreadthFirstSearch bfs;
    ReverseBreadthFirstSearch rbfs;
    Vector layoutScrollAutoNodes;
    Vertex_RGB_List borderRectVertices;
    Index_List borderRectIndices;
};

// ---------------------------
// --- Global GUI Instance ---
// ---------------------------
struct NU_GUI GUI;

// --------------------------------------------------
// --- Includes That Require Access To Global GUI ---
// --------------------------------------------------
#include <window/nu_window_manager.h>
#include <rendering/image/nu_image.h>
#include <templates/stylesheet/nu_stylesheet.h>
#include <rendering/canvas/nu_canvas_api.h>
#include <templates/xml/nu_xml.h>
#include <nu_layout.h>
#include <input_text/nu_input_text.h>
#include <nu_draw.h>
#include <events/nu_event_structs.h>
#include <nu_mouse_detection.h>
#include <events/nu_events.h>
#include <nu_dom.h>

void NU_Internal_Quit()
{
    TreeFree(&GUI.tree);
    NU_WindowManagerFree(&GUI.winManager);
    StringmapFree(&GUI.id_node_map);
    StringsetFree(&GUI.class_string_set);
    StringsetFree(&GUI.id_string_set);
    StringArena_Free(&GUI.nodeTextArena);
    for (u32 i=0; i<GUI.stylesheets.size; i++) 
    {
        NU_Stylesheet* stylesheet = Vector_Get(&GUI.stylesheets, i);
        NU_Stylesheet_Free(stylesheet);
    }
    Vector_Free(&GUI.stylesheets);
    HashmapFree(&GUI.on_click_events);
    HashmapFree(&GUI.on_input_changed_events);
    HashmapFree(&GUI.on_drag_events);
    HashmapFree(&GUI.on_released_events);
    HashmapFree(&GUI.on_resize_events);
    HashmapFree(&GUI.node_resize_tracking);
    HashmapFree(&GUI.on_mouse_down_events);
    HashmapFree(&GUI.on_mouse_up_events);
    HashmapFree(&GUI.on_mouse_down_outside_events);
    HashmapFree(&GUI.on_mouse_move_events);
    HashmapFree(&GUI.on_mouse_in_events);
    HashmapFree(&GUI.on_mouse_out_events);
    HashmapFree(&GUI.on_mouse_wheel_events);
    Container_Free(&GUI.canvasContexts);
    Vertex_RGB_List_Free(&GUI.borderRectVertices);
    Index_List_Free(&GUI.borderRectIndices);
    BreadthFirstSearch_Free(&GUI.bfs);
    ReverseBreadthFirstSearch_Free(&GUI.rbfs);
    SetFree(&GUI.deletedNodesWithRegisteredEvents);
    SDL_Quit();
}

int NU_Internal_Create_Gui(const char* xml_filepath, const char* css_filepath)
{
    // Init SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) return 0;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_SetHint("SDL_MOUSE_FOCUS_CLICKTHROUGH", "1");

    // Init Window Manager -> create the main window (hidden)
    NU_WindowManagerInit(&GUI.winManager);

    // Init string data structures
    StringArena_Init(&GUI.nodeTextArena, 1024);
    StringsetInit(&GUI.class_string_set, 1024, 100);
    StringsetInit(&GUI.id_string_set, 1024, 100);
    StringmapInit(&GUI.id_node_map, sizeof(NodeP*), 100, 1024);

    // Init canvas context container
    GUI.canvasContexts = Container_Create(sizeof(NU_Canvas_Context));

    // Init stylesheets vector
    Vector_Reserve(&GUI.stylesheets, sizeof(NU_Stylesheet), 2);

    // Events
    HashmapInit(&GUI.on_click_events,              sizeof(Node*), sizeof(struct NU_Callback_Info), 50);
    HashmapInit(&GUI.on_input_changed_events,      sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&GUI.on_drag_events,               sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&GUI.on_released_events,           sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&GUI.on_resize_events,             sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&GUI.node_resize_tracking,         sizeof(Node*), sizeof(NU_NodeDimensions)      , 10);
    HashmapInit(&GUI.on_mouse_down_events,         sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&GUI.on_mouse_up_events,           sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&GUI.on_mouse_down_outside_events, sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&GUI.on_mouse_move_events,         sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&GUI.on_mouse_in_events,           sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&GUI.on_mouse_out_events,          sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&GUI.on_mouse_wheel_events,        sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    SetInit(&GUI.deletedNodesWithRegisteredEvents, sizeof(Node*), 16);

    // Init layout and draw datastructures
    Vector_Reserve(&GUI.layoutScrollAutoNodes, sizeof(NodeP*), 20);
    Vertex_RGB_List_Init(&GUI.borderRectVertices, 5000); 
    Index_List_Init(&GUI.borderRectIndices, 15000);

    // Cursors
    GUI.cursorDefault    = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
    GUI.cursorPointer    = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
    GUI.cursorText       = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);
    GUI.cursorWait       = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
    GUI.cursorCrosshair  = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
    GUI.cursorMove       = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE);
    GUI.cursorNsResize   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);
    GUI.cursorEwResize   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
    GUI.cursorNwseResize = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NWSE_RESIZE);
    GUI.cursorNeswResize = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NESW_RESIZE);

    // Pseudo nodes
    GUI.hovered_node = NULL;
    GUI.prev_hovered_node = NULL;
    GUI.mouse_down_node = NULL;
    GUI.prev_mouse_down_node = NULL;
    GUI.focused_node = NULL;
    GUI.prev_focused_node = NULL;

    // Scroll nodes
    GUI.scroll_hovered_node = NULL;
    GUI.scroll_mouse_down_node = NULL;

    // State
    GUI.running = false;
    GUI.awaiting_redraw = true;
    GUI.recalculate_mouse_hover = true;

    // Traversal
    GUI.bfs = BreadthFirstSearch_Create(GUI.tree.root);
    GUI.rbfs = ReverseBreadthFirstSearch_Create(GUI.tree.root);

    // Register custom render event type
    GUI.SDL_CUSTOM_RENDER_EVENT = SDL_RegisterEvents(1);
    if (GUI.SDL_CUSTOM_RENDER_EVENT == (Uint32)-1) {
        NU_Internal_Quit();
        return 0;
    }

    // Load xml
    if (!NU_Internal_Load_XML(xml_filepath)) {
        NU_Internal_Quit();
        return 0;
    }

    // Load css
    GUI.stylesheet = NULL;
    u32 stylesheetHandle = NU_Internal_Load_Stylesheet(css_filepath);
    if (stylesheetHandle == 0) {
        NU_Internal_Quit();
        return 0;
    }

    // Apply css
    if (!NU_Internal_Apply_Stylesheet(stylesheetHandle)) {
        NU_Internal_Quit();
        return 0;
    }

    NU_Layout(); // Initial layout calculation
    GUI.running = true;

    // Event watcher
    SDL_AddEventWatch(EventWatcher, NULL);

    return 1; // Success
}