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
    Node* node;
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

void NU_Internal_Register_Event(Node* node, void* args, NU_Callback callback, enum NU_Event_Type event_type)
{
    struct NU_Event event = {0}; event.node = node;
    struct NU_Callback_Info cb_info = { event, args, callback };
    
    switch (event_type) {
        case NU_EVENT_ON_CLICK:
            node->eventFlags |= NU_EVENT_FLAG_ON_CLICK;
            HashmapSet(&__NGUI.on_click_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_INPUT_CHANGED:
            node->eventFlags |= NU_EVENT_FLAG_ON_INPUT_CHANGED;
            HashmapSet(&__NGUI.on_input_changed_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_DRAG:
            node->eventFlags |= NU_EVENT_FLAG_ON_DRAG;
            HashmapSet(&__NGUI.on_drag_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_RELEASED:
            node->eventFlags |= NU_EVENT_FLAG_ON_RELEASED;
            HashmapSet(&__NGUI.on_released_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_RESIZE:
            node->eventFlags |= NU_EVENT_FLAG_ON_RESIZE;
            HashmapSet(&__NGUI.on_resize_events, &node, &cb_info);
            NU_NodeDimensions initial_dimensions = { -1.0f, -1.0f };
            HashmapSet(&__NGUI.node_resize_tracking, &node, &initial_dimensions);
            break;
        case NU_EVENT_ON_MOUSE_DOWN:
            node->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_DOWN;
            HashmapSet(&__NGUI.on_mouse_down_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_UP:
            node->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_UP;
            HashmapSet(&__NGUI.on_mouse_up_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_MOVED:
            node->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_MOVED;
            HashmapSet(&__NGUI.on_mouse_move_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_IN:
            node->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_IN;
            HashmapSet(&__NGUI.on_mouse_in_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_OUT:
            node->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_OUT;
            HashmapSet(&__NGUI.on_mouse_out_events, &node, &cb_info);
            break;
    }
}

void CheckForResizeEvents()
{
    if (__NGUI.on_resize_events.itemCount > 0)
    {
        Hashmap* resizeHmap = &__NGUI.on_resize_events;
        Hashmap* resizeTrackingHmap = &__NGUI.node_resize_tracking;
        HashmapIterator it = HashmapCreateIterator(resizeHmap);
        void* key; void* val;

        while(HashmapIteratorNext(&it, &key, &val))
        {
            // cast key, val to correct types
            Node* node = *(Node**)key; 
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;

            // get current dimensions
            void* dims_get = HashmapGet(resizeTrackingHmap, &node);
            NU_NodeDimensions* dims = (NU_NodeDimensions*)dims_get;

            // Resize detected? -> update dims and call callback
            if (dims->width != -1.0f && (node->width != dims->width || node->height != dims->height)) {
                dims->width = node->width;
                dims->height = node->height;
                cb_info->callback(cb_info->event, cb_info->args);
                continue;
            }

            // Init dims
            if (dims->width == -1.0f) {
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
        Hashmap* hmap = &__NGUI.on_mouse_up_events;
        HashmapIterator it = HashmapCreateIterator(hmap);
        void* key; void* val;

        while(HashmapIteratorNext(&it, &key, &val))
        {
            // cast key, value to correct types
            uint32_t nodeHandle = *(uint32_t*)key; 
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;

            // set calback event values and trigger
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

    // ------------------------------------------------------------------------------------
    // --- Window closed -> main window ? close application : destroy sub window branch ---
    // ------------------------------------------------------------------------------------
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        __NGUI.running = false;
    }
    // ------------------------------------------------------------------------------------
    // --- Quit event -> close application ------------------------------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_QUIT) {
        __NGUI.running = false;
    }
    // ------------------------------------------------------------------------------------
    // --- Resized window -> redraw -------------------------------------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        __NGUI.awaiting_redraw = true;
    }
    // ------------------------------------------------------------------------------------
    // --- App render event called -> redraw ----------------------------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == __NGUI.SDL_CUSTOM_RENDER_EVENT) {
        __NGUI.awaiting_redraw = true;
    }
    // ------------------------------------------------------------------------------------
    // --- Keypress -----------------------------------------------------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_KEY_DOWN) {

        // if in text edit mode
        if (__NGUI.focused_node != NULL && SDL_TextInputActive(__NGUI.focused_node->node.window)) 
        {
            NodeP* inputNode = __NGUI.focused_node;
            NU_Font* font = Vector_Get(&__NGUI.stylesheet->fonts, inputNode->node.fontId);
            InputText* inputText = &inputNode->typeData.input.inputText;
            SDL_Keymod mods = SDL_GetModState();

            // backspace pressed
            if (event->key.key == SDLK_BACKSPACE) {
                // control + backspace
                if (mods & SDL_KMOD_CTRL && InputText_BackspaceWord(inputText, inputNode, font)) __NGUI.awaiting_redraw = true;
                // backspace
                else if (InputText_Backspace(inputText, inputNode, font)) __NGUI.awaiting_redraw = true;
            }

            // left arrow pressed
            else if (event->key.key == SDLK_LEFT) {
                // control = left arrow 
                if (mods & SDL_KMOD_CTRL && InputText_MoveCursorLeftSpan(inputText, inputNode, font)) __NGUI.awaiting_redraw = true;
                // only backspace
                else if (InputText_MoveCursorLeft(inputText, inputNode, font)) __NGUI.awaiting_redraw = true;
            }

            // right arrow pressed
            else if (event->key.key == SDLK_RIGHT) {
                // control = right arrow
                if (mods & SDL_KMOD_CTRL && InputText_MoveCursorRightSpan(inputText, inputNode, font)) __NGUI.awaiting_redraw = true;
                // only backspace
                else if (InputText_MoveCursorRight(inputText, inputNode, font)) __NGUI.awaiting_redraw = true;
            }
        }
    }
    // ------------------------------------------------------------------------------------
    // --- Type text ----------------------------------------------------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_TEXT_INPUT) {
        NodeP* focusedNode = __NGUI.focused_node;
        if (focusedNode->type == NU_INPUT) {
            NU_Font* font = Vector_Get(&__NGUI.stylesheet->fonts, focusedNode->node.fontId);
            int updated = InputText_Write(&focusedNode->typeData.input.inputText, focusedNode, font, event->text.text);
            if (updated && focusedNode->node.eventFlags & NU_EVENT_FLAG_ON_INPUT_CHANGED) {
                void* found_cb = HashmapGet(&__NGUI.on_input_changed_events, &__NGUI.focused_node->node);
                if (found_cb != NULL) {
                    struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
                    strcpy(cb_info->event.input.text, event->text.text);
                    cb_info->callback(cb_info->event, cb_info->args);
                }
            }
            __NGUI.awaiting_redraw |= updated;
        }

    }
    // ------------------------------------------------------------------------------------
    // --- Move mouse -> redraw if mouse moves off hovered node ---------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        // update hovered window
        __NGUI.winManager.hoveredWindow = SDL_GetWindowFromID(event->window.windowID);

        // get hovered node and save previous
        NodeP* prev_hovered_node = __NGUI.hovered_node;
        NU_Mouse_Hover();

        // get local mouse coordinates
        float mouseX, mouseY; GetLocalMouseCoords(&__NGUI.winManager, &mouseX, &mouseY);

        // on mouse in event triggered
        if (__NGUI.hovered_node != NULL &&
            prev_hovered_node != __NGUI.hovered_node && 
            __NGUI.hovered_node->node.eventFlags & NU_EVENT_FLAG_ON_MOUSE_IN)
        {
            void* found_cb = HashmapGet(&__NGUI.on_mouse_in_events, &__NGUI.hovered_node->node);
            if (found_cb != NULL) {
                struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
                cb_info->event.mouse.mouse_x = mouseX;
                cb_info->event.mouse.mouse_y = mouseY;
                cb_info->event.mouse.delta_x = event->motion.xrel;
                cb_info->event.mouse.delta_y = event->motion.yrel;
                cb_info->callback(cb_info->event, cb_info->args);
            }
        }

        // on mouse out event triggered
        if (prev_hovered_node != NULL &&
            prev_hovered_node != __NGUI.hovered_node && 
            prev_hovered_node->node.eventFlags & NU_EVENT_FLAG_ON_MOUSE_OUT)
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

        // if dragging scrollbar -> update node->node.scrollV 
        if (__NGUI.scroll_mouse_down_node != NULL) 
        {
            NodeP* node = __NGUI.scroll_mouse_down_node;

            // calculate drag_dist track_height thumb_height
            float track_height = node->node.height - node->node.borderTop - node->node.borderBottom;
            float inner_height_w_pad = track_height - node->node.padTop - node->node.padBottom;
            float inner_proportion_of_content_height = inner_height_w_pad / node->node.contentHeight;
            float thumb_height = inner_proportion_of_content_height * track_height;
            float track_top_y = node->node.y + node->node.borderTop;
            float drag_dist = (mouseY - __NGUI.v_scroll_thumb_grab_offset) - track_top_y;

            // apply scroll and clamp
            node->node.scrollV = drag_dist / (track_height - thumb_height);
            node->node.scrollV = min(max(node->node.scrollV, 0.0f), 1.0f); // Clamp to range [0,1]

            // must redraw later
            __NGUI.awaiting_redraw = true;
        }

        // check for mouse move events
        if (__NGUI.on_mouse_move_events.itemCount > 0)
        {
            Hashmap* hmap = &__NGUI.on_mouse_move_events;
            HashmapIterator it = HashmapCreateIterator(hmap);
            void* key; void* val;

            while(HashmapIteratorNext(&it, &key, &val))
            {
                // cast key, val to correct types
                uint32_t nodeHandle = *(uint32_t*)key; 
                struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;

                // set callback event values and trigger
                cb_info->event.mouse.mouse_x = mouseX;
                cb_info->event.mouse.mouse_y = mouseY;
                cb_info->event.mouse.delta_x = event->motion.xrel;
                cb_info->event.mouse.delta_y = event->motion.yrel;
                cb_info->callback(cb_info->event, cb_info->args);
            }
        }

        // if focused on text input -> update highlighting
        if (__NGUI.focused_node != NULL && __NGUI.focused_node->type == NU_INPUT) {
            NodeP* node = __NGUI.focused_node;
            // update mouse offset
            node->typeData.input.inputText.mouseOffset = mouseX - (node->node.x + node->node.borderLeft + node->node.padLeft);
        }
    }
    // ------------------------------------------------------------------------------------
    // --- Mouse button pressed down ------------------------------------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        // If mouse hasn't moved yet the hovered window and node will not have been set -> set both of these
        if (__NGUI.winManager.hoveredWindow == NULL) {
            __NGUI.winManager.hoveredWindow = SDL_GetWindowFromID(event->window.windowID);
            NU_Mouse_Hover();
        }

        // Get mouse down coordinates
        SDL_GetGlobalMouseState(&__NGUI.mouse_down_global_x, &__NGUI.mouse_down_global_y);

        // Set mouse down node
        __NGUI.mouse_down_node = __NGUI.hovered_node;

        // If mouse is hovered over scroll thumb -> set scroll mouse down node
        if (__NGUI.scroll_hovered_node != NULL) 
        {
            int win_x, win_y; 
            SDL_GetWindowPosition(__NGUI.scroll_hovered_node->node.window, &win_x, &win_y);
            float mouse_x = __NGUI.mouse_down_global_x - win_x;
            float mouse_y = __NGUI.mouse_down_global_y - win_y;
            
            if (NU_Mouse_Over_Node_V_Scrollbar(__NGUI.scroll_hovered_node, mouse_x, mouse_y)) 
            {
                __NGUI.scroll_mouse_down_node = __NGUI.scroll_hovered_node;

                // Record scroll thumb grab offset
                NodeP* node = __NGUI.scroll_mouse_down_node;
                float track_height = node->node.height - node->node.borderTop - node->node.borderBottom;
                float inner_height_w_pad = track_height - node->node.padTop - node->node.padBottom;
                float inner_proportion_of_content_height = inner_height_w_pad / node->node.contentHeight;
                float thumb_height = inner_proportion_of_content_height * track_height;
                float thumb_top_y = node->node.y + node->node.borderTop + (node->node.scrollV * (track_height - thumb_height));
                __NGUI.v_scroll_thumb_grab_offset = mouse_y - thumb_top_y;
            }
        }

        // If there is a mouse down node
        if (__NGUI.mouse_down_node != NULL) 
        {
            // Apply presses pseudo style
            NU_Apply_Pseudo_Style_To_Node(__NGUI.hovered_node, __NGUI.stylesheet, PSEUDO_PRESS);

            // If node has a mouse down event
            if (__NGUI.mouse_down_node->node.eventFlags & NU_EVENT_FLAG_ON_MOUSE_DOWN)
            {
                void* found_cb = HashmapGet(&__NGUI.on_mouse_down_events, &__NGUI.mouse_down_node->node);
                if (found_cb != NULL) {
                    int win_x, win_y; 
                    SDL_GetWindowPosition(__NGUI.mouse_down_node->node.window, &win_x, &win_y);
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
        NodeP* prevFocusedNode = __NGUI.focused_node;
        __NGUI.focused_node = __NGUI.hovered_node;

        // Defocus prev focused input node
        if (prevFocusedNode != NULL && __NGUI.focused_node != prevFocusedNode && prevFocusedNode->type == NU_INPUT) {
            InputText_Defocus(&prevFocusedNode->typeData.input.inputText);
        }

        // Focus on node
        if (__NGUI.focused_node != NULL) {

            // Focus on input node
            if (__NGUI.focused_node->type == NU_INPUT) {
                InputText_Focus(&__NGUI.focused_node->typeData.input.inputText); 
                SDL_StartTextInput(__NGUI.focused_node->node.window);
            } else {
                SDL_StopTextInput(__NGUI.hovered_node->node.window);
            }
        }

        __NGUI.awaiting_redraw = true;
    }
    // ------------------------------------------------------------------------------------
    // --- Focus on window -> redraw ------------------------------------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_WINDOW_FOCUS_GAINED)
    {
        __NGUI.mouse_down_node = __NGUI.hovered_node;
        __NGUI.awaiting_redraw = true;
    }
    // ------------------------------------------------------------------------------------
    // --- Released mouse button ----------------------------------------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        __NGUI.scroll_mouse_down_node = NULL;

        SDL_GetGlobalMouseState(&__NGUI.mouse_down_global_x, &__NGUI.mouse_down_global_y);

        // Trigger all mouse up events
        if (__NGUI.winManager.hoveredWindow != NULL)
        {
            int win_x, win_y; 
            SDL_GetWindowPosition(__NGUI.hovered_node->node.window, &win_x, &win_y);
            float mouse_x = __NGUI.mouse_down_global_x - win_x;
            float mouse_y = __NGUI.mouse_down_global_y - win_y;
            TriggerAllMouseupEvents(mouse_x, mouse_y, (int)event->button.button);
        }

        // If there is a pressed node
        if (__NGUI.mouse_down_node != NULL)
        {   
            // If the mouse is hovering over pressed node
            if (__NGUI.mouse_down_node == __NGUI.hovered_node) 
            { 
                // Apply hover style
                NU_Apply_Pseudo_Style_To_Node(__NGUI.mouse_down_node, __NGUI.stylesheet, PSEUDO_HOVER);
                __NGUI.awaiting_redraw = true;

                // Get mouse up coordinates
                int win_x, win_y; 
                SDL_GetGlobalMouseState(&__NGUI.mouse_down_global_x, &__NGUI.mouse_down_global_y);
                SDL_GetWindowPosition(__NGUI.hovered_node->node.window, &win_x, &win_y);
                float mouse_x = __NGUI.mouse_down_global_x - win_x;
                float mouse_y = __NGUI.mouse_down_global_y - win_y;

                // If there is a click event assigned to the pressed node
                if (__NGUI.hovered_node->node.eventFlags & NU_EVENT_FLAG_ON_CLICK) 
                {
                    void* found_cb = HashmapGet(&__NGUI.on_click_events, &__NGUI.hovered_node->node);
                    if (found_cb != NULL) {
                        struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
                        cb_info->callback(cb_info->event, cb_info->args);
                    }
                }
            }
            // If the mouse is released over something other than the pressed node -> revert pressed node to default style
            else 
            { 
                NU_Apply_Stylesheet_To_Node(__NGUI.mouse_down_node, __NGUI.stylesheet);
                __NGUI.awaiting_redraw = true;
            }

            // There is no longer a pressed node
            __NGUI.mouse_down_node = NULL;
        }
    }
    // ------------------------------------------------------------------------------------
    // --- Mouse scroll wheel -------------------------------------------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_MOUSE_WHEEL)
    {
        NodeP* node = __NGUI.scroll_hovered_node;
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
        timer_start();
        NU_Layout();
        timer_stop();
        timer_start();
        CheckForResizeEvents();
        NU_Draw();
        timer_stop();
        printf("\n");
    }
    return true;
}