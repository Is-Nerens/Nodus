#pragma once

// Library includes
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <vulkan/vulkan.h>
#include <math.h>
#include <stdbool.h>
#include <ft2build.h>
#include <stdio.h>
#include <stdlib.h>
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H

// Platform macros
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_MACOS
#elif defined(__linux__) || defined(__unix__)
    #define PLATFORM_LINUX
#else
    #error "Unsupported platform"
#endif

// MacOS MoltenVK portability
#ifdef PLATFORM_MACOS
    #define NU_VK_REQUIRED_INSTANCE_EXTENSIONS \
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
    #define NU_VK_INSTANCE_FLAGS \
        VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
    #define NU_VK_REQUIRED_DEVICE_EXTENSIONS \
        VK_KHR_SWAPCHAIN_EXTENSION_NAME, \
        "VK_KHR_portability_subset"
#else
    #define NU_VK_REQUIRED_INSTANCE_EXTENSIONS
    #define NU_VK_INSTANCE_FLAGS 0
    #define NU_VK_REQUIRED_DEVICE_EXTENSIONS \
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
#endif

// Math compatability macros
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

// Core definitions
typedef void (*NU_Callback)(void* args);
#define NU_EVENT_TIMER 1

// Nodus includes
#include <utils/nu_int.h>
#include <utils/nu_performance.h>
#include <utils/nu_threadpool.h>
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
#include <errors/nu_error.h>
#include <tree/nu_tree.h>
#include <graphics/rendering/nu_renderer_structures.h>
#include <graphics/text/nu_text.h>
#include <tree/input_text/nu_input_text.h>
#include <tree/nu_nodelist.h>
#include <graphics/display/nu_display_structs.h>
#include <templates/stylesheet/nu_stylesheet_structs.h>
#include <graphics/rendering/image/nu_image.h>
#include <events/nu_event_defs.h>
#include <core/nu_timer.h>
#include <core/nu_widget.h>

// Global struct struct
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

// Global gui instance
struct NU_GUI GUI;

// --------------------------------------------------
// --- Includes That Require Access To Global GUI ---
// --------------------------------------------------
#include <graphics/rendering/nu_renderer.h>
#include <graphics/rendering/nu_renderer_opengl.h>
#include <graphics/display/nu_display_manager.h>
#include <templates/stylesheet/nu_stylesheet.h>
#include <graphics/rendering/canvas/nu_canvas_api.h>
#include <templates/xml/nu_xml.h>
#include <core/nu_layout.h>
#include <tree/input_text/nu_input_text.h>
#include <core/nu_draw.h>
#include <core/nu_mouse_detection.h>
#include <events/nu_events.h>
