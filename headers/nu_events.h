#pragma once

#include <SDL3/SDL.h>
#include "performance.h"


void NU_Register_Event(struct NU_GUI* gui, struct Node* node, void* args, NU_Callback callback, enum NU_Event event)
{
    struct NU_Callback_Info cb_info = { node, args, callback };
    
    switch (event) {
        case NU_EVENT_ON_CLICK:
            node->event_flags |= NU_EVENT_FLAG_ON_CLICK;
            Hashmap_Set(&gui->on_click_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_CHANGED:
            node->event_flags |= NU_EVENT_FLAG_ON_CHANGED;
            Hashmap_Set(&gui->on_changed_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_DRAG:
            node->event_flags |= NU_EVENT_FLAG_ON_DRAG;
            Hashmap_Set(&gui->on_drag_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_RELEASED:
            node->event_flags |= NU_EVENT_FLAG_ON_RELEASED;
            Hashmap_Set(&gui->on_released_events, &node, &cb_info);
            break;
    }
}






// -----------------------------
// Event watcher ---------------
// -----------------------------
struct NU_Watcher_Data {
    struct NU_GUI* ngui;
};

bool ResizingEventWatcher(void* data, SDL_Event* event) 
{
    struct NU_Watcher_Data* wd = (struct NU_Watcher_Data*)data;

    switch (event->type) {
        case SDL_EVENT_WINDOW_RESIZED:
            NU_Calculate(wd->ngui);
            NU_Draw_Nodes(wd->ngui);
            break;
        case SDL_EVENT_MOUSE_MOTION:
            Uint32 id = event->motion.windowID;
            wd->ngui->hovered_window = SDL_GetWindowFromID(id);
            NU_Calculate(wd->ngui);
            NU_Draw_Nodes(wd->ngui);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            wd->ngui->mouse_down_node = wd->ngui->hovered_node;
            break;
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            wd->ngui->mouse_down_node = wd->ngui->hovered_node;
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (wd->ngui->mouse_down_node != NULL && wd->ngui->mouse_down_node == wd->ngui->hovered_node && wd->ngui->hovered_node->event_flags & NU_EVENT_FLAG_ON_CLICK) {
                void* found_cb = Hashmap_Get(&wd->ngui->on_click_events, &wd->ngui->hovered_node);
                if (found_cb != NULL)
                {
                    struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
                    cb_info->callback(cb_info->node, cb_info->args);
                }
            }
            break;
        default:
            break;
    }    
    return true;
}