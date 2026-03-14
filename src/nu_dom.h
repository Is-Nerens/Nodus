#pragma once
#include <string.h>
#include "nodus.h"

void NU_DissociateNode(NodeP* node)
{
    // Remove registered events
    if (node->eventFlags & NU_EVENT_FLAG_ON_CLICK) {
        HashmapDelete(&__NGUI.on_click_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_INPUT_CHANGED) {
        HashmapDelete(&__NGUI.on_input_changed_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_DRAG) {
        HashmapDelete(&__NGUI.on_drag_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_RELEASED) {
        HashmapDelete(&__NGUI.on_released_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_RESIZE) {
        HashmapDelete(&__NGUI.on_resize_events, &node->node);
        HashmapDelete(&__NGUI.node_resize_tracking, &node->node); 
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_DOWN) {
        HashmapDelete(&__NGUI.on_mouse_down_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_UP) {
        HashmapDelete(&__NGUI.on_mouse_up_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_DOWN_OUTSIDE) {
        HashmapDelete(&__NGUI.on_mouse_down_outside_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_MOVED) {
        HashmapDelete(&__NGUI.on_mouse_move_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_IN) {
        HashmapDelete(&__NGUI.on_mouse_in_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_OUT) {
        HashmapDelete(&__NGUI.on_mouse_out_events, &node->node);
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
        StringmapDelete(&__NGUI.id_node_map, node->id);
    }
    if (node == __NGUI.hovered_node) {
        __NGUI.hovered_node = NULL;
    } 
    if (node == __NGUI.mouse_down_node) {
        __NGUI.mouse_down_node = NULL;
    }
    if (node == __NGUI.scroll_hovered_node) {
        __NGUI.scroll_hovered_node = NULL;
    }
    if (node == __NGUI.scroll_mouse_down_node) {
        __NGUI.scroll_mouse_down_node = NULL;
    }
    if (node == __NGUI.focused_node) {
        __NGUI.focused_node = NULL;
    }
}