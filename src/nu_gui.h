#pragma once

#ifndef min
    #define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
    #define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

static inline float fmax_fast(float a, float b) {
    return a * (a > b) + b * (1 - (a > b));
}

static inline float fmin_fast(float a, float b) {
    return a * (a < b) + b * (1 - (a < b));
}

typedef void (*NU_Callback)(void* args);

#define NU_EVENT_TIMER 1

// ----------------------
// --- Nodus Includes ---
// ----------------------
#include <utils/nu_int.h>
#include <utils/performance.h>
#include <datastructures/Array.h>
#include <datastructures/string.h>
#include <datastructures/container.h>
#include <datastructures/stringset.h>
#include <datastructures/hashmap.h>
#include <datastructures/set.h>
#include <datastructures/stringmap.h>
#include <datastructures/linear_stringmap.h>
#include <datastructures/linear_stringset.h>
#include <datastructures/string_arena.h>
#include <datastructures/hashmap.h>
#include <datastructures/linalloc.h>
#include <datastructures/threadpool.h>
#include <errors/nu_error.h>
#include <tree/nu_node.h>
#include <tree/nu_tree.h>
#include <tree/nu_nodelist.h>
#include <rendering/nu_renderer_structures.h>
#include <text/nu_font.h>
#include <window/nu_window_manager_structs.h>
#include <templates/stylesheet/nu_stylesheet_structs.h>
#include <rendering/nu_renderer.h>
#include <rendering/image/nu_image.h>
#include <events/nu_event_defs.h>
#include <nu_timer.h>
#include <nu_widget.h>

struct NU_GUI
{
    ErrorSystem errorSystem;
    Tree tree;
    ThreadPool threadPool;
    WindowManager winManager;
    ImageResourceManager imageResourceManager;
    StringArena nodeTextArena;
    Stringset class_string_set;
    Stringset id_string_set;
    Stringmap id_node_map;
    Container canvasContexts;
    Container textInputs;
    Container timers;
    Container widgets;

    // Pseudo nodes
    Node* hovered_node;
    Node* prev_hovered_node;
    Node* mouse_down_node;
    Node* prev_mouse_down_node;
    Node* focused_node;
    Node* prev_focused_node;

    // Scroll state
    Node* scroll_hovered_node;
    Node* scroll_mouse_down_node;
    float v_scroll_thumb_grab_offset;

    // States
    bool running;
    bool awaiting_redraw;
    bool recalculate_mouse_hover;

    // styles
    Stylesheet stylesheet;
    SDL_GLContext gl_ctx;

    // Events
    EventSystem eventSystem;
    Set deletedNodesWithRegisteredEvents;

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
    DepthFirstSearch dfs;
    BreadthFirstSearch bfs;
    ReverseBreadthFirstSearch rbfs;
    Array layoutScrollAutoNodes;
    Array borderRectRenderDataArray;
    Array perFontTextRenderData;

    // Timer mutex
    SDL_Mutex *timerMutex;
};

// ---------------------------
// --- Global GUI Instance ---
// ---------------------------
struct NU_GUI GUI;

// --------------------------------------------------
// --- Includes That Require Access To Global GUI ---
// --------------------------------------------------
#include <window/nu_window_manager.h>
#include <templates/stylesheet/nu_stylesheet.h>
#include <rendering/canvas/nu_canvas_api.h>
#include <templates/xml/nu_xml.h>
#include <nu_layout.h>
#include <input_text/nu_input_text.h>
#include <nu_draw.h>
#include <nu_mouse_detection.h>
#include <events/nu_events.h>
