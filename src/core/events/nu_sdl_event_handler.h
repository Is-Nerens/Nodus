#pragma once

bool HandleEvent(SDL_Event* event)
{
    switch(event->type) {

        case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
            GUI.running = false;
            break;
        }

        case SDL_EVENT_QUIT: {
            GUI.running = false;
            break;
        }

        case SDL_EVENT_WINDOW_EXPOSED: {
            SDL_Window* sdlWindow = SDL_GetWindowFromID(event->window.windowID);
            Node* windowNode = DisplayManager_GetWindowNodeWithSDLWindow(&GUI.displayManager, sdlWindow);
            SetNodeWindowRepaint(windowNode);
            NU_Layout();
            NU_Mouse_Hover();
            NU_Draw();
            CheckForResizeEvents();
        }

        case SDL_EVENT_GAMEPAD_ADDED: {
            for (uint8 i=0; i<GUI.gamepadCapacity; i++) {
                if (GUI.gamepads[i] == NULL) {
                    GUI.gamepads[i] = SDL_OpenGamepad(event->gdevice.which);
                    TriggerAllGamepadConnectEvents(i);
                    break;
                }
            }
            break;
        }

        case SDL_EVENT_GAMEPAD_REMOVED: {
            SDL_JoystickID removedId = event->gdevice.which;
            for (uint8 i=0; i<GUI.gamepadCapacity; i++) {
                SDL_Gamepad* gamepad = GUI.gamepads[i];
                if (gamepad != NULL && SDL_GetGamepadID(gamepad) == removedId) {
                    SDL_CloseGamepad(gamepad);
                    GUI.gamepads[i] = NULL;
                    TriggerAllGamepadDisconnectEvents(i);
                    break;
                }
            }
            break;
        }
        // ------------------------------------------------------------------------------------
        // --- Window display scale changed -> redraw window content --------------------------
        // ------------------------------------------------------------------------------------
        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED: {
            SDL_Window* sdlWindow = SDL_GetWindowFromID(event->window.windowID);
            float displayScale = SDL_GetDisplayContentScale(event->window.windowID);

            // Get the window node and corresponding Window
            Node* windowNode = DisplayManager_GetWindowNodeWithSDLWindow(&GUI.displayManager, sdlWindow);
            Window* window = DisplayManager_GetNodeWindow(&GUI.displayManager, windowNode);
            SetWindowRepaint(window);
            break;
        }
        // ------------------------------------------------------------------------------------
        // --- Window moved to a different display -> re-evaluate style -----------------------
        // ------------------------------------------------------------------------------------
        case SDL_EVENT_WINDOW_DISPLAY_CHANGED: {

            SDL_Window* sdlWin = SDL_GetWindowFromID(event->window.windowID);
            Node* windowNode = DisplayManager_GetWindowNodeWithSDLWindow(&GUI.displayManager, sdlWin);
            Window* window = DisplayManager_GetNodeWindow(&GUI.displayManager, windowNode);
            Window_UpdateRenderScaleAndDisplaySize(window);
            Stylesheet_ApplyToBranch(&GUI.stylesheet, windowNode);
            SetWindowRepaint(window);
            break;
        }

        case SDL_EVENT_KEY_DOWN: {

            // Platform
            bool windows = stringEquals(SDL_GetPlatform(), "Windows");
            bool linux = stringEquals(SDL_GetPlatform(), "Linux");
            bool macOS = stringEquals(SDL_GetPlatform(), "macOS");

            if (GUI.focused_node != NULL)
            {
                // Get structs
                Node* inputNode = GUI.focused_node;
                Font* font = Stylesheet_Get_Font(&GUI.stylesheet, inputNode->fontIdx);
                TextInput* inputText = Container_Get(&GUI.textInputs, inputNode->special.input.textInputHandle);

                // Key commands
                SDL_Keymod mods = event->key.mod;
                bool controlEquivalent = (((windows || linux) && (mods & SDL_KMOD_CTRL)) || macOS && (mods & SDL_KMOD_GUI));

                // Apply stylesheet if input updated
                bool textChanged = false;

                // Backspace pressed
                if (event->key.key == SDLK_BACKSPACE)
                {
                    // Highlight + Backspace
                    if (InputText_IsHighlighting(inputText) && InputText_RemoveHighlightedText(inputText, inputNode, font)) {
                        SetNodeWindowRepaint(inputNode); textChanged = true;
                    }
                    // Control + Backspace
                    else if (controlEquivalent && InputText_BackspaceWord(inputText, inputNode, font)) {
                        SetNodeWindowRepaint(inputNode); textChanged = true;
                    }
                    // Backspace only
                    else if (InputText_Backspace(inputText, inputNode, font)) {
                        SetNodeWindowRepaint(inputNode); textChanged = true;
                    }
                }

                // Left arrow pressed
                else if (event->key.key == SDLK_LEFT) {
                    // Control + left arrow
                    if (controlEquivalent && InputText_MoveCursorLeftSpan(inputText, inputNode, font)) {
                        SetNodeWindowRepaint(inputNode);
                    }
                    // Left arrow only
                    else if (InputText_MoveCursorLeft(inputText, inputNode, font)) {
                        SetNodeWindowRepaint(inputNode);
                    }
                }

                // Right arrow pressed
                else if (event->key.key == SDLK_RIGHT) {
                    // Control + right arrow
                    if (controlEquivalent && InputText_MoveCursorRightSpan(inputText, inputNode, font)) {
                        SetNodeWindowRepaint(inputNode);
                    }
                    // Right arrow only
                    else if (InputText_MoveCursorRight(inputText, inputNode, font)) {
                        SetNodeWindowRepaint(inputNode);
                    }
                }

                // If controlEquivalent + c AND highliting text -> copy to clipboard
                if (event->key.key == SDLK_C && controlEquivalent && InputText_IsHighlighting(inputText)) {
                    InputText_CopyToClipboard(inputText);
                }

                // If controlEquivalent + v -> paste
                if (event->key.key == SDLK_V && controlEquivalent && InputText_PasteFromClipboard(inputText, inputNode, font)) {
                    SetNodeWindowRepaint(inputNode);
                    textChanged = true;
                }

                // If controlEquivalent + a -> select all
                if (event->key.key == SDLK_A && controlEquivalent) {
                    InputText_SelectAll(inputText, font);
                    SetNodeWindowRepaint(inputNode);
                }

                // Update styling
                if (textChanged) {
                    TriggerOnInputChangedEvent(inputNode, "");
                    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, GUI.focused_node, PSEUDO_FOCUS);
                }
            }

            // Toggle fullscreen with f11 (on windows and linux)
            if (event->key.key == SDLK_F11 && (windows || linux)) {
                bool fullscreen = (SDL_GetWindowFlags(GUI.displayManager.lastClickedWindow) & SDL_WINDOW_FULLSCREEN) != 0;
                SDL_SetWindowFullscreen(GUI.displayManager.lastClickedWindow, !fullscreen);
            }

            // Toggle fullscreen with control + command + f (on macOS)
            if (event->key.key == SDLK_F && macOS && (event->key.mod & SDL_KMOD_CTRL) && (event->key.mod & SDL_KMOD_GUI)) {
                bool fullscreen = (SDL_GetWindowFlags(GUI.displayManager.lastClickedWindow) & SDL_WINDOW_FULLSCREEN) != 0;
                SDL_SetWindowFullscreen(GUI.displayManager.lastClickedWindow, !fullscreen);
            }

            TriggerAllOnKeyDownEvents(event->key.key, event->key.repeat);
            break;
        }

        case SDL_EVENT_KEY_UP: {
            TriggerAllOnKeyUpEvents(event->key.key, event->key.repeat);
            break;
        }

        case SDL_EVENT_TEXT_INPUT: {

            TriggerAllOnTypeEvents(event->text.text);

            if (GUI.focused_node) {

                // Get structs
                Font* font = Stylesheet_Get_Font(&GUI.stylesheet, GUI.focused_node->fontIdx);
                TextInput* inputText = Container_Get(&GUI.textInputs, GUI.focused_node->special.input.textInputHandle);

                // Set true if text changed
                bool textChanged = false;

                // Write
                textChanged = InputText_Write(inputText, GUI.focused_node, font, event->text.text);

                // Update style
                if (textChanged) {
                    TriggerOnInputChangedEvent(GUI.focused_node, event->text.text);
                    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, GUI.focused_node, PSEUDO_FOCUS);
                }

                SetNodeWindowRepaint(GUI.focused_node);
            }

            break;
        }

        case SDL_EVENT_MOUSE_MOTION: {

            // Update hovered window
            DisplayManager_SetHoveredWindow(&GUI.displayManager, SDL_GetWindowFromID(event->window.windowID));

            // Perform hover procedure
            NU_Mouse_Hover();

            // Get local mouse coordinates
            float mouseX, mouseY;
            DisplayManager_GetLocalMouseCoords(&GUI.displayManager, &mouseX, &mouseY);

            // If dragging scrollbar -> update node->node.scrollV
            if (GUI.scroll_mouse_down_node != NULL)
            {
                // Compute scrollV
                Node* node = GUI.scroll_mouse_down_node;
                Stylesheet_Scrollbar_Style* scrollbarStyle = &GUI.stylesheet.scrollbarStyle;
                NodePublicBase* n = &node->base;
                float trackHeight = node->base.height - n->borderTop - n->borderBottom;
                float usableTrackHeight = trackHeight - scrollbarStyle->trackPadTop - scrollbarStyle->trackPadBottom;
                float scrollContentHeight = node->base.contentHeight;
                float scrollViewHeight = usableTrackHeight - n->padTop - n->padBottom;
                float scrollScaleFactor = scrollViewHeight / scrollContentHeight;
                float thumbHeight = fmaxf(scrollViewHeight / node->base.contentHeight * usableTrackHeight, scrollbarStyle->thumbMinSize);
                float scrollTravel = usableTrackHeight - thumbHeight;
                float trackY = node->base.y + n->borderTop;
                float thumbY = trackY + scrollbarStyle->trackPadTop + node->base.scrollV * scrollTravel;
                float trackTop = trackY + scrollbarStyle->trackPadTop;
                float dragDist = (mouseY - GUI.v_scroll_thumb_grab_offset) - trackTop;
                node->base.scrollV = dragDist / (usableTrackHeight - thumbHeight);
                node->base.scrollV = min(max(node->base.scrollV, 0.0f), 1.0f); // Clamp to range [0,1]
                TriggerOnScrollEvent(node);

                SetNodeWindowRepaint(GUI.scroll_mouse_down_node);
            }

            // Check for mouse move events
            TriggerAllMouseMoveEvents(mouseX, mouseY, event->motion.xrel, event->motion.yrel);

            // If focused on text input -> update highlighting
            if (GUI.focused_node != NULL) {
                Node* node = GUI.focused_node;
                Font* font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontIdx);
                TextInput* inputText = Container_Get(&GUI.textInputs, node->special.input.textInputHandle);
                if (InputText_MouseDrag(inputText, node, font, mouseX)) {
                    SetNodeWindowRepaint(node);
                }
            }
            break;
        }

        case SDL_EVENT_WINDOW_FOCUS_GAINED: {
            // Update last clicked window
            GUI.displayManager.lastClickedWindow = SDL_GetWindowFromID(event->button.windowID);
            GUI.mouse_down_node = GUI.hovered_node;
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_DOWN: {

            // Update last clicked window
            SDL_Window* sdlWin = SDL_GetWindowFromID(event->button.windowID);
            GUI.displayManager.lastClickedWindow = sdlWin;

            // Special states
            bool doubleClick = (event->button.button == SDL_BUTTON_LEFT) && (event->button.clicks == 2);

            // If mouse hasn't moved yet the hovered window and node will not have been set -> set both of these
            if (!GUI.displayManager.hoveredWindowNode) {
                DisplayManager_SetHoveredWindow(&GUI.displayManager, sdlWin);
            }

            // Get mouse down coordinates
            float mouseX; float mouseY;
            DisplayManager_GetLocalMouseCoords(&GUI.displayManager, &mouseX, &mouseY);

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
                SDL_StartTextInput(sdlWin);
                GUI.focused_node = GUI.hovered_node;
            }
            else {
                GUI.focused_node = NULL;
                SDL_StopTextInput(sdlWin);
            }

            // Place cursor on input node
            if (GUI.focused_node != NULL)
            {
                Font* font = Stylesheet_Get_Font(&GUI.stylesheet, GUI.focused_node->fontIdx);
                TextInput* inputText = Container_Get(&GUI.textInputs, GUI.focused_node->special.input.textInputHandle);

                if (GUI.focused_node != prevFocusedNode)
                {
                    InputText_MousePlaceCursor(inputText, GUI.focused_node, font, mouseX, doubleClick);
                    TriggerOnInputFocusEvent(GUI.focused_node);
                }
                else {
                    InputText_MousePlaceCursor(inputText, GUI.focused_node, font, mouseX, doubleClick);
                }
                SetNodeWindowRepaint(GUI.focused_node);
            }

            // Defocus prev focused input node
            if (prevFocusedNode != NULL && prevFocusedNode->type == NU_TYPE_INPUT && prevFocusedNode != GUI.focused_node)
            {
                TextInput* inputText = Container_Get(&GUI.textInputs, prevFocusedNode->special.input.textInputHandle);

                InputText_Defocus(inputText);

                // Trigger defocus event
                TriggerOnInputDefocusEvent(prevFocusedNode);

                // Remove focus pseudo from prev focused node
                Stylesheet_ApplyStyleToNode(&GUI.stylesheet, prevFocusedNode, PSEUDO_NONE);
                SetNodeWindowRepaint(prevFocusedNode);
            }

            // Apply PRESS pseudo style
            if (GUI.mouse_down_node && GUI.mouse_down_node != GUI.focused_node) {
                Stylesheet_ApplyStyleToNode(&GUI.stylesheet, GUI.mouse_down_node, PSEUDO_PRESS);
                SetNodeWindowRepaint(GUI.mouse_down_node);
            }

            TriggerAllMouseDownOutsideEvents(mouseX, mouseY, (int)event->button.button);
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_UP: {

            GUI.scroll_mouse_down_node = NULL;

            float mouseX; float mouseY;
            DisplayManager_GetLocalMouseCoords(&GUI.displayManager, &mouseX, &mouseY);

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
                    SetNodeWindowRepaint(GUI.hovered_node);

                    // If there is a click event assigned to the pressed node
                    TriggerOnClickEvent(GUI.hovered_node, mouseX, mouseY, (int)event->button.button);
                }

                if (GUI.mouse_down_node && GUI.mouse_down_node != GUI.focused_node) {

                    // Apply psuedo HOVER
                    if (GUI.mouse_down_node == GUI.hovered_node) {
                        Stylesheet_ApplyStyleToNode(&GUI.stylesheet, GUI.hovered_node, PSEUDO_HOVER);
                        SetNodeWindowRepaint(GUI.hovered_node);
                    }
                    // Reset style
                    else {
                         Stylesheet_ApplyStyleToNode(&GUI.stylesheet, GUI.mouse_down_node, PSEUDO_NONE);
                         SetNodeWindowRepaint(GUI.mouse_down_node);
                    }
                }

                // There is no longer a pressed node
                GUI.mouse_down_node = NULL;
            }

            // if focused on input text node
            if (GUI.focused_node != NULL) {
                TextInput* inputText = Container_Get(&GUI.textInputs, GUI.focused_node->special.input.textInputHandle);
                InputText_MouseUp(inputText);
                SetNodeWindowRepaint(GUI.focused_node);
            }
            break;
        }

        case SDL_EVENT_MOUSE_WHEEL: {

            // If scrolling
            Node* node = GUI.scroll_hovered_node;
            if (node != NULL)
            {
                NU_Mouse_Hover();

                // Update scrollV
                Stylesheet_Scrollbar_Style* scrollbarStyle = &GUI.stylesheet.scrollbarStyle;
                NodePublicBase* n = &node->base;
                float trackHeight = node->base.height - n->borderTop - n->borderBottom;
                float usableTrackHeight = trackHeight - scrollbarStyle->trackPadTop - scrollbarStyle->trackPadBottom;
                node->base.scrollV -= event->wheel.y * (usableTrackHeight / node->base.contentHeight) * 0.12f;
                node->base.scrollV = min(max(node->base.scrollV, 0.0f), 1.0f); // Clamp to range [0,1]
                TriggerOnScrollEvent(node);
                SetNodeWindowRepaint(node);
            }

            // Triggger mouse wheel events
            TriggerAllMouseWheelEvents(event->wheel.y);
            break;
        }
        default: break;
    }

    return true;
}
