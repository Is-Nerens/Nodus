#pragma once

#include <SDL3/SDL.h>
#include "performance.h"

#define NU_EVENT_FLAG_ON_CLICK       0x01        // 0b00000001
#define NU_EVENT_FLAG_ON_CHANGED     0x02        // 0b00000010
#define NU_EVENT_FLAG_ON_DRAG        0x04        // 0b00000100
#define NU_EVENT_FLAG_ON_RELEASED    0x08        // 0b00001000


void NU_Register_Event(uint32_t node_handle, void* args, NU_Callback callback, enum NU_Event event)
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
    }
}






// -----------------------------
// Event watcher ---------------
// -----------------------------

bool EventWatcher(void* data, SDL_Event* event) 
{
    bool draw = false;

    switch (event->type) {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            __nu_global_gui.running = false;
            break;
        case SDL_EVENT_QUIT:
            __nu_global_gui.running = false;
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            draw = true;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            Uint32 id = event->motion.windowID;
            __nu_global_gui.hovered_window = SDL_GetWindowFromID(id);
            struct Node* hovered_node = __nu_global_gui.hovered_node;
            NU_Mouse_Hover();
            if (hovered_node != __nu_global_gui.hovered_node) draw = true;
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            __nu_global_gui.mouse_down_node = __nu_global_gui.hovered_node;
            if (__nu_global_gui.mouse_down_node != NULL) {
                NU_Apply_Pseudo_Style_To_Node(__nu_global_gui.hovered_node, __nu_global_gui.stylesheet, PSEUDO_PRESS);
            }
            draw = true;
            break;
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            __nu_global_gui.mouse_down_node = __nu_global_gui.hovered_node;
            draw = true;
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (__nu_global_gui.mouse_down_node != NULL)
            {
                if (__nu_global_gui.mouse_down_node == __nu_global_gui.hovered_node) 
                { 
                    // Apply hover style
                    NU_Apply_Pseudo_Style_To_Node(__nu_global_gui.mouse_down_node, __nu_global_gui.stylesheet, PSEUDO_HOVER);

                    if (__nu_global_gui.hovered_node->event_flags & NU_EVENT_FLAG_ON_CLICK) 
                    {
                        void* found_cb = Hashmap_Get(&__nu_global_gui.on_click_events, &__nu_global_gui.hovered_node->handle);
                        if (found_cb != NULL) {
                            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
                            cb_info->callback(cb_info->handle, cb_info->args);
                        }
                    }
                    draw = true;
                }
                else { // Apply press style
                    NU_Apply_Stylesheet_To_Node(__nu_global_gui.mouse_down_node, __nu_global_gui.stylesheet);
                    draw = true;
                }

                __nu_global_gui.mouse_down_node = NULL;
            }
            break;
        default:
            break;
    }    

    if (draw) {
        timer_start();
        NU_Reflow();
        // timer_stop();
        // timer_start();
        // timer_stop();
        NU_Draw();
        timer_stop();

        // printf("\n\n");
    }
    return true;
}