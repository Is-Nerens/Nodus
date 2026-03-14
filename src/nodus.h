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
#include <datastructures/container.h>
#include <datastructures/stringset.h>
#include <datastructures/hashmap.h>
#include <datastructures/set.h>
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

    // state
    NodeP* prev_hovered_node;
    NodeP* hovered_node;
    NodeP* mouse_down_node;
    NodeP* scroll_hovered_node;
    NodeP* scroll_mouse_down_node;
    NodeP* focused_node;
    float mouseDownGlobalX;
    float mouseDownGlobalY;
    float v_scroll_thumb_grab_offset;
    bool running;
    bool awaiting_redraw;
    bool recalculate_mouse_hover;

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
    Hashmap on_mouse_down_outside_events;
    Hashmap on_mouse_move_events;
    Hashmap on_mouse_in_events;
    Hashmap on_mouse_out_events;
    Hashmap on_mouse_wheel_events;
    Set deletedNodesWithRegisteredEvents;
    Uint32 SDL_CUSTOM_RENDER_EVENT;

    // cursors
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

// global gui instance
struct NU_GUI __NGUI;
#include <window/nu_window_manager.h>
#include <rendering/image/nu_image.h>
#include <templates/stylesheet/nu_stylesheet.h>
#include <rendering/canvas/nu_canvas_api.h>
#include <templates/xml/nu_xml.h>
#include <nu_layout.h>
#include <tree/nu_input_text.h>
#include <nu_draw.h>
#include <nu_event_defs.h>
#include <nu_dom.h>
#include <nu_events.h>
#include <cursor.h>

void NU_Internal_Set_Class(Node* node, const char* class)
{   
    NodeP* nodeP = NODEP_OF(node);
    if (class == nodeP->class) return;

    char* prevNodeClass = nodeP->class;
    nodeP->class = NULL;

    // Look for class in gui class string set
    char* gui_class_get = StringsetGet(&__NGUI.class_string_set, class);
    if (gui_class_get == NULL) { // Not found? Look in the stylesheet
        char* style_class_get = LinearStringsetGet(&__NGUI.stylesheet->class_string_set, class);

        // If found in the stylesheet -> add it to the gui class set
        if (style_class_get) {
            nodeP->class = StringsetAdd(&__NGUI.class_string_set, class);
        }
    } 
    else {
        nodeP->class = gui_class_get; 
    }

    // Update styling
    if (prevNodeClass != NULL) NU_Apply_Default_Style_To_Node(nodeP);
    NU_Apply_Stylesheet_To_Node(nodeP, __NGUI.stylesheet);
    if (nodeP == __NGUI.scroll_mouse_down_node) {
        NU_Apply_Pseudo_Style_To_Node(nodeP, __NGUI.stylesheet, PSEUDO_PRESS);
    } else if (nodeP == __NGUI.hovered_node) {
        NU_Apply_Pseudo_Style_To_Node(nodeP, __NGUI.stylesheet, PSEUDO_HOVER);
    }

    __NGUI.awaiting_redraw = true;
}

