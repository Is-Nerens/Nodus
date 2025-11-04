#pragma once

#include <SDL3/SDL.h>
#include "performance.h"

#define NU_EVENT_FLAG_ON_CLICK       0x01        // 0b00000001
#define NU_EVENT_FLAG_ON_CHANGED     0x02        // 0b00000010
#define NU_EVENT_FLAG_ON_DRAG        0x04        // 0b00000100
#define NU_EVENT_FLAG_ON_RELEASED    0x08        // 0b00001000
#define NU_EVENT_FLAG_ON_RESIZE      0x10        // 0b00010000
#define NU_EVENT_FLAG_ON_MOUSE_DOWN  0x20        // 0b00100000
#define NU_EVENT_FLAG_ON_MOUSE_UP    0x40        // 0b01000000
#define NU_EVENT_FLAG_ON_MOUSE_MOVED 0x80        // 0b10000000

enum NU_Event_Type
{
    NU_EVENT_ON_CLICK,
    NU_EVENT_ON_CHANGED,
    NU_EVENT_ON_DRAG,
    NU_EVENT_ON_RELEASED,
    NU_EVENT_ON_RESIZE,
    NU_EVENT_ON_MOUSE_DOWN,
    NU_EVENT_ON_MOUSE_UP,
    NU_EVENT_ON_MOUSE_MOVED
};

typedef struct NU_Event_Info_Mouse
{
    int mouse_x, mouse_y;
    float delta_x, delta_y;
} NU_Event_Info_Mouse;

typedef struct NU_Event
{
    uint32_t handle;
    NU_Event_Info_Mouse mouse;
} NU_Event;

typedef void (*NU_Callback)(NU_Event event, void* args);

struct NU_Callback_Info
{
    NU_Event event;
    void* args;
    NU_Callback callback;
};



