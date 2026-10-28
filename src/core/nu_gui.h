#pragma once

/* Batches multiple images together
 * to reduce image render data allocations
 * and draw calls
 */
typedef struct {
    Array imagesRenderData;
    int groupIndex;
} ImageRenderGroup;

struct NU_GUI
{
    // Subsystems
    Tree tree;
    ErrorSystem errorSystem;
    ThreadPool threadPool;
    DisplayManager displayManager;
    ImageResourceManager imageResourceManager;
    EventSystem eventSystem;
    Stylesheet stylesheet;

    // Datastructures
    Set deletedNodesWithRegisteredEvents;
    StringArena nodeTextArena;
    Stringset class_string_set;
    Stringset id_string_set;
    Stringmap id_node_map;
    Container canvasContexts;
    Container textInputs;
    Container timers;
    Container widgets;
    DepthFirstSearch dfs;
    BreadthFirstSearch bfs;
    ReverseBreadthFirstSearch rbfs;
    Array layoutScrollAutoNodes;

    // Render data
    Array borderRectsRenderData;
    Array perFontTextRenderData; // Array of <Array of <GlyphRenderData>>
    Array imageRenderGroups; // Array of <NuImageRenderGroup>

    // Node tracking state
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

    // Application state
    bool running;
    bool awaiting_redraw;
    bool recalculate_mouse_hover;

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

    // Timer mutex
    SDL_Mutex *timerMutex;
};
