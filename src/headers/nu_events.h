#pragma once
#define NU_EVENT_FLAG_ON_CLICK       0x01         // 0b0000000000000001
#define NU_EVENT_FLAG_ON_CHANGED     0x02         // 0b0000000000000010
#define NU_EVENT_FLAG_ON_DRAG        0x04         // 0b0000000000000100
#define NU_EVENT_FLAG_ON_RELEASED    0x08         // 0b0000000000001000
#define NU_EVENT_FLAG_ON_RESIZE      0x10         // 0b0000000000010000
#define NU_EVENT_FLAG_ON_MOUSE_DOWN  0x20         // 0b0000000000100000
#define NU_EVENT_FLAG_ON_MOUSE_UP    0x40         // 0b0000000001000000
#define NU_EVENT_FLAG_ON_MOUSE_MOVED 0x80         // 0b0000000010000000
#define NU_EVENT_FLAG_ON_MOUSE_IN    0x100        // 0b0000000100000000
#define NU_EVENT_FLAG_ON_MOUSE_OUT   0x200        // 0b0000001000000000

enum NU_Event_Type
{
    NU_EVENT_ON_CLICK,
    NU_EVENT_ON_CHANGED,
    NU_EVENT_ON_DRAG,
    NU_EVENT_ON_RELEASED,
    NU_EVENT_ON_RESIZE,
    NU_EVENT_ON_MOUSE_DOWN,
    NU_EVENT_ON_MOUSE_UP,
    NU_EVENT_ON_MOUSE_MOVED,
    NU_EVENT_ON_MOUSE_IN,
    NU_EVENT_ON_MOUSE_OUT,
};

