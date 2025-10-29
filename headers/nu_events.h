#pragma once

#include <SDL3/SDL.h>

#define NU_EVENT_FLAG_ON_CLICK       0x01        // 0b00000001
#define NU_EVENT_FLAG_ON_CHANGED     0x02        // 0b00000010
#define NU_EVENT_FLAG_ON_DRAG        0x04        // 0b00000100
#define NU_EVENT_FLAG_ON_RELEASED    0x08        // 0b00001000
#define NU_EVENT_FLAG_ON_RESIZE      0x10        // 0b00010000



void NU_Internal_Register_Event(uint32_t node_handle, void* args, NU_Callback callback, enum NU_Event event)
{
    struct Node* node = NODE(node_handle);
    struct NU_Callback_Info cb_info = { node_handle, args, callback };
    
    switch (event) {
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
    }
}





// -------------------------------------------------------
// --- Function is triggered when SDL detects an event ---
// -------------------------------------------------------
bool EventWatcher(void* data, SDL_Event* event) 
{
    bool draw = false;

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
        struct Node* hovered_node = __nu_global_gui.hovered_node;
        NU_Mouse_Hover();

        // Get golbal mouse coordinates
        float mouse_x_global, mouse_y_global;
        SDL_GetGlobalMouseState(&mouse_x_global, &mouse_y_global);


        if (__nu_global_gui.scroll_mouse_down_node) { // Is dragging scrollbar
            struct Node* node = __nu_global_gui.scroll_mouse_down_node;
            NU_Layer* child_layer = &__nu_global_gui.tree.layers[node->layer + 1];

            // Get relative mouse coords within window
            int win_x, win_y; 
            SDL_GetWindowPosition(__nu_global_gui.scroll_mouse_down_node->window, &win_x, &win_y);
            float mouse_y_local = mouse_y_global - win_y;

            // Calculate track_height thumb_height drag_dist
            float track_height = node->height - node->border_top - node->border_bottom;
            float inner_height_w_pad = track_height - node->pad_top - node->pad_bottom;
            float inner_proportion_of_content_height = inner_height_w_pad / node->content_height;
            float thumb_height = inner_proportion_of_content_height * track_height;
            float track_top_y = node->y + node->border_top;
            float drag_dist = (mouse_y_local - __nu_global_gui.v_scroll_thumb_grab_offset) - track_top_y;

            // Apply scroll and clamp
            node->scroll_v = drag_dist / (track_height - thumb_height);
            node->scroll_v = max(node->scroll_v, 0.0f);
            node->scroll_v = min(node->scroll_v, 1.0f);
        }
        if (hovered_node != __nu_global_gui.hovered_node || __nu_global_gui.scroll_mouse_down_node != NULL) draw = true;
    }
    // -----------------------------------------------------------------------------------
    // --- Mouse button pressed down -----------------------------------------------------
    // -----------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        // Get mouse down coordinates
        int win_x, win_y; 
        SDL_GetGlobalMouseState(&__nu_global_gui.mouse_down_global_x, &__nu_global_gui.mouse_down_global_y);
        SDL_GetWindowPosition(__nu_global_gui.hovered_node->window, &win_x, &win_y);
        float mouse_x_local = __nu_global_gui.mouse_down_global_x - win_x;
        float mouse_y_local = __nu_global_gui.mouse_down_global_y - win_y;

        // Set mouse down node
        __nu_global_gui.mouse_down_node = __nu_global_gui.hovered_node;

        // If mouse is hovered over scroll thumb -> set scroll mouse down node
        if (__nu_global_gui.scroll_hovered_node != NULL) {
            if (NU_Mouse_Over_Node_V_Scrollbar(__nu_global_gui.scroll_hovered_node, mouse_x_local, mouse_y_local)) {
                __nu_global_gui.scroll_mouse_down_node = __nu_global_gui.scroll_hovered_node;
            }
        }

        // Record scroll thumb grab offset
        if (__nu_global_gui.scroll_mouse_down_node != NULL) { 
            struct Node* node = __nu_global_gui.scroll_mouse_down_node;
            float track_height = node->height - node->border_top - node->border_bottom;
            float inner_height_w_pad = track_height - node->pad_top - node->pad_bottom;
            float inner_proportion_of_content_height = inner_height_w_pad / node->content_height;
            float thumb_height = inner_proportion_of_content_height * track_height;
            float thumb_top_y = node->y + node->border_top + (node->scroll_v * (track_height - thumb_height));
            __nu_global_gui.v_scroll_thumb_grab_offset = mouse_y_local - thumb_top_y;
        }

        // Apply presses pseudo style
        if (__nu_global_gui.mouse_down_node != NULL) {
            NU_Apply_Pseudo_Style_To_Node(__nu_global_gui.hovered_node, __nu_global_gui.stylesheet, PSEUDO_PRESS);
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
        __nu_global_gui.scroll_mouse_down_node = NULL;

        // If there is a pressed node
        if (__nu_global_gui.mouse_down_node != NULL)
        {   
            // If the mouse is hovering over pressed node
            if (__nu_global_gui.mouse_down_node == __nu_global_gui.hovered_node) 
            { 
                // Apply hover style
                NU_Apply_Pseudo_Style_To_Node(__nu_global_gui.mouse_down_node, __nu_global_gui.stylesheet, PSEUDO_HOVER);

                // If there is a click event assigned to the pressed node
                if (__nu_global_gui.hovered_node->event_flags & NU_EVENT_FLAG_ON_CLICK) 
                {
                    void* found_cb = Hashmap_Get(&__nu_global_gui.on_click_events, &__nu_global_gui.hovered_node->handle);
                    if (found_cb != NULL) {
                        struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
                        cb_info->callback(cb_info->handle, cb_info->args);
                    }
                }

                __nu_global_gui.awaiting_redraw = true;
            }
            // If the mouse is released over something other than the pressed node -> revert pressed node to default style
            else { 
                NU_Apply_Stylesheet_To_Node(__nu_global_gui.mouse_down_node, __nu_global_gui.stylesheet);
                __nu_global_gui.awaiting_redraw = true;
            }

            // There is no longer a pressed node
            __nu_global_gui.mouse_down_node = NULL;
        }
    }
    else if (event->type == SDL_EVENT_MOUSE_WHEEL)
    {
        struct Node* node = __nu_global_gui.scroll_hovered_node;
        if (node != NULL) 
        {
            float track_height = node->height - node->border_top - node->border_bottom;
            float inner_height_w_pad = track_height - node->pad_top - node->pad_bottom;
            float inner_proportion_of_content_height = inner_height_w_pad / node->content_height;
            float thumb_height = inner_proportion_of_content_height * track_height;

            node->scroll_v -= event->wheel.y * (track_height / node->content_height) * 0.2f;
            node->scroll_v = max(node->scroll_v, 0.0f);
            node->scroll_v = min(node->scroll_v, 1.0f); 
        }
    }



    if (__nu_global_gui.awaiting_redraw) 
    {
        NU_Reflow();
        NU_Draw();
    }


    // Check for resize events
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
                cb_info->callback(cb_info->handle, cb_info->args);

                continue;
            }
            
            // Init dims
            if (dims->width == -1.0f) {
                dims->width = node->width;
                dims->height = node->height;
            }
        }
    }


    return true;
}