void NU_Internal_Quit()
{
    TreeFree(&__NGUI.tree);
    NU_WindowManagerFree(&__NGUI.winManager);
    StringmapFree(&__NGUI.id_node_map);
    StringsetFree(&__NGUI.class_string_set);
    StringsetFree(&__NGUI.id_string_set);
    StringArena_Free(&__NGUI.nodeTextArena);
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
    HashmapFree(&__NGUI.on_mouse_down_outside_events);
    HashmapFree(&__NGUI.on_mouse_move_events);
    HashmapFree(&__NGUI.on_mouse_in_events);
    HashmapFree(&__NGUI.on_mouse_out_events);
    HashmapFree(&__NGUI.on_mouse_wheel_events);
    Container_Free(&__NGUI.canvasContexts);
    Vertex_RGB_List_Free(&__NGUI.borderRectVertices);
    Index_List_Free(&__NGUI.borderRectIndices);
    BreadthFirstSearch_Free(&__NGUI.bfs);
    ReverseBreadthFirstSearch_Free(&__NGUI.rbfs);
    SetFree(&__NGUI.deletedNodesWithRegisteredEvents);
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
    NU_WindowManagerInit(&__NGUI.winManager);

    // Init string data structures
    StringArena_Init(&__NGUI.nodeTextArena, 1024);
    StringsetInit(&__NGUI.class_string_set, 1024, 100);
    StringsetInit(&__NGUI.id_string_set, 1024, 100);
    StringmapInit(&__NGUI.id_node_map, sizeof(NodeP*), 100, 1024);

    // Init canvas context container
    __NGUI.canvasContexts = Container_Create(sizeof(NU_Canvas_Context));

    // Init stylesheets vector
    Vector_Reserve(&__NGUI.stylesheets, sizeof(NU_Stylesheet), 2);

    // Events
    HashmapInit(&__NGUI.on_click_events,              sizeof(Node*), sizeof(struct NU_Callback_Info), 50);
    HashmapInit(&__NGUI.on_input_changed_events,      sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_drag_events,               sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_released_events,           sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_resize_events,             sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.node_resize_tracking,         sizeof(Node*), sizeof(NU_NodeDimensions)     , 10);
    HashmapInit(&__NGUI.on_mouse_down_events,         sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_mouse_up_events,           sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_mouse_down_outside_events, sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_mouse_move_events,         sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_mouse_in_events,           sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_mouse_out_events,          sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    HashmapInit(&__NGUI.on_mouse_wheel_events,        sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    SetInit(&__NGUI.deletedNodesWithRegisteredEvents, sizeof(Node*), 16);

    // Init layout and draw datastructures
    Vector_Reserve(&__NGUI.layoutScrollAutoNodes, sizeof(NodeP*), 20);
    Vertex_RGB_List_Init(&__NGUI.borderRectVertices, 5000); 
    Index_List_Init(&__NGUI.borderRectIndices, 15000);

    // Cursors
    __NGUI.cursorDefault    = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
    __NGUI.cursorPointer    = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
    __NGUI.cursorText       = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);
    __NGUI.cursorWait       = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
    __NGUI.cursorCrosshair  = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
    __NGUI.cursorMove       = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE);
    __NGUI.cursorNsResize   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);
    __NGUI.cursorEwResize   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
    __NGUI.cursorNwseResize = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NWSE_RESIZE);
    __NGUI.cursorNeswResize = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NESW_RESIZE);

    // State
    __NGUI.prev_hovered_node = NULL;
    __NGUI.hovered_node = NULL;
    __NGUI.mouse_down_node = NULL;
    __NGUI.scroll_hovered_node = NULL;
    __NGUI.scroll_mouse_down_node = NULL;
    __NGUI.focused_node = NULL;
    __NGUI.stylesheet = NULL;
    __NGUI.running = false;
    __NGUI.awaiting_redraw = true;
    __NGUI.recalculate_mouse_hover = true;


    // Register custom render event type
    __NGUI.SDL_CUSTOM_RENDER_EVENT = SDL_RegisterEvents(1);
    if (__NGUI.SDL_CUSTOM_RENDER_EVENT == (Uint32)-1) {
        NU_Internal_Quit();
        return 0;
    }

    // Load xml
    if (!NU_Internal_Load_XML(xml_filepath)) {
        NU_Internal_Quit();
        return 0;
    }

    // Load css
    u32 stylesheetHandle = NU_Internal_Load_Stylesheet(css_filepath);
    if (stylesheetHandle == 0) return 0;
    if (!NU_Internal_Apply_Stylesheet(stylesheetHandle)) {
        NU_Internal_Quit();
        return 0;
    }

    // Traversal
    __NGUI.bfs = BreadthFirstSearch_Create(__NGUI.tree.root);
    __NGUI.rbfs = ReverseBreadthFirstSearch_Create(__NGUI.tree.root);


    NU_Layout(); // Initial layout calculation
    __NGUI.running = true;

    // Event watcher
    SDL_AddEventWatch(EventWatcher, NULL);

    return 1; // Success
}

int NU_Internal_Running()
{
    if (!__NGUI.running) return 0;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {}

    if (__NGUI.awaiting_redraw) 
    {
        NU_Layout();
        if (__NGUI.recalculate_mouse_hover) NU_Mouse_Hover();
        NU_Draw();
        CheckForResizeEvents();
    }

    // Wait for next event, with timeout to save CPU
    SDL_WaitEventTimeout(&event, GetFrametime());

    return 1;
}

void NU_Internal_Render()
{
    SDL_Event e;
    SDL_zero(e);
    e.type = __NGUI.SDL_CUSTOM_RENDER_EVENT;
    SDL_PushEvent(&e);      
}