void NU_Internal_Register_Event(uint32_t node_handle, void* args, NU_Callback callback, enum NU_Event_Type event_type)
{
    struct Node* node = NODE(node_handle);
    struct NU_Event event = {0}; event.handle = node_handle;
    struct NU_Callback_Info cb_info = { event, args, callback };
    
    switch (event_type) {
        case NU_EVENT_ON_CLICK:
            node->event_flags |= NU_EVENT_FLAG_ON_CLICK;
            Hashmap_Set(&__nu_global_gui.on_click_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_CHANGED:
            node->event_flags |= NU_EVENT_FLAG_ON_CHANGED;
            Hashmap_Set(&__nu_global_gui.on_changed_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_DRAG:
            node->event_flags |= NU_EVENT_FLAG_ON_DRAG;
            Hashmap_Set(&__nu_global_gui.on_drag_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_RELEASED:
            node->event_flags |= NU_EVENT_FLAG_ON_RELEASED;
            Hashmap_Set(&__nu_global_gui.on_released_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_RESIZE:
            node->event_flags |= NU_EVENT_FLAG_ON_RESIZE;
            Hashmap_Set(&__nu_global_gui.on_resize_events, &node_handle, &cb_info);
            NU_Node_Dimensions initial_dimensions = { -1.0f, -1.0f };
            Hashmap_Set(&__nu_global_gui.node_resize_tracking, &node_handle, &initial_dimensions);
            break;
        case NU_EVENT_ON_MOUSE_DOWN:
            node->event_flags |= NU_EVENT_FLAG_ON_MOUSE_DOWN;
            Hashmap_Set(&__nu_global_gui.on_mouse_down_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_UP:
            node->event_flags |= NU_EVENT_FLAG_ON_MOUSE_UP;
            Hashmap_Set(&__nu_global_gui.on_mouse_up_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_MOVED:
            node->event_flags |= NU_EVENT_FLAG_ON_MOUSE_MOVED;
            Hashmap_Set(&__nu_global_gui.on_mouse_move_events, &node_handle, &cb_info);
            break;
    }
}









// ---------------------------------
// --- Node event trigger checks ---
// ---------------------------------
void Check_For_Resizes_Events()
{
    if (__nu_global_gui.on_resize_events.item_count > 0)
    {
        Hashmap_Iterate_Begin(&__nu_global_gui.on_resize_events);
        while(Hashmap_Iterate_Continue(&__nu_global_gui.on_resize_events))
        {
            void* key; 
            void* val;
            Hashmap_Iterate_Get(&__nu_global_gui.on_resize_events, &key, &val);
            if (key == NULL || val == NULL) continue; // Error (shouldn't happen ever)
            uint32_t handle = *(uint32_t*)key; 
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;
            struct Node* node = NODE(handle);
            void* dims_get =  Hashmap_Get(&__nu_global_gui.node_resize_tracking, &handle);
            if (dims_get == NULL) continue; // Error (shouldn't happen ever)
            NU_Node_Dimensions* dims = (NU_Node_Dimensions*)dims_get;

            // Resize detected!
            if (dims->width != -1.0f && (node->width != dims->width || node->height != dims->height))
            {
                // Upate dims 
                dims->width = node->width;
                dims->height = node->height;

                // Call callback
                cb_info->callback(cb_info->event, cb_info->args);
                continue;

            }

            // Init dims
            if (dims->width == -1.0f) 
            {
                dims->width = node->width;
                dims->height = node->height;
            }
        }
    }
}


// -------------------------------------------------------
// --- Function is triggered when SDL detects an event ---
// -------------------------------------------------------
bool EventWatcher(void* data, SDL_Event* event) 
{
    // -----------------------------------------------------------------------------------
    // --- Window Closed -> main Window? close application : destroy sub window branch ---
    // -----------------------------------------------------------------------------------
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        __nu_global_gui.running = false;
    }
    // -----------------------------------------------------------------------------------
    // --- Close application -------------------------------------------------------------
    // -----------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_QUIT)
    {
        __nu_global_gui.running = false;
    }
    // -----------------------------------------------------------------------------------
    // --- Resize -> redraw --------------------------------------------------------------
    // -----------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_WINDOW_RESIZED)
    {
        __nu_global_gui.awaiting_redraw = true;
    }
    // -----------------------------------------------------------------------------------
    // --- Render -> redraw --------------------------------------------------------------
    // -----------------------------------------------------------------------------------
    else if (event->type == __nu_global_gui.SDL_CUSTOM_RENDER_EVENT)
    {
        __nu_global_gui.awaiting_redraw = true;
    }
    // -----------------------------------------------------------------------------------
    // --- Move mouse -> redraw if mouse moves off hovered node --------------------------
    // -----------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        Uint32 id = event->motion.windowID;
        __nu_global_gui.hovered_window = SDL_GetWindowFromID(id);
        NU_Mouse_Hover();

        // Get global mouse coordinates
        float mouse_x_global, mouse_y_global;
        int win_x, win_y; 
        SDL_GetGlobalMouseState(&mouse_x_global, &mouse_y_global);
        SDL_GetWindowPosition(__nu_global_gui.hovered_window, &win_x, &win_y);
        float mouse_x = mouse_x_global - win_x;
        float mouse_y = mouse_y_global - win_y;

        // If dragging scrollbar -> update node->scroll_v 
        if (__nu_global_gui.scroll_mouse_down_node != UINT32_MAX) 
        {
            struct Node* node = NODE(__nu_global_gui.scroll_mouse_down_node);
            NU_Layer* child_layer = &__nu_global_gui.tree.layers[node->layer + 1];

            // Calculate drag_dist track_height thumb_height
            float track_height = node->height - node->border_top - node->border_bottom;
            float inner_height_w_pad = track_height - node->pad_top - node->pad_bottom;
            float inner_proportion_of_content_height = inner_height_w_pad / node->content_height;
            float thumb_height = inner_proportion_of_content_height * track_height;
            float track_top_y = node->y + node->border_top;
            float drag_dist = (mouse_y - __nu_global_gui.v_scroll_thumb_grab_offset) - track_top_y;

            // Apply scroll and clamp
            node->scroll_v = drag_dist / (track_height - thumb_height);
            node->scroll_v = min(max(node->scroll_v, 0.0f), 1.0f); // Clamp to range [0,1]

            // Must redraw later
            __nu_global_gui.awaiting_redraw = true;
        }

        // Check for mouse move events
        if (__nu_global_gui.on_mouse_move_events.item_count > 0)
        {
            Hashmap_Iterate_Begin(&__nu_global_gui.on_mouse_move_events);
            while(Hashmap_Iterate_Continue(&__nu_global_gui.on_mouse_move_events))
            {
                void* key; 
                void* val;
                Hashmap_Iterate_Get(&__nu_global_gui.on_mouse_move_events, &key, &val);
                uint32_t handle = *(uint32_t*)key; 
                struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;

                // Call callback
                cb_info->event.mouse.mouse_x = mouse_x;
                cb_info->event.mouse.mouse_y = mouse_y;
                cb_info->event.mouse.delta_x = event->motion.xrel;
                cb_info->event.mouse.delta_y = event->motion.yrel;
                cb_info->callback(cb_info->event, cb_info->args);
            }
        }
    }
    // -----------------------------------------------------------------------------------
    // --- Mouse button pressed down -----------------------------------------------------
    // -----------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        // Get mouse down coordinates
        SDL_GetGlobalMouseState(&__nu_global_gui.mouse_down_global_x, &__nu_global_gui.mouse_down_global_y);

        // Set mouse down node
        __nu_global_gui.mouse_down_node = __nu_global_gui.hovered_node;

        // If mouse is hovered over scroll thumb -> set scroll mouse down node
        if (__nu_global_gui.scroll_hovered_node != UINT32_MAX) 
        {
            int win_x, win_y; 
            SDL_GetWindowPosition(NODE(__nu_global_gui.scroll_hovered_node)->window, &win_x, &win_y);
            float mouse_x = __nu_global_gui.mouse_down_global_x - win_x;
            float mouse_y = __nu_global_gui.mouse_down_global_y - win_y;
            
            if (NU_Mouse_Over_Node_V_Scrollbar(NODE(__nu_global_gui.scroll_hovered_node), mouse_x, mouse_y)) 
            {
                __nu_global_gui.scroll_mouse_down_node = __nu_global_gui.scroll_hovered_node;

                // Record scroll thumb grab offset
                struct Node* node = NODE(__nu_global_gui.scroll_mouse_down_node);
                float track_height = node->height - node->border_top - node->border_bottom;
                float inner_height_w_pad = track_height - node->pad_top - node->pad_bottom;
                float inner_proportion_of_content_height = inner_height_w_pad / node->content_height;
                float thumb_height = inner_proportion_of_content_height * track_height;
                float thumb_top_y = node->y + node->border_top + (node->scroll_v * (track_height - thumb_height));
                __nu_global_gui.v_scroll_thumb_grab_offset = mouse_y - thumb_top_y;
            }
        }

        // If there is a mouse down node
        if (__nu_global_gui.mouse_down_node != UINT32_MAX) 
        {
            // Apply presses pseudo style
            NU_Apply_Pseudo_Style_To_Node(NODE(__nu_global_gui.hovered_node), __nu_global_gui.stylesheet, PSEUDO_PRESS);

            // If node has a mouse down event
            if (NODE(__nu_global_gui.mouse_down_node)->event_flags & NU_EVENT_FLAG_ON_MOUSE_DOWN)
            {
                void* found_cb = Hashmap_Get(&__nu_global_gui.on_mouse_down_events, &__nu_global_gui.mouse_down_node);
                if (found_cb != NULL) {
                    int win_x, win_y; 
                    SDL_GetWindowPosition(NODE(__nu_global_gui.mouse_down_node)->window, &win_x, &win_y);
                    float mouse_x = __nu_global_gui.mouse_down_global_x - win_x;
                    float mouse_y = __nu_global_gui.mouse_down_global_y - win_y;
                    
                    struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
                    cb_info->event.mouse.mouse_x = mouse_x;
                    cb_info->event.mouse.mouse_y = mouse_y;
                    cb_info->callback(cb_info->event, cb_info->args);
                }
            }
        } 

        __nu_global_gui.awaiting_redraw = true;
    }
    // -----------------------------------------------------------------------------------
    // --- Focus on window -> redraw -----------------------------------------------------
    // -----------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_WINDOW_FOCUS_GAINED)
    {
        __nu_global_gui.mouse_down_node = __nu_global_gui.hovered_node;
        __nu_global_gui.awaiting_redraw = true;
    }
    // -----------------------------------------------------------------------------------
    // --- Released mouse button ---------------------------------------------------------
    // -----------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        __nu_global_gui.scroll_mouse_down_node = UINT32_MAX;

        // If there is a pressed node
        if (__nu_global_gui.mouse_down_node != UINT32_MAX)
        {   
            // If the mouse is hovering over pressed node
            if (__nu_global_gui.mouse_down_node == __nu_global_gui.hovered_node) 
            { 
                // Apply hover style
                NU_Apply_Pseudo_Style_To_Node(NODE(__nu_global_gui.mouse_down_node), __nu_global_gui.stylesheet, PSEUDO_HOVER);
                __nu_global_gui.awaiting_redraw = true;

                // Get mouse up coordinates
                int win_x, win_y; 
                SDL_GetGlobalMouseState(&__nu_global_gui.mouse_down_global_x, &__nu_global_gui.mouse_down_global_y);
                SDL_GetWindowPosition(NODE(__nu_global_gui.hovered_node)->window, &win_x, &win_y);
                float mouse_x = __nu_global_gui.mouse_down_global_x - win_x;
                float mouse_y = __nu_global_gui.mouse_down_global_y - win_y;

                // If there is a click event assigned to the pressed node
                if (NODE(__nu_global_gui.hovered_node)->event_flags & NU_EVENT_FLAG_ON_CLICK) 
                {
                    void* found_cb = Hashmap_Get(&__nu_global_gui.on_click_events, &__nu_global_gui.hovered_node);
                    if (found_cb != NULL) {
                        struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
                        cb_info->callback(cb_info->event, cb_info->args);
                    }
                }

                // If there is a mouse up event assigned to the pressed node
                if (NODE(__nu_global_gui.hovered_node)->event_flags & NU_EVENT_FLAG_ON_MOUSE_UP) 
                {
                    void* found_cb = Hashmap_Get(&__nu_global_gui.on_mouse_up_events, &__nu_global_gui.hovered_node);
                    if (found_cb != NULL) {
                        struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
                        cb_info->event.mouse.mouse_x = mouse_x;
                        cb_info->event.mouse.mouse_y = mouse_y;
                        cb_info->callback(cb_info->event, cb_info->args);
                    }
                }
            }
            // If the mouse is released over something other than the pressed node -> revert pressed node to default style
            else 
            { 
                NU_Apply_Stylesheet_To_Node(NODE(__nu_global_gui.mouse_down_node), __nu_global_gui.stylesheet);
                __nu_global_gui.awaiting_redraw = true;
            }

            // There is no longer a pressed node
            __nu_global_gui.mouse_down_node = UINT32_MAX;
        }
    }
    // -----------------------------------------------------------------------------------
    // --- Mouse scroll wheel ------------------------------------------------------------
    // -----------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_MOUSE_WHEEL)
    {
        struct Node* node = NODE(__nu_global_gui.scroll_hovered_node);
        if (node != NULL) 
        {
            float track_height = node->height - node->border_top - node->border_bottom;
            float inner_height_w_pad = track_height - node->pad_top - node->pad_bottom;
            float inner_proportion_of_content_height = inner_height_w_pad / node->content_height;
            float thumb_height = inner_proportion_of_content_height * track_height;
            node->scroll_v -= event->wheel.y * (track_height / node->content_height) * 0.2f;
            node->scroll_v = min(max(node->scroll_v, 0.0f), 1.0f); // Clamp to range [0,1]

            // Re-render and exit function
            NU_Reflow();
            NU_Mouse_Hover();
            NU_Draw();
            return true;
        }
    }


    if (__nu_global_gui.awaiting_redraw) 
    {
        NU_Reflow();
        NU_Draw();
        Check_For_Resizes_Events();
    }
    return true;
}