
void NU_Internal_Register_Event(Node* node, void* args, NU_Callback callback, enum NU_Event_Type event_type)
{
    struct NU_Event event = {0}; event.node = node;
    struct NU_Callback_Info cb_info = { event, args, callback };

    NodeP* nodeP = NODEP_OF(node);
    
    switch (event_type) {
        case NU_EVENT_ON_CLICK:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_CLICK;
            HashmapSet(&GUI.on_click_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_INPUT_CHANGED:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_INPUT_CHANGED;
            HashmapSet(&GUI.on_input_changed_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_DRAG:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_DRAG;
            HashmapSet(&GUI.on_drag_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_RELEASED:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_RELEASED;
            HashmapSet(&GUI.on_released_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_RESIZE:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_RESIZE;
            HashmapSet(&GUI.on_resize_events, &node, &cb_info);
            NU_NodeDimensions initial_dimensions = { -1.0f, -1.0f };
            HashmapSet(&GUI.node_resize_tracking, &node, &initial_dimensions);
            break;
        case NU_EVENT_ON_MOUSE_DOWN:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_DOWN;
            HashmapSet(&GUI.on_mouse_down_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_UP:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_UP;
            HashmapSet(&GUI.on_mouse_up_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_DOWN_OUTSIDE:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_DOWN_OUTSIDE;
            HashmapSet(&GUI.on_mouse_down_outside_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_MOVED:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_MOVED;
            HashmapSet(&GUI.on_mouse_move_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_IN:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_IN;
            HashmapSet(&GUI.on_mouse_in_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_OUT:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_OUT;
            HashmapSet(&GUI.on_mouse_out_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_WHEEL:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_WHEEL;
            HashmapSet(&GUI.on_mouse_wheel_events, &node, &cb_info);
            break;
    }
}

void CheckForResizeEvents()
{
    if (GUI.on_resize_events.itemCount > 0)
    {
        Hashmap* resizeHmap = &GUI.on_resize_events;
        Hashmap* resizeTrackingHmap = &GUI.node_resize_tracking;
        HashmapIterator it = HashmapCreateIterator(resizeHmap);
        void* key; void* val;

        while(HashmapIteratorNext(&it, &key, &val))
        {
            // cast key, val to correct types
            Node* node = *(Node**)key; 
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;

            // if node was deleted during this function -> skip
            //if (SetContains(&GUI.deletedNodesWithRegisteredEvents, node)) continue;

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

    // // 
    // if (GUI.deletedNodesWithRegisteredEvents.itemCount > 0) {
    //     SetIterator iter = SetCreateIterator(&GUI.deletedNodesWithRegisteredEvents);
    //     void* key;
    //     while(SetIteratorNext(&iter, &key)) {
    //         Node* deletedNode = (Node*)key;
    //         NU_DeleteEventsTiedToDeletedNode(deletedNode)
    //     }
    // }
}

void TriggerAllMouseupEvents(float mouseX, float mouseY, int mouseBtn)
{
    if (GUI.on_mouse_up_events.itemCount > 0)
    {
        Hashmap* hmap = &GUI.on_mouse_up_events;
        HashmapIterator it = HashmapCreateIterator(hmap);
        void* key; void* val;

        while(HashmapIteratorNext(&it, &key, &val))
        {
            // cast key, value to correct types
            Node* node = *(Node**)key; 
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;

            // if node was deleted -> skip
            //if (SetContains(&GUI.deletedNodesWithRegisteredEvents, node)) continue;

            // set calback event values and trigger
            cb_info->event.mouse.mouseBtn = mouseBtn;
            cb_info->event.mouse.mouseX = mouseX;
            cb_info->event.mouse.mouseY = mouseY;
            cb_info->event.mouse.deltaX = 0.0f;
            cb_info->event.mouse.deltaY = 0.0f;
            cb_info->event.mouse.wheelDelta = 0.0f;
            cb_info->callback(cb_info->event, cb_info->args);
        }
    }
}

void TriggerAllMouseMoveEvents(float mouseX, float mouseY, float mouseDeltaX, float mouseDeltaY)
{
    if (GUI.on_mouse_move_events.itemCount > 0)
    {
        Hashmap* hmap = &GUI.on_mouse_move_events;
        HashmapIterator it = HashmapCreateIterator(hmap);
        void* key; void* val;

        while(HashmapIteratorNext(&it, &key, &val))
        {
            // cast key, val to correct types
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;

            // set callback event values and trigger
            cb_info->event.mouse.mouseBtn = -1;
            cb_info->event.mouse.mouseX = mouseX;
            cb_info->event.mouse.mouseY = mouseY;
            cb_info->event.mouse.deltaX = mouseDeltaX;
            cb_info->event.mouse.deltaY = mouseDeltaY;
            cb_info->event.mouse.wheelDelta = 0.0f;
            cb_info->callback(cb_info->event, cb_info->args);
        }
    }
}

void TriggerAllMouseWheelEvents(float wheelDelta)
{
    if (GUI.on_mouse_wheel_events.itemCount > 0)
    {
        Hashmap* hmap = &GUI.on_mouse_wheel_events;
        HashmapIterator it = HashmapCreateIterator(hmap);
        void* key; void* val;

        while(HashmapIteratorNext(&it, &key, &val))
        {
            // cast key, value to correct types
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;

            // set calback event values and trigger
            cb_info->event.mouse.wheelDelta = wheelDelta;
            cb_info->callback(cb_info->event, cb_info->args);
        }
    }
}

void TriggerAllMouseDownOutsideEvents(float mouseX, float mouseY, int mouseBtn)
{
    if (GUI.on_mouse_down_outside_events.itemCount > 0)
    {
        Hashmap* hmap = &GUI.on_mouse_down_outside_events;
        HashmapIterator it = HashmapCreateIterator(hmap);
        void* key; void* val;

        while(HashmapIteratorNext(&it, &key, &val))
        {
            Node* node = (Node*)key;
            if (node == &GUI.hovered_node->node) continue;

            // cast key, value to correct types
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;

            // set calback event values and trigger
            cb_info->event.mouse.mouseBtn = mouseBtn;
            cb_info->event.mouse.mouseX = mouseX;
            cb_info->event.mouse.mouseY = mouseY;
            cb_info->event.mouse.deltaX = 0.0f;
            cb_info->event.mouse.deltaY = 0.0f;
            cb_info->event.mouse.wheelDelta = 0.0f;
            cb_info->callback(cb_info->event, cb_info->args);
        }
    }
}

bool EventWatcher(void* data, SDL_Event* event) 
{
    // ------------------------------------------------------------------------------------
    // --- Window closed -> main window ? close application : destroy sub window branch ---
    // ------------------------------------------------------------------------------------
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        GUI.running = false;
    }
    // ------------------------------------------------------------------------------------
    // --- Quit event -> close application ------------------------------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_QUIT) {
        GUI.running = false;
    }
    // ------------------------------------------------------------------------------------
    // --- Resized window -> redraw -------------------------------------------------------
    // ------------------------------------------------------------------------------------
    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        NU_Layout();
        CheckForResizeEvents();
        NU_Draw();
    }
    // ------------------------------------------------------------------------------------
    // --- App render event called -> redraw ----------------------------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == GUI.SDL_CUSTOM_RENDER_EVENT) {
        GUI.awaiting_redraw = true;
    }
    // ------------------------------------------------------------------------------------
    // --- Keypress -----------------------------------------------------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_KEY_DOWN) {

        // if in text edit mode
        SDL_Window* window = GetSDL_Window(&GUI.winManager, GUI.focused_node->windowID);
        if (GUI.focused_node != NULL && SDL_TextInputActive(window)) 
        {
            NodeP* inputNode = GUI.focused_node;
            NU_Font* font = Stylesheet_Get_Font(GUI.stylesheet, inputNode->fontId);
            InputText* inputText = &inputNode->typeData.input.inputText;
            SDL_Keymod mods = event->key.mod;

            bool textChanged = false;

            // backspace pressed
            if (event->key.key == SDLK_BACKSPACE) {
                // highlight + backspace
                if (InputText_IsHighlighting(inputText)) {
                    InputText_RemoveHighlightedText(inputText, inputNode, font);
                    textChanged = true; GUI.awaiting_redraw = true;
                }
                // control + backspace
                else if (mods & SDL_KMOD_CTRL && InputText_BackspaceWord(inputText, inputNode, font)) {
                    textChanged = true; GUI.awaiting_redraw = true;
                }
                // backspace
                else if (InputText_Backspace(inputText, inputNode, font)) {
                    textChanged = true; GUI.awaiting_redraw = true;
                }
            }

            // left arrow pressed
            else if (event->key.key == SDLK_LEFT) {
                // control = left arrow 
                if (mods & SDL_KMOD_CTRL && InputText_MoveCursorLeftSpan(inputText, inputNode, font)) GUI.awaiting_redraw = true;
                // only backspace
                else if (InputText_MoveCursorLeft(inputText, inputNode, font)) GUI.awaiting_redraw = true;
            }

            // right arrow pressed
            else if (event->key.key == SDLK_RIGHT) {
                // control = right arrow
                if (mods & SDL_KMOD_CTRL && InputText_MoveCursorRightSpan(inputText, inputNode, font)) GUI.awaiting_redraw = true;
                // only backspace
                else if (InputText_MoveCursorRight(inputText, inputNode, font)) GUI.awaiting_redraw = true;
            }

            // if control|command + c AND highliting text -> copy to clipboard
            if (InputText_IsHighlighting(inputText)) {
                if (event->key.key == SDLK_C && (mods & (SDL_KMOD_CTRL | SDL_KMOD_GUI))) {
                    InputText_CopyToClipboard(inputText);
                }
            }

            // if control|command + v -> paste
            if (event->key.key == SDLK_V && (mods & (SDL_KMOD_CTRL | SDL_KMOD_GUI))) {
                if (InputText_IsHighlighting(inputText)) {
                    InputText_RemoveHighlightedText(inputText, inputNode, font);
                    InputText_PasteFromClipboard(inputText, inputNode, font);
                } 
                else {
                    InputText_PasteFromClipboard(inputText, inputNode, font);
                }
                textChanged = true; GUI.awaiting_redraw = true;
            }

            // if control|command + a -> select all
            if (event->key.key == SDLK_A && (mods & (SDL_KMOD_CTRL | SDL_KMOD_GUI))) {
                InputText_SelectAll(inputText, font);
                GUI.awaiting_redraw = true;
            }

            if (textChanged) {

                // Trigger On input changed event
                if (inputNode->eventFlags & NU_EVENT_FLAG_ON_INPUT_CHANGED) {
                    Node* node = &inputNode->node;
                    void* found_cb = HashmapGet(&GUI.on_input_changed_events, &node);
                    if (found_cb != NULL) {
                        struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
                        strcpy(cb_info->event.input.text, "");
                        cb_info->callback(cb_info->event, cb_info->args);
                    }
                }
            }
        }
    }
    // ------------------------------------------------------------------------------------
    // --- Type text ----------------------------------------------------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_TEXT_INPUT) {
        NodeP* focusedNode = GUI.focused_node;
        if (focusedNode->type == NU_INPUT) {
            NU_Font* font = Stylesheet_Get_Font(GUI.stylesheet, focusedNode->fontId);
            InputText* inputText = &focusedNode->typeData.input.inputText;
            
            int updated = 0;
            if (InputText_IsHighlighting(inputText)) {
                InputText_RemoveHighlightedText(inputText, focusedNode, font);
                InputText_Write(inputText, focusedNode, font, event->text.text);
                updated = 1;
            }
            else {
                updated = InputText_Write(inputText, focusedNode, font, event->text.text);
            }
            
            if (updated && focusedNode->eventFlags & NU_EVENT_FLAG_ON_INPUT_CHANGED) {
                Node* node = &GUI.focused_node->node;
                void* found_cb = HashmapGet(&GUI.on_input_changed_events, &node);
                if (found_cb != NULL) {
                    struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
                    strcpy(cb_info->event.input.text, event->text.text);
                    cb_info->callback(cb_info->event, cb_info->args);
                }
            }
            GUI.awaiting_redraw |= updated;
        }

    }
    // ------------------------------------------------------------------------------------
    // --- Move mouse -> redraw if mouse moves off hovered node ---------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        // update hovered window
        SetHoveredWindowID(&GUI.winManager, SDL_GetWindowFromID(event->window.windowID));

        NU_Mouse_Hover();

        // get local mouse coordinates
        float mouseX, mouseY; GetLocalMouseCoords(&GUI.winManager, &mouseX, &mouseY);

        // if dragging scrollbar -> update node->node.scrollV 
        if (GUI.scroll_mouse_down_node != NULL) 
        {
            NodeP* node = GUI.scroll_mouse_down_node;

            // calculate drag_dist track_height thumb_height
            float track_height = node->node.height - node->node.borderTop - node->node.borderBottom;
            float inner_height_w_pad = track_height - node->node.padTop - node->node.padBottom;
            float inner_proportion_of_content_height = inner_height_w_pad / node->node.contentHeight;
            float thumb_height = inner_proportion_of_content_height * track_height;
            float track_top_y = node->node.y + node->node.borderTop;
            float drag_dist = (mouseY - GUI.v_scroll_thumb_grab_offset) - track_top_y;

            // apply scroll and clamp
            node->scrollV = drag_dist / (track_height - thumb_height);
            node->scrollV = min(max(node->scrollV, 0.0f), 1.0f); // Clamp to range [0,1]

            // must redraw later
            GUI.awaiting_redraw = true;
        }

        // check for mouse move events
        TriggerAllMouseMoveEvents(mouseX, mouseY, event->motion.xrel, event->motion.yrel);

        // if focused on text input -> update highlighting
        if (GUI.focused_node != NULL && GUI.focused_node->type == NU_INPUT) {
            NodeP* node = GUI.focused_node;
            NU_Font* font = Stylesheet_Get_Font(GUI.stylesheet, node->fontId);
            if (InputText_MouseDrag(&node->typeData.input.inputText, node, font, mouseX)) {
                GUI.awaiting_redraw = true;
            }
        }
    }
    // ------------------------------------------------------------------------------------
    // --- Focus on window -> redraw ------------------------------------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_WINDOW_FOCUS_GAINED)
    {
        GUI.mouse_down_node = GUI.hovered_node;
        GUI.awaiting_redraw = true;
    }
    // ------------------------------------------------------------------------------------
    // --- Mouse button pressed down ------------------------------------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        // If mouse hasn't moved yet the hovered window and node will not have been set -> set both of these
        if (GUI.winManager.hoveredWindowID == -1) {
            SetHoveredWindowID(&GUI.winManager, SDL_GetWindowFromID(event->window.windowID));
        }

        // Get mouse down coordinates
        int win_x, win_y; 
        SDL_GetGlobalMouseState(&GUI.mouseDownGlobalX, &GUI.mouseDownGlobalY);
        SDL_GetWindowPosition(GetSDL_Window(&GUI.winManager, GUI.winManager.hoveredWindowID), &win_x, &win_y);
        float mouseX = GUI.mouseDownGlobalX - win_x;
        float mouseY = GUI.mouseDownGlobalY - win_y;

        // Set mouse down node
        GUI.mouse_down_node = GUI.hovered_node;

        // If mouse is hovered over scroll thumb -> set scroll mouse down node
        if (GUI.scroll_hovered_node != NULL) 
        {    
            if (NU_Mouse_Over_Node_V_Scrollbar(GUI.scroll_hovered_node, mouseX, mouseY)) 
            {
                GUI.scroll_mouse_down_node = GUI.scroll_hovered_node;

                // Record scroll thumb grab offset
                NodeP* node = GUI.scroll_mouse_down_node;
                float track_height = node->node.height - node->node.borderTop - node->node.borderBottom;
                float inner_height_w_pad = track_height - node->node.padTop - node->node.padBottom;
                float inner_proportion_of_content_height = inner_height_w_pad / node->node.contentHeight;
                float thumb_height = inner_proportion_of_content_height * track_height;
                float thumb_top_y = node->node.y + node->node.borderTop + (node->scrollV * (track_height - thumb_height));
                GUI.v_scroll_thumb_grab_offset = mouseY - thumb_top_y;
            }
        }

        // If there is a mouse down node
        if (GUI.mouse_down_node != NULL) 
        {
            // Apply presses pseudo style
            NU_Apply_Pseudo_Style_To_Node(GUI.hovered_node, GUI.stylesheet, PSEUDO_PRESS);

            // If node has a mouse down event
            if (GUI.mouse_down_node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_DOWN)
            {
                Node* node = &GUI.mouse_down_node->node;
                void* found_cb = HashmapGet(&GUI.on_mouse_down_events, &node);
                if (found_cb != NULL) 
                {                   
                    struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
                    cb_info->event.mouse.mouseBtn = (int)event->button.button;
                    cb_info->event.mouse.mouseX = mouseX;
                    cb_info->event.mouse.mouseY = mouseY;
                    cb_info->callback(cb_info->event, cb_info->args);
                }
            }
        } 

        // Set focused node
        NodeP* prevFocusedNode = GUI.focused_node;
        GUI.focused_node = GUI.hovered_node;

        // Defocus prev focused input node
        if (prevFocusedNode != NULL && GUI.focused_node != prevFocusedNode && prevFocusedNode->type == NU_INPUT) {
            InputText_Defocus(&prevFocusedNode->typeData.input.inputText);
        }

        // Focus on node
        if (GUI.focused_node != NULL) {
            NU_Font* font = Stylesheet_Get_Font(GUI.stylesheet, GUI.focused_node->fontId);

            // Focus on input node
            if (GUI.focused_node->type == NU_INPUT) {
                InputText_Focus(&GUI.focused_node->typeData.input.inputText, GUI.focused_node, font); 
                InputText_MousePlaceCursor(&GUI.focused_node->typeData.input.inputText, GUI.focused_node, font, mouseX);
                SDL_StartTextInput(GetSDL_Window(&GUI.winManager, GUI.focused_node->windowID));
            } else {
                SDL_StopTextInput(GetSDL_Window(&GUI.winManager, GUI.focused_node->windowID));
            }
        }

        TriggerAllMouseDownOutsideEvents(mouseX, mouseY, (int)event->button.button);

        GUI.awaiting_redraw = true;
    }
    // ------------------------------------------------------------------------------------
    // --- Released mouse button ----------------------------------------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        GUI.scroll_mouse_down_node = NULL;

        SDL_GetGlobalMouseState(&GUI.mouseDownGlobalX, &GUI.mouseDownGlobalY);

        // Trigger all mouse up events
        if (GUI.winManager.hoveredWindowID != -1)
        {
            int win_x, win_y; 
            SDL_GetWindowPosition(GetSDL_Window(&GUI.winManager, GUI.winManager.hoveredWindowID), &win_x, &win_y);
            float mouseX = GUI.mouseDownGlobalX - win_x;
            float mouseY = GUI.mouseDownGlobalY - win_y;
            TriggerAllMouseupEvents(mouseX, mouseY, (int)event->button.button);
        }

        // If there is a pressed node
        if (GUI.mouse_down_node != NULL)
        {   
            // If the mouse is hovering over pressed node
            if (GUI.mouse_down_node == GUI.hovered_node) 
            { 
                // Apply hover style
                NU_Apply_Pseudo_Style_To_Node(GUI.mouse_down_node, GUI.stylesheet, PSEUDO_HOVER);
                GUI.awaiting_redraw = true;

                // Get mouse up coordinates
                int win_x, win_y; 
                SDL_GetGlobalMouseState(&GUI.mouseDownGlobalX, &GUI.mouseDownGlobalY);
                SDL_GetWindowPosition(GetSDL_Window(&GUI.winManager, GUI.winManager.hoveredWindowID), &win_x, &win_y);
                float mouseX = GUI.mouseDownGlobalX - win_x;
                float mouseY = GUI.mouseDownGlobalY - win_y;


                // If there is a click event assigned to the pressed node
                if (GUI.hovered_node->eventFlags & NU_EVENT_FLAG_ON_CLICK) 
                {
                    Node* node = &GUI.hovered_node->node;
                    void* found_cb = HashmapGet(&GUI.on_click_events, &node);
                    if (found_cb != NULL) {
                        struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
                        cb_info->event.mouse.mouseBtn = (int)event->button.button;
                        cb_info->callback(cb_info->event, cb_info->args);
                    }
                }
            }
            // If the mouse is released over something other than the pressed node -> revert pressed node to default style
            else 
            { 
                NU_Apply_Stylesheet_To_Node(GUI.mouse_down_node, GUI.stylesheet);
                GUI.awaiting_redraw = true;
            }

            // There is no longer a pressed node
            GUI.mouse_down_node = NULL;
        }

        // if focused on input text node
        if (GUI.focused_node != NULL && GUI.focused_node->type == NU_INPUT) {
            InputText_MouseUp(&GUI.focused_node->typeData.input.inputText);
            GUI.awaiting_redraw = true;
        }
    }
    // ------------------------------------------------------------------------------------
    // --- Mouse scroll wheel -------------------------------------------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_MOUSE_WHEEL)
    {
        
        NodeP* node = GUI.scroll_hovered_node;
        if (node != NULL) 
        {
            float track_height = node->node.height - node->node.borderTop - node->node.borderBottom;
            float inner_height_w_pad = track_height - node->node.padTop - node->node.padBottom;
            float inner_proportion_of_content_height = inner_height_w_pad / node->node.contentHeight;
            float thumb_height = inner_proportion_of_content_height * track_height;
            node->scrollV -= event->wheel.y * (track_height / node->node.contentHeight) * 0.2f;
            node->scrollV = min(max(node->scrollV, 0.0f), 1.0f); // Clamp to range [0,1]
            GUI.awaiting_redraw = true;
        }

        // Triggger mouse wheel events
        TriggerAllMouseWheelEvents(event->wheel.y);
    }
    return true;
}