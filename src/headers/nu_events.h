#include "nu_mouse_detection.h"

typedef struct NU_Event_Info_Mouse
{
    int mouse_btn;
    int mouse_x, mouse_y;
    float delta_x, delta_y;
} NU_Event_Info_Mouse;

typedef struct NU_Event_Info_Input
{
    char text[5];
} NU_Event_Info_Input;

typedef struct NU_Event
{
    uint32_t nodeHandle;
    NU_Event_Info_Mouse mouse;
    NU_Event_Info_Input input;
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
    NodeP* node = NODE_P(node_handle);
    struct NU_Event event = {0}; event.nodeHandle = node_handle;
    struct NU_Callback_Info cb_info = { event, args, callback };
    
    switch (event_type) {
        case NU_EVENT_ON_CLICK:
            node->node.eventFlags |= NU_EVENT_FLAG_ON_CLICK;
            HashmapSet(&__NGUI.on_click_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_INPUT_CHANGED:
            node->node.eventFlags |= NU_EVENT_FLAG_ON_INPUT_CHANGED;
            HashmapSet(&__NGUI.on_input_changed_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_DRAG:
            node->node.eventFlags |= NU_EVENT_FLAG_ON_DRAG;
            HashmapSet(&__NGUI.on_drag_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_RELEASED:
            node->node.eventFlags |= NU_EVENT_FLAG_ON_RELEASED;
            HashmapSet(&__NGUI.on_released_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_RESIZE:
            node->node.eventFlags |= NU_EVENT_FLAG_ON_RESIZE;
            HashmapSet(&__NGUI.on_resize_events, &node_handle, &cb_info);
            NU_NodeDimensions initial_dimensions = { -1.0f, -1.0f };
            HashmapSet(&__NGUI.node_resize_tracking, &node_handle, &initial_dimensions);
            break;
        case NU_EVENT_ON_MOUSE_DOWN:
            node->node.eventFlags |= NU_EVENT_FLAG_ON_MOUSE_DOWN;
            HashmapSet(&__NGUI.on_mouse_down_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_UP:
            node->node.eventFlags |= NU_EVENT_FLAG_ON_MOUSE_UP;
            HashmapSet(&__NGUI.on_mouse_up_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_MOVED:
            node->node.eventFlags |= NU_EVENT_FLAG_ON_MOUSE_MOVED;
            HashmapSet(&__NGUI.on_mouse_move_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_IN:
            node->node.eventFlags |= NU_EVENT_FLAG_ON_MOUSE_IN;
            HashmapSet(&__NGUI.on_mouse_in_events, &node_handle, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_OUT:
            node->node.eventFlags |= NU_EVENT_FLAG_ON_MOUSE_OUT;
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
            uint32_t nodeHandle = *(uint32_t*)key; 
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;
            NodeP* node = NODE_P(nodeHandle);
            void* dims_get =  HashmapGet(&__NGUI.node_resize_tracking, &nodeHandle);
            if (dims_get == NULL) continue; // Error (shouldn't happen ever)
            NU_NodeDimensions* dims = (NU_NodeDimensions*)dims_get;

            // Resize detected!
            if (dims->width != -1.0f && (node->node.width != dims->width || node->node.height != dims->height))
            {
                // Upate dims 
                dims->width = node->node.width;
                dims->height = node->node.height;

                // Call callback
                cb_info->callback(cb_info->event, cb_info->args);
                continue;

            }

            // Init dims
            if (dims->width == -1.0f) 
            {
                dims->width = node->node.width;
                dims->height = node->node.height;
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
            uint32_t nodeHandle = *(uint32_t*)key; 
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
    // --- Keypress ----------------------------------------------------------------------
    // -----------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_KEY_DOWN) {

        // if in text edit mode
        if (__NGUI.focused_node != UINT32_MAX && SDL_TextInputActive(NODE(__NGUI.focused_node)->window)) {

            NodeP* focusedNode = NODE_P(__NGUI.focused_node);
            InputText* inputText = &focusedNode->typeData.input.inputText;

            // backspace pressed
            if (event->key.key == SDLK_BACKSPACE) 
            {
                if (inputText->cursor > 0) __NGUI.awaiting_redraw = true;
                InputText_Backspace(inputText);
            }

            // left arrow pressed
            else if (event->key.key == SDLK_LEFT) 
            {
                if (inputText->cursor > 0) __NGUI.awaiting_redraw = true;
                InputText_MoveCursorLeft(inputText);
            }

            // right arrow pressed
            else if (event->key.key == SDLK_RIGHT) 
            {
                if (inputText->cursor < inputText->length) __NGUI.awaiting_redraw = true;
                InputText_MoveCursorRight(inputText);
            }
        }
    }
    // -----------------------------------------------------------------------------------
    // --- Type text ---------------------------------------------------------------------
    // -----------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_TEXT_INPUT) {
        NodeP* focusedNode = NODE_P(__NGUI.focused_node);
        if (focusedNode->type == INPUT && focusedNode->node.eventFlags & NU_EVENT_FLAG_ON_INPUT_CHANGED) {

            InputText_Write(&focusedNode->typeData.input.inputText, event->text.text);

            void* found_cb = HashmapGet(&__NGUI.on_input_changed_events, &__NGUI.focused_node);
            if (found_cb != NULL) {
                struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
                strcpy(cb_info->event.input.text, event->text.text);
                cb_info->callback(cb_info->event, cb_info->args);
            }
        }
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

        // If dragging scrollbar -> update node->node.scrollV 
        if (__NGUI.scroll_mouse_down_node != UINT32_MAX) 
        {
            NodeP* node = NODE_P(__NGUI.scroll_mouse_down_node);
            Layer* child_layer = &__NGUI.tree.layers[node->layer + 1];

            // Calculate drag_dist track_height thumb_height
            float track_height = node->node.height - node->node.borderTop - node->node.borderBottom;
            float inner_height_w_pad = track_height - node->node.padTop - node->node.padBottom;
            float inner_proportion_of_content_height = inner_height_w_pad / node->node.contentHeight;
            float thumb_height = inner_proportion_of_content_height * track_height;
            float track_top_y = node->node.y + node->node.borderTop;
            float drag_dist = (mouseY - __NGUI.v_scroll_thumb_grab_offset) - track_top_y;

            // Apply scroll and clamp
            node->node.scrollV = drag_dist / (track_height - thumb_height);
            node->node.scrollV = min(max(node->node.scrollV, 0.0f), 1.0f); // Clamp to range [0,1]

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
                uint32_t nodeHandle = *(uint32_t*)key; 
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
            
            if (NU_Mouse_Over_Node_V_Scrollbar(NODE_P(__NGUI.scroll_hovered_node), mouse_x, mouse_y)) 
            {
                __NGUI.scroll_mouse_down_node = __NGUI.scroll_hovered_node;

                // Record scroll thumb grab offset
                NodeP* node = NODE_P(__NGUI.scroll_mouse_down_node);
                float track_height = node->node.height - node->node.borderTop - node->node.borderBottom;
                float inner_height_w_pad = track_height - node->node.padTop - node->node.padBottom;
                float inner_proportion_of_content_height = inner_height_w_pad / node->node.contentHeight;
                float thumb_height = inner_proportion_of_content_height * track_height;
                float thumb_top_y = node->node.y + node->node.borderTop + (node->node.scrollV * (track_height - thumb_height));
                __NGUI.v_scroll_thumb_grab_offset = mouse_y - thumb_top_y;
            }
        }

        // If there is a mouse down node
        if (__NGUI.mouse_down_node != UINT32_MAX) 
        {
            // Apply presses pseudo style
            NU_Apply_Pseudo_Style_To_Node(NODE_P(__NGUI.hovered_node), __NGUI.stylesheet, PSEUDO_PRESS);

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

        // Set focused node
        __NGUI.focused_node = __NGUI.hovered_node;

        // toggle SDL_TextInput based on focus node type
        if (NODE_P(__NGUI.focused_node)->type == INPUT) {
            SDL_StartTextInput(NODE(__NGUI.focused_node)->window);
        } else {
            SDL_StopTextInput(NODE(__NGUI.hovered_node)->window);
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
                NU_Apply_Pseudo_Style_To_Node(NODE_P(__NGUI.mouse_down_node), __NGUI.stylesheet, PSEUDO_HOVER);
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
                NU_Apply_Stylesheet_To_Node(NODE_P(__NGUI.mouse_down_node), __NGUI.stylesheet);
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
        NodeP* node = NODE_P(__NGUI.scroll_hovered_node);
        if (node != NULL) 
        {
            float track_height = node->node.height - node->node.borderTop - node->node.borderBottom;
            float inner_height_w_pad = track_height - node->node.padTop - node->node.padBottom;
            float inner_proportion_of_content_height = inner_height_w_pad / node->node.contentHeight;
            float thumb_height = inner_proportion_of_content_height * track_height;
            node->node.scrollV -= event->wheel.y * (track_height / node->node.contentHeight) * 0.2f;
            node->node.scrollV = min(max(node->node.scrollV, 0.0f), 1.0f); // Clamp to range [0,1]

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