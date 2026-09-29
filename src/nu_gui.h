#pragma once

// -------------------------
// --- External Includes ---
// -------------------------
#include <math.h>
#include <stdio.h>
#include <SDL3/SDL.h>
#include <GL/glew.h>

#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_MACOS
#elif defined(__linux__) || defined(__unix__)
    #define PLATFORM_LINUX
#else
    #error "Unsupported platform"
#endif

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
#include <errors/nu_error.h>
#include <text/nu_font.h>
#include <tree/nu_node.h>
#include <tree/nu_tree.h>
#include <tree/nu_nodelist.h>
#include <rendering/nu_renderer_structures.h>
#include <window/nu_window_manager_structs.h>
#include <templates/stylesheet/nu_stylesheet_structs.h>
#include <rendering/nu_renderer.h>
#include <rendering/image/nu_image.h>
#include <events/nu_event_defs.h>
#include <nu_timer.h>

struct NU_GUI
{
    ErrorSystem errorSystem;
    Tree tree;
    WindowManager winManager;
    ImageResourceManager imageResourceManager;
    StringArena nodeTextArena;
    Stringset class_string_set;
    Stringset id_string_set;
    Stringmap id_node_map;
    Container canvasContexts;
    Container textInputs;
    Container timers;

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
    Array layoutScrollAutoNodes;
    Array borderRectRenderDataArray;
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
#include <nu_dom.h>

void NU_Internal_Quit()
{
    TreeFree(&GUI.tree);
    WindowManager_Free(&GUI.winManager);
    ImageResourceManager_Free(&GUI.imageResourceManager);
    ErrorSystem_Free(&GUI.errorSystem);
    Stringmap_Free(&GUI.id_node_map);
    Stringset_Free(&GUI.class_string_set);
    Stringset_Free(&GUI.id_string_set);
    StringArena_Free(&GUI.nodeTextArena);
    Stylesheet_Free(&GUI.stylesheet);
    Container_Free(&GUI.canvasContexts);
    Container_Free(&GUI.textInputs);
    Container_Free(&GUI.timers);
    Array_Free(&GUI.borderRectRenderDataArray);
    BreadthFirstSearch_Free(&GUI.bfs);
    ReverseBreadthFirstSearch_Free(&GUI.rbfs);
    EventSystem_Free();
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
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_SetHint("SDL_MOUSE_FOCUS_CLICKTHROUGH", "1");
    SDL_SetHint(SDL_HINT_MAC_SCROLL_MOMENTUM, "1");

    // Init Systems
    WindowManager_Init(&GUI.winManager);
    ImageResourceManager_Init(&GUI.imageResourceManager);
    ErrorSystem_Init(&GUI.errorSystem);

    // Init string data structures
    StringArena_Init(&GUI.nodeTextArena, 1024);
    Stringset_Init(&GUI.class_string_set, 1024, 100);
    Stringset_Init(&GUI.id_string_set, 1024, 100);
    Stringmap_Init(&GUI.id_node_map, sizeof(NodeP*), 128, 2048);

    // Init canvas context and text input containers
    GUI.canvasContexts = Container_Create(sizeof(NU_Canvas_Context));
    GUI.textInputs = Container_Create(sizeof(InputText));
    GUI.timers = Container_Create(sizeof(NU_Timer));

    // Init Event System (allocates memory)
    EventSystem_Init();

    // Init layout and draw datastructures
    Array_Init(&GUI.layoutScrollAutoNodes, sizeof(NodeP*), 20);
    Array_Init(&GUI.borderRectRenderDataArray, sizeof(BorderRectRenderData), 2000);

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

    // Create an image resource loader
    ImageResourceLoader imageResourceLoader;
    ImageResourceLoader_Init(&imageResourceLoader, &GUI.imageResourceManager);

    // Load xml
    if (!NU_Internal_Load_XML(xml_filepath, &imageResourceLoader)) {
        NU_Internal_Quit();
        return 0;
    }

    // Load css
    if (!Stylesheet_Create(&GUI.stylesheet, css_filepath, &imageResourceLoader)) {
        NU_Internal_Quit();
        return 0;
    }

    // Upload image to the GPU and free loader memory
    ImageResourceLoader_UploadImagesAndFree(&imageResourceLoader);

    // Apply css
    printf("applying css\n");
    timer_start();
    if (!NU_Internal_Apply_Stylesheet(&GUI.stylesheet)) {
        NU_Internal_Quit();
        return 0;
    }
    timer_stop();

    printf("calculating layout\n");
    timer_start();
    NU_Layout(); // Initial layout calculation
    timer_stop();

    // Event watcher
    SDL_AddEventWatch(EventWatcher, NULL);

    // Success
    return 1;
}
