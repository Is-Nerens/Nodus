#pragma once

#include <string.h>
#include "nodus.h"



// --------------------------------
// --- Node Retrieval Functions ---
// --------------------------------
uint32_t NU_Internal_Get_Node_By_Id(char* id)
{
    uint32_t* handle = StringmapGet(&__NGUI.id_node_map, id);
    if (handle == NULL) return UINT16_MAX;
    return *handle;
}

NU_Nodelist NU_Internal_Get_Nodes_By_Class(char* class)
{
    NU_Nodelist result;
    NU_Nodelist_Init(&result, 8);

    // For each layer
    for (uint32_t l=0; l<=__NGUI.tree.depth-1; l++)
    {
        Layer* layer = &__NGUI.tree.layers[l];

        // Iterate over layer
        for (uint32_t n=0; n<layer->size; n++)
        {   
            NodeP* node = LayerGet(layer, n);
            if (!node->state) continue;

            if (node->node.class != NULL && strcmp(class, node->node.class) == 0) {
                NU_Nodelist_Push(&result, node->handle);
            }
        }
    }

    return result;
}

NU_Nodelist NU_Internal_Get_Nodes_By_Tag(NodeType type)
{
    NU_Nodelist result;
    NU_Nodelist_Init(&result, 8);

    // For each layer
    for (uint32_t l=0; l<=__NGUI.tree.depth-1; l++)
    {
        Layer* layer = &__NGUI.tree.layers[l];
        
        // Iterate over layer
        for (uint32_t n=0; n<layer->size; n++)
        {   
            NodeP* node = LayerGet(layer, n);
            if (!node->state) continue;

            if (node->type == type) {
                NU_Nodelist_Push(&result, node->handle);
            }
        }
    }

    return result;
}



// -------------------------------
// --- Node Deletion Functions ---
// -------------------------------

void NU_DissociateNode(NodeP* node)
{
    printf("dissociating node: %p\n", node);

    if (node->type == WINDOW) {
        SDL_DestroyWindow(node->node.window);
    }
    if (node->node.textContent != NULL) {
        StringArena_Delete(&__NGUI.node_text_arena, node->node.textContent); // Delete text content
    }
    if (node->node.id != NULL) {
        StringmapDelete(&__NGUI.id_node_map, node->node.id);  // Delete node from id -> handle map
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_CLICK) {
        HashmapDelete(&__NGUI.on_click_events, &node->handle);    // Delete on_click event
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_CHANGED) {
        HashmapDelete(&__NGUI.on_changed_events, &node->handle);  // Delete on_changed event
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_DRAG) {
        HashmapDelete(&__NGUI.on_drag_events, &node->handle);     // Delete on_drag event
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_RELEASED) {
        HashmapDelete(&__NGUI.on_released_events, &node->handle); // Delete on_released event
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_RESIZE) {
        HashmapDelete(&__NGUI.on_resize_events, &node->handle); // Delete on_resize event
        HashmapDelete(&__NGUI.node_resize_tracking, &node->handle); 
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_MOUSE_DOWN) {
        HashmapDelete(&__NGUI.on_mouse_down_events, &node->handle); // Delete on_mouse_down event
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_MOUSE_UP) {
        HashmapDelete(&__NGUI.on_mouse_up_events, &node->handle); // Delete on_mouse_up event
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_MOUSE_MOVED) {
        HashmapDelete(&__NGUI.on_mouse_move_events, &node->handle); // Delete on_mouse_move event
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_MOUSE_OUT) {
        HashmapDelete(&__NGUI.on_mouse_out_events, &node->handle); // Delete on_mouse_out event
    }
    if (node->handle == __NGUI.hovered_node) {
        __NGUI.hovered_node = UINT32_MAX;
    } 
    if (node->handle == __NGUI.mouse_down_node) {
        __NGUI.mouse_down_node = UINT32_MAX;
    }
    if (node->handle == __NGUI.scroll_hovered_node)
    {
        __NGUI.scroll_hovered_node = UINT32_MAX;
    }
    if (node->handle == __NGUI.scroll_mouse_down_node)
    {
        __NGUI.scroll_mouse_down_node = UINT32_MAX;
    }
    if (node->type == CANVAS) {
        HashmapDelete(&__NGUI.canvas_contexts, &node->handle); // Delete canvas context
    }
}


// ---------------------------
// --- Node Move Functions ---
// ---------------------------
void NU_Reparent_Node(NodeP* node, NodeP* new_parent)
{

}

void NU_Reorder_In_Parent(NodeP* node, uint32_t index) 
{

}






// -------------------------------------
// --- Node special property updates ---
// -------------------------------------
void NU_Internal_Set_Class(uint32_t handle, char* class)
{
    NodeP* node = NODE_P(handle);
    node->node.class = NULL;

    // Look for class in gui class string set
    char* gui_class_get = StringsetGet(&__NGUI.class_string_set, class);
    if (gui_class_get == NULL) { // Not found? Look in the stylesheet
        char* style_class_get = LinearStringsetGet(&__NGUI.stylesheet->class_string_set, class);

        // If found in the stylesheet -> add it to the gui class set
        if (style_class_get) {
            node->node.class = StringsetAdd(&__NGUI.class_string_set, class);
        }
    } 
    else {
        node->node.class = gui_class_get; 
    }

    // Update styling
    NU_Apply_Stylesheet_To_Node(node, __NGUI.stylesheet);
    if (node->handle == __NGUI.scroll_mouse_down_node) {
        NU_Apply_Pseudo_Style_To_Node(node, __NGUI.stylesheet, PSEUDO_PRESS);
    } else if (node->handle == __NGUI.hovered_node) {
        NU_Apply_Pseudo_Style_To_Node(node, __NGUI.stylesheet, PSEUDO_HOVER);
    }

    __NGUI.awaiting_redraw = true;
}

void NU_Internal_Hide(uint32_t handle)
{
    NODE(handle)->layoutFlags |= HIDDEN;
}

void NU_Internal_Show(uint32_t handle)
{
    NODE(handle)->layoutFlags &= ~HIDDEN;
}