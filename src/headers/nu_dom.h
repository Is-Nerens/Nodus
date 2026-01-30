#pragma once
#include <string.h>
#include "nodus.h"

void NU_DissociateNode(NodeP* node)
{
    if (node->type == NU_WINDOW) {
        SDL_DestroyWindow(node->node.window);
    }
    if (node->node.textContent != NULL) {
        StringArena_Delete(&__NGUI.node_text_arena, node->node.textContent);
    }
    if (node->node.id != NULL) {
        StringmapDelete(&__NGUI.id_node_map, node->node.id);
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_CLICK) {
        HashmapDelete(&__NGUI.on_click_events, &node);
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_INPUT_CHANGED) {
        HashmapDelete(&__NGUI.on_input_changed_events, &node);
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_DRAG) {
        HashmapDelete(&__NGUI.on_drag_events, &node);
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_RELEASED) {
        HashmapDelete(&__NGUI.on_released_events, &node);
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_RESIZE) {
        HashmapDelete(&__NGUI.on_resize_events, &node);
        HashmapDelete(&__NGUI.node_resize_tracking, &node); 
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_MOUSE_DOWN) {
        HashmapDelete(&__NGUI.on_mouse_down_events, &node);
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_MOUSE_UP) {
        HashmapDelete(&__NGUI.on_mouse_up_events, &node);
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_MOUSE_MOVED) {
        HashmapDelete(&__NGUI.on_mouse_move_events, &node);
    }
    if (node->node.eventFlags & NU_EVENT_FLAG_ON_MOUSE_OUT) {
        HashmapDelete(&__NGUI.on_mouse_out_events, &node);
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
    if (node->type == NU_CANVAS) {
        HashmapDelete(&__NGUI.canvas_contexts, &node->node);
    }
}

Node* NU_Internal_Get_Node_By_Id(char* id)
{
    void* found = StringmapGet(&__NGUI.id_node_map, id);
    if (found == NULL) return NULL;
    NodeP* node = *(NodeP**)found;
    return &node->node;
}

NU_Nodelist NU_Internal_Get_Nodes_By_Class(char* class)
{
    NU_Nodelist result;
    NU_Nodelist_Init(&result, 8);
    DepthFirstSearch dfs = DepthFirstSearch_Create(__NGUI.tree.root);
    NodeP* node;
    while(DepthFirstSearch_Next(&dfs, &node)) {
        if (node->node.class != NULL && strcmp(class, node->node.class) == 0) {
            NU_Nodelist_Push(&result, &node->node);
        }
    }
    return result;
}

NU_Nodelist NU_Internal_Get_Nodes_By_Tag(NodeType type)
{
    NU_Nodelist result;
    NU_Nodelist_Init(&result, 8);
    DepthFirstSearch dfs = DepthFirstSearch_Create(__NGUI.tree.root);
    NodeP* node;
    while(DepthFirstSearch_Next(&dfs, &node)) {
        if (node->type == type) {
            NU_Nodelist_Push(&result, &node->node);
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

inline Node* PARENT(Node* node)
{
    NodeP* nodeP = NODEP_OF(node, NodeP, node); // clever macro stuff
    if (nodeP->parent == NULL) return NULL;
    return &nodeP->parent->node;
}

inline Node* CHILD(Node* node, u32 childIndex)
{
    NodeP* nodeP = NODEP_OF(node, NodeP, node); // clever macro stuff
    if (nodeP == NULL || childIndex >= nodeP->childCount) return NULL;
    NodeP* child = nodeP->firstChild;
    u32 i = 0;
    while(child != NULL) {
        if (i == childIndex) return &child->node;
        i++;
        child = child->nextSibling;
    }
    return NULL;
}

inline u32 CHILD_COUNT(Node* node)
{
    NodeP* nodeP = NODEP_OF(node, NodeP, node); // clever macro stuff
    return nodeP->childCount;
}

inline int DEPTH(Node* node)
{
    NodeP* nodeP = NODEP_OF(node, NodeP, node); // clever macro stuff
    return (int)(nodeP->layer);
}

inline Node* CREATE_NODE(Node* parent, NodeType type)
{ 
    if (parent == NULL || type == NU_WINDOW) return NULL; // Nodus doesn't yet support window creation
    NodeP* parentP = NODEP_OF(parent, NodeP, node); // clever macro stuff
    NodeP* node = TreeCreateNode(&__NGUI.tree, parentP, type);
    NU_Apply_Stylesheet_To_Node(node, __NGUI.stylesheet);
    return &node->node;
}

inline void DELETE_NODE(Node* node)
{
    NodeP* nodeP = NODEP_OF(node, NodeP, node); // clever macro stuff
    return TreeDeleteNode(&__NGUI.tree, nodeP, NU_DissociateNode);
}

inline const char* INPUT_TEXT_CONTENT(Node* node)
{
    NodeP* nodeP = NODEP_OF(node, NodeP, node); // clever macro stuff
    if (nodeP->type != NU_INPUT) return NULL;
    return nodeP->typeData.input.inputText.buffer;
}

inline void SHOW(Node* node)
{
    node->layoutFlags &= ~HIDDEN;
}

inline void HIDE(Node* node)
{
    node->layoutFlags |= HIDDEN;
}