typedef struct NU_Event_Info_Mouse
{
    int mouse_btn;
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
    Node* node = NODE(node_handle);
    struct NU_Event event = {0}; event.handle = node_handle;
    struct NU_Callback_Info cb_info = { event, args, callback };
    
    switch (event_type) {
        case NU_EVENT_ON_CLICK:
            node->eventFlags |= NU_EVENT_FLAG_ON_CLICK;
            HashmapSet(&__NGUI.on_click_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_CHANGED:
            node->eventFlags |= NU_EVENT_FLAG_ON_CHANGED;
            HashmapSet(&__NGUI.on_changed_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_DRAG:
            node->eventFlags |= NU_EVENT_FLAG_ON_DRAG;
            HashmapSet(&__NGUI.on_drag_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_RELEASED:
            node->eventFlags |= NU_EVENT_FLAG_ON_RELEASED;
            HashmapSet(&__NGUI.on_released_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_RESIZE:
            node->eventFlags |= NU_EVENT_FLAG_ON_RESIZE;
            HashmapSet(&__NGUI.on_resize_events, &node_handle, &cb_info);
            NU_Node_Dimensions initial_dimensions = { -1.0f, -1.0f };
            HashmapSet(&__NGUI.node_resize_tracking, &node_handle, &initial_dimensions);
            break;
        case NU_EVENT_ON_MOUSE_DOWN:
            node->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_DOWN;
            HashmapSet(&__NGUI.on_mouse_down_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_UP:
            node->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_UP;
            HashmapSet(&__NGUI.on_mouse_up_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_MOVED:
            node->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_MOVED;
            HashmapSet(&__NGUI.on_mouse_move_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_IN:
            node->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_IN;
            HashmapSet(&__NGUI.on_mouse_in_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_OUT:
            node->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_OUT;
            HashmapSet(&__NGUI.on_mouse_out_events, &node_handle, &cb_info);
            break;
    }
}

void CheckForResizeEvents()
{
    if (__NGUI.on_resize_events.itemCount > 0)
    {
        HashmapIterateBegin(&__NGUI.on_resize_events);
        while(HashmapIterateContinue(&__NGUI.on_resize_events))
        {
            void* key; 
            void* val;
            HashmapIterateGet(&__NGUI.on_resize_events, &key, &val);
            if (key == NULL || val == NULL) continue; // Error (shouldn't happen ever)
            uint32_t handle = *(uint32_t*)key; 
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;
            Node* node = NODE(handle);
            void* dims_get =  HashmapGet(&__NGUI.node_resize_tracking, &handle);
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

void TriggerAllMouseupEvents(float mouse_x, float mouse_y, int mouse_btn)
{
    if (__NGUI.on_mouse_up_events.itemCount > 0)
    {
        HashmapIterateBegin(&__NGUI.on_mouse_up_events);
        while(HashmapIterateContinue(&__NGUI.on_mouse_up_events))
        {
            void* key;
            void* val;
            HashmapIterateGet(&__NGUI.on_mouse_up_events, &key, &val);
            if (key == NULL || val == NULL) continue; // Error (shouldn't happen ever)
            uint32_t handle = *(uint32_t*)key; 
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;
            cb_info->event.mouse.mouse_btn = mouse_btn;
            cb_info->event.mouse.mouse_x = mouse_x;
            cb_info->event.mouse.mouse_y = mouse_y;
            cb_info->callback(cb_info->event, cb_info->args);
        }
    }
}

bool EventWatcher(void* data, SDL_Event* event) 
{
    if (!__NGUI.running) return false;

    // -----------------------------------------------------------------------------------
    // --- Window Closed -> main Window? close application : destroy sub window branch ---
    // -----------------------------------------------------------------------------------
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        __NGUI.running = false;
    }
    // -----------------------------------------------------------------------------------
    // --- Close application -------------------------------------------------------------
    // -----------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_QUIT) {
        __NGUI.running = false;
    }
    // -----------------------------------------------------------------------------------
    // --- Resize -> redraw --------------------------------------------------------------
    // -----------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        __NGUI.awaiting_redraw = true;
    }
    // -----------------------------------------------------------------------------------
    // --- Render -> redraw --------------------------------------------------------------
    // -----------------------------------------------------------------------------------
    else if (event->type == __NGUI.SDL_CUSTOM_RENDER_EVENT) {
        __NGUI.awaiting_redraw = true;
    }
    // -----------------------------------------------------------------------------------
    // --- Move mouse -> redraw if mouse moves off hovered node --------------------------
    // -----------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        Uint32 id = event->motion.windowID;
        __NGUI.winManager.hoveredWindow = SDL_GetWindowFromID(id);
        uint32_t prev_hovered_node = __NGUI.hovered_node;
        NU_Mouse_Hover();

        // Get local mouse coordinates
        float mouseX, mouseY; GetLocalMouseCoords(&__NGUI.winManager, &mouseX, &mouseY);

        // On mouse in event triggered
        if (__NGUI.hovered_node != UINT32_MAX &&
            prev_hovered_node != __NGUI.hovered_node && 
            NODE(__NGUI.hovered_node)->eventFlags & NU_EVENT_FLAG_ON_MOUSE_IN)
        {
            void* found_cb = HashmapGet(&__NGUI.on_mouse_in_events, &__NGUI.hovered_node);
            if (found_cb != NULL) {
                struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
                cb_info->event.mouse.mouse_x = mouseX;
                cb_info->event.mouse.mouse_y = mouseY;
                cb_info->event.mouse.delta_x = event->motion.xrel;
                cb_info->event.mouse.delta_y = event->motion.yrel;
                cb_info->callback(cb_info->event, cb_info->args);
            }
        }

        // On mouse out event triggered
        if (prev_hovered_node != UINT32_MAX &&
            prev_hovered_node != __NGUI.hovered_node && 
            NODE(prev_hovered_node)->eventFlags & NU_EVENT_FLAG_ON_MOUSE_OUT)
        {
            void* found_cb = HashmapGet(&__NGUI.on_mouse_out_events, &prev_hovered_node);
            if (found_cb != NULL) {
                struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
                cb_info->event.mouse.mouse_x = mouseX;
                cb_info->event.mouse.mouse_y = mouseY;
                cb_info->event.mouse.delta_x = event->motion.xrel;
                cb_info->event.mouse.delta_y = event->motion.yrel;
                cb_info->callback(cb_info->event, cb_info->args);
            }
        }

        // If dragging scrollbar -> update node->scrollV 
        if (__NGUI.scroll_mouse_down_node != UINT32_MAX) 
        {
            Node* node = NODE(__NGUI.scroll_mouse_down_node);
            NU_Layer* child_layer = &__NGUI.tree.layers[node->layer + 1];

            // Calculate drag_dist track_height thumb_height
            float track_height = node->height - node->borderTop - node->borderBottom;
            float inner_height_w_pad = track_height - node->padTop - node->padBottom;
            float inner_proportion_of_content_height = inner_height_w_pad / node->contentHeight;
            float thumb_height = inner_proportion_of_content_height * track_height;
            float track_top_y = node->y + node->borderTop;
            float drag_dist = (mouseY - __NGUI.v_scroll_thumb_grab_offset) - track_top_y;

            // Apply scroll and clamp
            node->scrollV = drag_dist / (track_height - thumb_height);
            node->scrollV = min(max(node->scrollV, 0.0f), 1.0f); // Clamp to range [0,1]

            // Must redraw later
            __NGUI.awaiting_redraw = true;
        }

        // Check for mouse move events
        if (__NGUI.on_mouse_move_events.itemCount > 0)
        {
            HashmapIterateBegin(&__NGUI.on_mouse_move_events);
            while(HashmapIterateContinue(&__NGUI.on_mouse_move_events))
            {
                void* key; 
                void* val;
                HashmapIterateGet(&__NGUI.on_mouse_move_events, &key, &val);
                uint32_t handle = *(uint32_t*)key; 
                struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;

                // Call callback
                cb_info->event.mouse.mouse_x = mouseX;
                cb_info->event.mouse.mouse_y = mouseY;
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
        SDL_GetGlobalMouseState(&__NGUI.mouse_down_global_x, &__NGUI.mouse_down_global_y);

        // Set mouse down node
        __NGUI.mouse_down_node = __NGUI.hovered_node;

        // If mouse is hovered over scroll thumb -> set scroll mouse down node
        if (__NGUI.scroll_hovered_node != UINT32_MAX) 
        {
            int win_x, win_y; 
            SDL_GetWindowPosition(NODE(__NGUI.scroll_hovered_node)->window, &win_x, &win_y);
            float mouse_x = __NGUI.mouse_down_global_x - win_x;
            float mouse_y = __NGUI.mouse_down_global_y - win_y;
            
            if (NU_Mouse_Over_Node_V_Scrollbar(NODE(__NGUI.scroll_hovered_node), mouse_x, mouse_y)) 
            {
                __NGUI.scroll_mouse_down_node = __NGUI.scroll_hovered_node;

                // Record scroll thumb grab offset
                Node* node = NODE(__NGUI.scroll_mouse_down_node);
                float track_height = node->height - node->borderTop - node->borderBottom;
                float inner_height_w_pad = track_height - node->padTop - node->padBottom;
                float inner_proportion_of_content_height = inner_height_w_pad / node->contentHeight;
                float thumb_height = inner_proportion_of_content_height * track_height;
                float thumb_top_y = node->y + node->borderTop + (node->scrollV * (track_height - thumb_height));
                __NGUI.v_scroll_thumb_grab_offset = mouse_y - thumb_top_y;
            }
        }

        // If there is a mouse down node
        if (__NGUI.mouse_down_node != UINT32_MAX) 
        {
            // Apply presses pseudo style
            NU_Apply_Pseudo_Style_To_Node(NODE(__NGUI.hovered_node), __NGUI.stylesheet, PSEUDO_PRESS);

            // If node has a mouse down event
            if (NODE(__NGUI.mouse_down_node)->eventFlags & NU_EVENT_FLAG_ON_MOUSE_DOWN)
            {
                void* found_cb = HashmapGet(&__NGUI.on_mouse_down_events, &__NGUI.mouse_down_node);
                if (found_cb != NULL) {
                    int win_x, win_y; 
                    SDL_GetWindowPosition(NODE(__NGUI.mouse_down_node)->window, &win_x, &win_y);
                    float mouse_x = __NGUI.mouse_down_global_x - win_x;
                    float mouse_y = __NGUI.mouse_down_global_y - win_y;
                    
                    struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
                    cb_info->event.mouse.mouse_btn = (int)event->button.button;
                    cb_info->event.mouse.mouse_x = mouse_x;
                    cb_info->event.mouse.mouse_y = mouse_y;
                    cb_info->callback(cb_info->event, cb_info->args);
                }
            }
        } 

        __NGUI.awaiting_redraw = true;
    }
    // -----------------------------------------------------------------------------------
    // --- Focus on window -> redraw -----------------------------------------------------
    // -----------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_WINDOW_FOCUS_GAINED)
    {
        __NGUI.mouse_down_node = __NGUI.hovered_node;
        __NGUI.awaiting_redraw = true;
    }
    // -----------------------------------------------------------------------------------
    // --- Released mouse button ---------------------------------------------------------
    // -----------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        __NGUI.scroll_mouse_down_node = UINT32_MAX;

        SDL_GetGlobalMouseState(&__NGUI.mouse_down_global_x, &__NGUI.mouse_down_global_y);

        // Trigger all mouse up events
        if (__NGUI.hovered_window != NULL)
        {
            int win_x, win_y; 
            SDL_GetWindowPosition(NODE(__NGUI.hovered_node)->window, &win_x, &win_y);
            float mouse_x = __NGUI.mouse_down_global_x - win_x;
            float mouse_y = __NGUI.mouse_down_global_y - win_y;
            TriggerAllMouseupEvents(mouse_x, mouse_y, (int)event->button.button);
        }

        // If there is a pressed node
        if (__NGUI.mouse_down_node != UINT32_MAX)
        {   
            // If the mouse is hovering over pressed node
            if (__NGUI.mouse_down_node == __NGUI.hovered_node) 
            { 
                // Apply hover style
                NU_Apply_Pseudo_Style_To_Node(NODE(__NGUI.mouse_down_node), __NGUI.stylesheet, PSEUDO_HOVER);
                __NGUI.awaiting_redraw = true;

                // Get mouse up coordinates
                int win_x, win_y; 
                SDL_GetGlobalMouseState(&__NGUI.mouse_down_global_x, &__NGUI.mouse_down_global_y);
                SDL_GetWindowPosition(NODE(__NGUI.hovered_node)->window, &win_x, &win_y);
                float mouse_x = __NGUI.mouse_down_global_x - win_x;
                float mouse_y = __NGUI.mouse_down_global_y - win_y;

                // If there is a click event assigned to the pressed node
                if (NODE(__NGUI.hovered_node)->eventFlags & NU_EVENT_FLAG_ON_CLICK) 
                {
                    void* found_cb = HashmapGet(&__NGUI.on_click_events, &__NGUI.hovered_node);
                    if (found_cb != NULL) {
                        struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
                        cb_info->callback(cb_info->event, cb_info->args);
                    }
                }
            }
            // If the mouse is released over something other than the pressed node -> revert pressed node to default style
            else 
            { 
                NU_Apply_Stylesheet_To_Node(NODE(__NGUI.mouse_down_node), __NGUI.stylesheet);
                __NGUI.awaiting_redraw = true;
            }

            // There is no longer a pressed node
            __NGUI.mouse_down_node = UINT32_MAX;
        }
    }
    // -----------------------------------------------------------------------------------
    // --- Mouse scroll wheel ------------------------------------------------------------
    // -----------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_MOUSE_WHEEL)
    {
        Node* node = NODE(__NGUI.scroll_hovered_node);
        if (node != NULL) 
        {
            float track_height = node->height - node->borderTop - node->borderBottom;
            float inner_height_w_pad = track_height - node->padTop - node->padBottom;
            float inner_proportion_of_content_height = inner_height_w_pad / node->contentHeight;
            float thumb_height = inner_proportion_of_content_height * track_height;
            node->scrollV -= event->wheel.y * (track_height / node->contentHeight) * 0.2f;
            node->scrollV = min(max(node->scrollV, 0.0f), 1.0f); // Clamp to range [0,1]

            // Re-render and exit function
            NU_Layout();
            NU_Mouse_Hover();
            NU_Draw();
            return true;
        }
    }


    if (__NGUI.awaiting_redraw) 
    {
        NU_Layout();
        NU_Draw();
        CheckForResizeEvents();
    }
    return true;
}