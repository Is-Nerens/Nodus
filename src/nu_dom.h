#pragma once
#include <string.h>
#include "nodus.h"

void NU_DissociateNode(NodeP* node)
{
    // Remove registered events
    if (node->eventFlags & NU_EVENT_FLAG_ON_CLICK) {
        HashmapDelete(&GUI.on_click_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_INPUT_CHANGED) {
        HashmapDelete(&GUI.on_input_changed_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_DRAG) {
        HashmapDelete(&GUI.on_drag_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_RELEASED) {
        HashmapDelete(&GUI.on_released_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_RESIZE) {
        HashmapDelete(&GUI.on_resize_events, &node->node);
        HashmapDelete(&GUI.node_resize_tracking, &node->node); 
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_DOWN) {
        HashmapDelete(&GUI.on_mouse_down_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_UP) {
        HashmapDelete(&GUI.on_mouse_up_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_DOWN_OUTSIDE) {
        HashmapDelete(&GUI.on_mouse_down_outside_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_MOVED) {
        HashmapDelete(&GUI.on_mouse_move_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_IN) {
        HashmapDelete(&GUI.on_mouse_in_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_OUT) {
        HashmapDelete(&GUI.on_mouse_out_events, &node->node);
    }


    switch(node->type) {
        case NU_WINDOW:
            //SDL_DestroyWindow(node->node.window);
            break;
        case NU_CANVAS:
            NU_DeleteCanvasContext(node->typeData.canvas.ctxHandle);
            break;
        case NU_INPUT:
            InputText_Free(&node->typeData.input.inputText);
            break;
        default:
            break;
    }
    if (node->id != NULL) {
        StringmapDelete(&GUI.id_node_map, node->id);
    }
    if (node == GUI.hovered_node) {
        GUI.hovered_node = NULL;
    } 
    if (node == GUI.mouse_down_node) {
        GUI.mouse_down_node = NULL;
    }
    if (node == GUI.scroll_hovered_node) {
        GUI.scroll_hovered_node = NULL;
    }
    if (node == GUI.scroll_mouse_down_node) {
        GUI.scroll_mouse_down_node = NULL;
    }
    if (node == GUI.focused_node) {
        GUI.focused_node = NULL;
    }
}