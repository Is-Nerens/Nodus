#pragma once
#include <string.h>
#include "nodus.h"

void NU_DissociateNode(NodeP* node)
{
    if (node->type == WINDOW) {
        SDL_DestroyWindow(node->node.window);
    }
    if (node->node.textContent != NULL) {
        StringArena_Delete(&__NGUI.node_text_arena, node->node.textContent);
    }
    if (node->node.id != NULL) {
        StringmapDelete(&__NGUI.id_node_map, node->node.id);
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_CLICK) {
        HashmapDelete(&__NGUI.on_click_events, &node->handle);
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_INPUT_CHANGED) {
        HashmapDelete(&__NGUI.on_input_changed_events, &node->handle);
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_DRAG) {
        HashmapDelete(&__NGUI.on_drag_events, &node->handle);
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_RELEASED) {
        HashmapDelete(&__NGUI.on_released_events, &node->handle);
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_RESIZE) {
        HashmapDelete(&__NGUI.on_resize_events, &node->handle);
        HashmapDelete(&__NGUI.node_resize_tracking, &node->handle); 
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_MOUSE_DOWN) {
        HashmapDelete(&__NGUI.on_mouse_down_events, &node->handle);
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_MOUSE_UP) {
        HashmapDelete(&__NGUI.on_mouse_up_events, &node->handle);
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_MOUSE_MOVED) {
        HashmapDelete(&__NGUI.on_mouse_move_events, &node->handle);
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_MOUSE_OUT) {
        HashmapDelete(&__NGUI.on_mouse_out_events, &node->handle);
    }
    if (node->handle == __NGUI.hovered_node) {
        __NGUI.hovered_node = UINT32_MAX;
    } 
    if (node->handle == __NGUI.mouse_down_node) {
        __NGUI.mouse_down_node = UINT32_MAX;
    }
    if (node->handle == __NGUI.scroll_hovered_node) {
        __NGUI.scroll_hovered_node = UINT32_MAX;
    }
    if (node->handle == __NGUI.scroll_mouse_down_node) {
        __NGUI.scroll_mouse_down_node = UINT32_MAX;
    }
    if (node->type == CANVAS) {
        HashmapDelete(&__NGUI.canvas_contexts, &node->handle);
    }
}

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

void NU_Reparent_Node(NodeP* node, NodeP* new_parent)
{

}

void NU_Reorder_In_Parent(NodeP* node, uint32_t index) 
{

}

inline Node* NODE(u32 nodeHandle)
{
    NodeP* nodeP = NodeTableGet(&__NGUI.tree.table, nodeHandle);
    if (nodeP == NULL) return NULL;
    return &nodeP->node; 
}

inline u32 PARENT(u32 nodeHandle)
{
    NodeP* nodeP = NodeTableGet(&__NGUI.tree.table, nodeHandle);
    if (nodeP == NULL) return UINT32_MAX;
    return nodeP->parentHandle;
}

inline u32 CHILD(u32 nodeHandle, u32 childIndex)
{
    NodeP* nodeP = NodeTableGet(&__NGUI.tree.table, nodeHandle);
    if (nodeP == NULL || childIndex >= nodeP->childCount) return UINT32_MAX;
    NodeP* child = &__NGUI.tree.layers[nodeP->layer+1].nodeArray[nodeP->firstChildIndex + childIndex];
    return child->handle;
}

inline u32 CHILD_COUNT(u32 nodeHandle)
{
    NodeP* nodeP = NodeTableGet(&__NGUI.tree.table, nodeHandle);
    if (nodeP == NULL) return UINT32_MAX;
    return nodeP->childCount;
}

inline u32 DEPTH(u32 nodeHandle)
{
    NodeP* nodeP = NodeTableGet(&__NGUI.tree.table, nodeHandle);
    if (nodeP == NULL) return UINT32_MAX;
    return nodeP->layer;
}

inline u32 CREATE_NODE(u32 parentHandle, NodeType type)
{
    if (type == WINDOW) return UINT32_MAX; // Nodus doesn't yet support window creation
    u32 nodeHandle = TreeCreateNode(&__NGUI.tree, parentHandle, type);
    NodeP* node = NodeTableGet(&__NGUI.tree.table, nodeHandle);
    NU_ApplyNodeDefaults(node);
    NU_Apply_Stylesheet_To_Node(node, __NGUI.stylesheet);
    return nodeHandle;
}

inline void DELETE_NODE(u32 nodeHandle)
{
    NodeP* nodeP = NodeTableGet(&__NGUI.tree.table, nodeHandle);
    if (nodeP == NULL) return;
    return TreeDeleteNode(&__NGUI.tree, nodeHandle, NU_DissociateNode);
}

inline void SHOW(u32 nodeHandle)
{
    NODE(nodeHandle)->layoutFlags &= ~HIDDEN;
}

inline void HIDE(u32 nodeHandle)
{
    NODE(nodeHandle)->layoutFlags |= HIDDEN;
}