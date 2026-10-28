#pragma once

bool EventWatcher(void* data, SDL_Event* event)
{
    switch(event->type) {
        // ------------------------------------------------------------------------------------
        // --- Window closed -> main window ? close application : destroy sub window branch ---
        // ------------------------------------------------------------------------------------
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
            GUI.running = false;
            break;
        }
        // ------------------------------------------------------------------------------------
        // --- Quit event -> close application ------------------------------------------------
        // ------------------------------------------------------------------------------------
        case SDL_EVENT_QUIT: {
            GUI.running = false;
            break;
        }
        // ------------------------------------------------------------------------------------
        // --- Timer event -> trigger user callback -------------------------------------------
        // ------------------------------------------------------------------------------------

        case SDL_EVENT_USER: {
            if (event->user.code == NU_EVENT_TIMER) {
                ((NU_Callback)event->user.data1)(event->user.data2);
            }
            break;
        }
        // ------------------------------------------------------------------------------------
        // --- Resized window -> redraw -------------------------------------------------------
        // ------------------------------------------------------------------------------------
        case SDL_EVENT_WINDOW_RESIZED: {
            NU_Layout();
            CheckForResizeEvents();
            NU_Draw();
            break;
        }
        // ------------------------------------------------------------------------------------
        // --- Window display scale changed -> redraw window content --------------------------
        // ------------------------------------------------------------------------------------
        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED: {
            break;
            SDL_Window* window = SDL_GetWindowFromID(event->window.windowID);
            float displayScale = SDL_GetDisplayContentScale(event->window.windowID);

            // Get the window node and corresponding Window
            Node* windowNode = GetWindowNodeFromSDL_Window(&GUI.displayManager, window);
            Window* win = DisplayManager_GetNodeWindow(&GUI.displayManager, windowNode);

            // Redraw window content
            GUI.awaiting_redraw = true;
            break;
        }
        // ------------------------------------------------------------------------------------
        // --- Window moved to a different display -> re-evaluate style -----------------------
        // ------------------------------------------------------------------------------------
        case SDL_EVENT_WINDOW_DISPLAY_CHANGED: {
            SDL_Window* window = SDL_GetWindowFromID(event->window.windowID);
            Node* windowNode = GetWindowNodeFromSDL_Window(&GUI.displayManager, window);
            Window* win = DisplayManager_GetNodeWindow(&GUI.displayManager, windowNode);

            int prevScreenWidth = win->screenWidth;
            //WindowManager_UpdateWindowRenderScale(&GUI.displayManager, windowNode->windowID);
            //WindowManager_UpdateWindowScreenDimensions(&GUI.displayManager, windowNode->windowID);

            if (prevScreenWidth != win->screenWidth) {
                Stylesheet_ApplyToBranch(&GUI.stylesheet, windowNode);
                GUI.awaiting_redraw = true;
            }

            break;
        }
        // ------------------------------------------------------------------------------------
        // --- Keypress -----------------------------------------------------------------------
        // ------------------------------------------------------------------------------------
        case SDL_EVENT_KEY_DOWN: {

            // if in text edit mode
            if (GUI.focused_node != NULL)
            {
                Node* inputNode = GUI.focused_node;
                Font* font = Stylesheet_Get_Font(&GUI.stylesheet, inputNode->fontIdx);
                InputText* inputText = Container_Get(&GUI.textInputs, inputNode->special.input.textInputHandle);
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
                    TriggerOnInputChangedEvent(inputNode, "");
                    Stylesheet_ApplyPseudoStyleToNode(&GUI.stylesheet, GUI.focused_node, PSEUDO_FOCUS);
                }
            }

            // toggle window fullscreen with f11 (on windows and linux)
            if (event->key.key == SDLK_F11 &&
                (stringEquals(SDL_GetPlatform(), "Windows") || stringEquals(SDL_GetPlatform(), "Linux")))
            {
                bool fullscreen = (SDL_GetWindowFlags(GUI.displayManager.lastClickedWindow) & SDL_WINDOW_FULLSCREEN) != 0;
                SDL_SetWindowFullscreen(GUI.displayManager.lastClickedWindow, !fullscreen);
                GUI.awaiting_redraw = true;
            }

            // toggle fullscreen with control + command + f (on macOS)
            if (event->key.key == SDLK_F &&
                stringEquals(SDL_GetPlatform(), "macOS") &&
                (event->key.mod & SDL_KMOD_CTRL) &&
                (event->key.mod & SDL_KMOD_GUI))
            {
                bool fullscreen = (SDL_GetWindowFlags(GUI.displayManager.lastClickedWindow) & SDL_WINDOW_FULLSCREEN) != 0;
                SDL_SetWindowFullscreen(GUI.displayManager.lastClickedWindow, !fullscreen);
                GUI.awaiting_redraw = true;
            }

            TriggerAllOnKeyDownEvents(event->key.key, event->key.repeat);
            break;
        }
        case SDL_EVENT_KEY_UP: {
            TriggerAllOnKeyUpEvents(event->key.key, event->key.repeat);
            break;
        }
        // ------------------------------------------------------------------------------------
        // --- Type text -> update text input if focused --------------------------------------
        // ------------------------------------------------------------------------------------
        case SDL_EVENT_TEXT_INPUT: {

            TriggerAllOnTypeEvents(event->text.text);

            if (GUI.focused_node != NULL) {
                Font* font = Stylesheet_Get_Font(&GUI.stylesheet, GUI.focused_node->fontIdx);
                InputText* inputText = Container_Get(&GUI.textInputs, GUI.focused_node->special.input.textInputHandle);

                int updated = 0;
                if (InputText_IsHighlighting(inputText)) {
                    InputText_RemoveHighlightedText(inputText, GUI.focused_node, font);
                    InputText_Write(inputText, GUI.focused_node, font, event->text.text);
                    updated = 1;
                }
                else {
                    updated = InputText_Write(inputText, GUI.focused_node, font, event->text.text);
                }

                if (updated) {
                    TriggerOnInputChangedEvent(GUI.focused_node, event->text.text);
                    Stylesheet_ApplyPseudoStyleToNode(&GUI.stylesheet, GUI.focused_node, PSEUDO_FOCUS);
                }

                GUI.awaiting_redraw |= updated;
            }

            break;
        }
        // ------------------------------------------------------------------------------------
        // --- Move mouse -> redraw if node styling changes -----------------------------------
        // ------------------------------------------------------------------------------------
        case SDL_EVENT_MOUSE_MOTION: {
            GUI.awaiting_redraw = true;
            break;

            // update hovered window
            DisplayManager_SetHoveredWindow(&GUI.displayManager, SDL_GetWindowFromID(event->window.windowID));

            NU_Mouse_Hover();

            // get local mouse coordinates
            float mouseX, mouseY; GetLocalMouseCoords(&GUI.displayManager, &mouseX, &mouseY);

            // if dragging scrollbar -> update node->node.scrollV
            if (GUI.scroll_mouse_down_node != NULL)
            {
                // Compute scrollV
                Node* node = GUI.scroll_mouse_down_node;
                Stylesheet_Scrollbar_Style* scrollbarStyle = &GUI.stylesheet.scrollbarStyle;
                NuNodeGenericAccess* n = &node->public.generic;
                float trackHeight = node->public.generic.height - n->borderTop - n->borderBottom;
                float usableTrackHeight = trackHeight - scrollbarStyle->trackPadTop - scrollbarStyle->trackPadBottom;
                float scrollContentHeight = node->public.generic.contentHeight;
                float scrollViewHeight = usableTrackHeight - n->padTop - n->padBottom;
                float scrollScaleFactor = scrollViewHeight / scrollContentHeight;
                float thumbHeight = fmaxf(scrollViewHeight / node->public.generic.contentHeight * usableTrackHeight, scrollbarStyle->thumbMinSize);
                float scrollTravel = usableTrackHeight - thumbHeight;
                float trackY = node->public.generic.y + n->borderTop;
                float thumbY = trackY + scrollbarStyle->trackPadTop + node->public.generic.scrollV * scrollTravel;
                float trackTop = trackY + scrollbarStyle->trackPadTop;
                float dragDist = (mouseY - GUI.v_scroll_thumb_grab_offset) - trackTop;
                node->public.generic.scrollV = dragDist / (usableTrackHeight - thumbHeight);
                node->public.generic.scrollV = min(max(node->public.generic.scrollV, 0.0f), 1.0f); // Clamp to range [0,1]

                TriggerOnScrollEvent(node);

                // must redraw later
                GUI.awaiting_redraw = true;
            }

            // check for mouse move events
            TriggerAllMouseMoveEvents(mouseX, mouseY, event->motion.xrel, event->motion.yrel);

            // if focused on text input -> update highlighting
            if (GUI.focused_node != NULL) {
                Node* node = GUI.focused_node;
                Font* font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontIdx);
                InputText* inputText = Container_Get(&GUI.textInputs, node->special.input.textInputHandle);
                if (InputText_MouseDrag(inputText, node, font, mouseX)) {
                    GUI.awaiting_redraw = true;
                }
            }
            break;
        }
        // ------------------------------------------------------------------------------------
        // --- Focus on window -> redraw ------------------------------------------------------
        // ------------------------------------------------------------------------------------
        case SDL_EVENT_WINDOW_FOCUS_GAINED: {
            break;
            // Update last clicked window
            GUI.displayManager.lastClickedWindow = SDL_GetWindowFromID(event->button.windowID);

            GUI.mouse_down_node = GUI.hovered_node;
            GUI.awaiting_redraw = true;
            break;
        }
        // ------------------------------------------------------------------------------------
        // --- Mouse button pressed down ------------------------------------------------------
        // ------------------------------------------------------------------------------------
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {

            break;

            // Update last clicked window
            GUI.displayManager.lastClickedWindow = SDL_GetWindowFromID(event->button.windowID);

            // If mouse hasn't moved yet the hovered window and node will not have been set -> set both of these
            if (!GUI.displayManager.hoveredWindowNode) {
                DisplayManager_SetHoveredWindow(&GUI.displayManager, SDL_GetWindowFromID(event->button.windowID));
            }

            // Get mouse down coordinates
            float mouseX; float mouseY;
            GetLocalMouseCoords(&GUI.displayManager, &mouseX, &mouseY);

            // Set mouse down node
            GUI.prev_mouse_down_node = GUI.mouse_down_node;
            GUI.mouse_down_node = GUI.hovered_node;

            // If mouse is hovered over scroll thumb -> set scroll mouse down node
            if (GUI.scroll_hovered_node != NULL)
            {
                float grabOffset;
                if (NU_Mouse_Over_Node_V_Scrollbar(GUI.scroll_hovered_node, &GUI.stylesheet.scrollbarStyle, mouseX, mouseY, &grabOffset))
                {
                    GUI.scroll_mouse_down_node = GUI.scroll_hovered_node;
                    GUI.v_scroll_thumb_grab_offset = grabOffset;
                }
            }

            // If there is a mouse down node
            if (GUI.mouse_down_node != NULL)
            {
                TriggerOnMouseDownEvent(GUI.mouse_down_node, mouseX, mouseY, (int)event->button.button);
            }

            // Update focused node and prev focused node
            Node* prevFocusedNode = GUI.focused_node;
            if (GUI.hovered_node && GUI.hovered_node->type == NU_TYPE_INPUT) {
                GUI.focused_node = GUI.hovered_node;
            }
            else {
                GUI.focused_node = NULL;
            }

            // Place cursor on input node
            if (GUI.focused_node != NULL)
            {
                Font* font = Stylesheet_Get_Font(&GUI.stylesheet, GUI.focused_node->fontIdx);
                InputText* inputText = Container_Get(&GUI.textInputs, GUI.focused_node->special.input.textInputHandle);

                if (GUI.focused_node != prevFocusedNode) {
                    InputText_MousePlaceCursor(inputText, GUI.focused_node, font, mouseX);

                    // Trigger focus event
                    TriggerOnInputFocusEvent(GUI.focused_node);
                }
                else {
                    InputText_MousePlaceCursor(inputText, GUI.focused_node, font, mouseX);
                }
                GUI.awaiting_redraw = true;
            }

            // Defocus prev focused input node
            if (prevFocusedNode != NULL && prevFocusedNode->type == NU_TYPE_INPUT && prevFocusedNode != GUI.focused_node)
            {
                InputText* inputText = Container_Get(&GUI.textInputs, prevFocusedNode->special.input.textInputHandle);

                InputText_Defocus(inputText);

                // Trigger defocus event
                TriggerOnInputDefocusEvent(prevFocusedNode);

                // Remove focus pseudo from prev focused node
                Stylesheet_ApplyStyleToNode(&GUI.stylesheet, prevFocusedNode);

                GUI.awaiting_redraw = true;
            }

            // Apply PRESS pseudo style
            if (GUI.mouse_down_node && GUI.mouse_down_node != GUI.focused_node) {
                Stylesheet_ApplyPseudoStyleToNode(&GUI.stylesheet, GUI.mouse_down_node, PSEUDO_PRESS);
                GUI.awaiting_redraw = true;
            }

            TriggerAllMouseDownOutsideEvents(mouseX, mouseY, (int)event->button.button);
            break;
        }
        // ------------------------------------------------------------------------------------
        // --- Released mouse button ----------------------------------------------------------
        // ------------------------------------------------------------------------------------
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            break;

            GUI.scroll_mouse_down_node = NULL;

            float mouseX; float mouseY;
            GetLocalMouseCoords(&GUI.displayManager, &mouseX, &mouseY);

            // Trigger all mouse up events
            if (GUI.displayManager.hoveredWindowNode) {
                TriggerAllMouseupEvents(mouseX, mouseY, (int)event->button.button);
            }

            // If there is a pressed node
            if (GUI.mouse_down_node != NULL)
            {
                // If the mouse is hovering over pressed node
                if (GUI.mouse_down_node == GUI.hovered_node)
                {
                    GUI.awaiting_redraw = true;

                    // If there is a click event assigned to the pressed node
                    TriggerOnClickEvent(GUI.hovered_node, mouseX, mouseY, (int)event->button.button);
                }

                if (GUI.mouse_down_node && GUI.mouse_down_node != GUI.focused_node) {

                    // Apply psuedo HOVER
                    if (GUI.mouse_down_node == GUI.hovered_node) {
                        Stylesheet_ApplyPseudoStyleToNode(&GUI.stylesheet, GUI.hovered_node, PSEUDO_HOVER);
                    }
                    // Reset style
                    else {
                         Stylesheet_ApplyStyleToNode(&GUI.stylesheet, GUI.mouse_down_node);
                    }

                    GUI.awaiting_redraw = true;
                }

                // There is no longer a pressed node
                GUI.mouse_down_node = NULL;
            }

            // if focused on input text node
            if (GUI.focused_node != NULL) {
                InputText* inputText = Container_Get(&GUI.textInputs, GUI.focused_node->special.input.textInputHandle);
                InputText_MouseUp(inputText);
                GUI.awaiting_redraw = true;
            }
            break;
        }
        // ------------------------------------------------------------------------------------
        // --- Mouse scroll wheel -------------------------------------------------------------
        // ------------------------------------------------------------------------------------
        case SDL_EVENT_MOUSE_WHEEL: {
            break;
            // If scrolling
            Node* node = GUI.scroll_hovered_node;
            if (node != NULL)
            {
                NU_Mouse_Hover();

                // Update scrollV
                Stylesheet_Scrollbar_Style* scrollbarStyle = &GUI.stylesheet.scrollbarStyle;
                NuNodeGenericAccess* n = &node->public.generic;
                float trackHeight = node->public.generic.height - n->borderTop - n->borderBottom;
                float usableTrackHeight = trackHeight - scrollbarStyle->trackPadTop - scrollbarStyle->trackPadBottom;
                node->public.generic.scrollV -= event->wheel.y * (usableTrackHeight / node->public.generic.contentHeight) * 0.12f;
                node->public.generic.scrollV = min(max(node->public.generic.scrollV, 0.0f), 1.0f); // Clamp to range [0,1]
                TriggerOnScrollEvent(node);
                GUI.awaiting_redraw = true;
            }

            // Triggger mouse wheel events
            TriggerAllMouseWheelEvents(event->wheel.y);
            break;
        }
        default: break;
    }

    return true;
}
