#pragma once
#include <events/nu_events.h>

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
        if (GUI.focused_node != NULL) 
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
                TriggerOnInputChangedEvent(inputNode, "");
                NU_Apply_Pseudo_Style_To_Node(GUI.focused_node, GUI.stylesheet, PSEUDO_FOCUS);
            }
        }
    }
    // ------------------------------------------------------------------------------------
    // --- Type text ----------------------------------------------------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_TEXT_INPUT) {

        NU_Font* font = Stylesheet_Get_Font(GUI.stylesheet, GUI.focused_node->fontId);
        InputText* inputText = &GUI.focused_node->typeData.input.inputText;
        
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
            NU_Apply_Pseudo_Style_To_Node(GUI.focused_node, GUI.stylesheet, PSEUDO_FOCUS);
        }

        GUI.awaiting_redraw |= updated;
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
        if (GUI.focused_node != NULL) {
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
        GUI.prev_mouse_down_node = GUI.mouse_down_node;
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
            TriggerOnMouseDownEvent(GUI.mouse_down_node, mouseX, mouseY, (int)event->button.button);
        } 

        // Update focused node and prev focused node
        NodeP* prevFocusedNode = GUI.focused_node;
        if (GUI.hovered_node && GUI.hovered_node->type == NU_INPUT) {
            GUI.focused_node = GUI.hovered_node;
        }
        else {
            GUI.focused_node = NULL;
        }

        // Place cursor on input node
        if (GUI.focused_node != NULL) 
        {
            NU_Font* font = Stylesheet_Get_Font(GUI.stylesheet, GUI.focused_node->fontId);

            if (GUI.focused_node != prevFocusedNode) {
                InputText_MousePlaceCursor(&GUI.focused_node->typeData.input.inputText, GUI.focused_node, font, mouseX);
                SDL_StartTextInput(GetSDL_Window(&GUI.winManager, GUI.focused_node->windowID));

                // Trigger focus event
                TriggerOnInputFocusEvent(GUI.focused_node);
            }
            else {
                InputText_MousePlaceCursor(&GUI.focused_node->typeData.input.inputText, GUI.focused_node, font, mouseX);
            }
            GUI.awaiting_redraw = true;
        }

        // Defocus prev focused input node
        if (prevFocusedNode != NULL && prevFocusedNode->type == NU_INPUT && prevFocusedNode != GUI.focused_node)
        {
            InputText_Defocus(&prevFocusedNode->typeData.input.inputText);

            // Trigger defocus event
            TriggerOnInputDefocusEvent(prevFocusedNode);

            // Remove focus pseudo from prev focused node
            NU_Apply_Stylesheet_To_Node(prevFocusedNode, GUI.stylesheet);

            GUI.awaiting_redraw = true;
        }

        if (!(GUI.focused_node))
        {
            // Disable text typing events
            SDL_StopTextInput(SDL_GetWindowFromID(event->window.windowID));
        }

        // Apply PRESS pseudo style
        if (GUI.mouse_down_node && GUI.mouse_down_node != GUI.focused_node) {
            NU_Apply_Pseudo_Style_To_Node(GUI.mouse_down_node, GUI.stylesheet, PSEUDO_PRESS);
            GUI.awaiting_redraw = true;
        }

        TriggerAllMouseDownOutsideEvents(mouseX, mouseY, (int)event->button.button);
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
                GUI.awaiting_redraw = true;

                // Get mouse up coordinates
                int win_x, win_y; 
                SDL_GetGlobalMouseState(&GUI.mouseDownGlobalX, &GUI.mouseDownGlobalY);
                SDL_GetWindowPosition(GetSDL_Window(&GUI.winManager, GUI.winManager.hoveredWindowID), &win_x, &win_y);
                float mouseX = GUI.mouseDownGlobalX - win_x;
                float mouseY = GUI.mouseDownGlobalY - win_y;

                // If there is a click event assigned to the pressed node
                TriggerOnClickEvent(GUI.hovered_node, mouseX, mouseY, (int)event->button.button);
            }

            if (GUI.mouse_down_node && GUI.mouse_down_node != GUI.focused_node) {

                // Apply psuedo HOVER
                if (GUI.mouse_down_node == GUI.hovered_node) {
                    NU_Apply_Pseudo_Style_To_Node(GUI.hovered_node, GUI.stylesheet, PSEUDO_HOVER);
                }
                // Reset style
                else {
                    NU_Apply_Stylesheet_To_Node(GUI.mouse_down_node, GUI.stylesheet);
                }

                GUI.awaiting_redraw = true;
            }

            // There is no longer a pressed node
            GUI.mouse_down_node = NULL;
        }

        // if focused on input text node
        if (GUI.focused_node != NULL) {
            InputText_MouseUp(&GUI.focused_node->typeData.input.inputText);
            GUI.awaiting_redraw = true;
        }
    }
    // ------------------------------------------------------------------------------------
    // --- Mouse scroll wheel -------------------------------------------------------------
    // ------------------------------------------------------------------------------------
    else if (event->type == SDL_EVENT_MOUSE_WHEEL)
    {
        // If scrolling
        NodeP* node = GUI.scroll_hovered_node;
        if (node != NULL) 
        {
            NU_Mouse_Hover();

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