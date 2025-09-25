#pragma once

#include <string.h>
#include "nodus.h"


// --------------------------------
// --- Node Retrieval Functions ---
// --------------------------------
uint32_t NU_Get_Node_By_Id(struct NU_GUI* gui, char* id)
{
    uint32_t* handle = String_Map_Get(&gui->id_node_map, id);
    if (handle == NULL) return UINT32_MAX;
    return *handle;
}

struct Vector NU_Get_Nodes_By_Class(struct NU_GUI* gui, char* class)
{
    struct Vector result;
    Vector_Reserve(&result, sizeof(uint32_t), 8);

    // For each layer
    for (uint16_t l=0; l<=gui->deepest_layer; l++)
    {
        NU_Layer* layer = &gui->tree.layers[l];

        // Iterate over layer
        for (uint32_t n=0; n<layer->size; n++)
        {   
            struct Node* node = NU_Layer_Get(layer, n);
            if (!node->node_present) continue;

            if (node->class != NULL && strcmp(class, node->class) == 0) {
                Vector_Push(&result, &node->handle);
            }
        }
    }

    return result;
}

struct Vector NU_Get_Nodes_By_Tag(struct NU_GUI* gui, enum Tag tag)
{
    struct Vector result;
    Vector_Reserve(&result, sizeof(uint32_t), 8);

    // For each layer
    for (uint16_t l=0; l<=gui->deepest_layer; l++)
    {
        NU_Layer* layer = &gui->tree.layers[l];
        
        // Iterate over layer
        for (uint32_t n=0; n<layer->size; n++)
        {   
            struct Node* node = NU_Layer_Get(layer, n);
            if (!node->node_present) continue;

            if (node->tag == tag) {
                Vector_Push(&result, &node->handle);
            }
        }
    }

    return result;
}



// -------------------------------
// --- Node Creation Functions ---
// -------------------------------
struct Node* NU_Create_Node(struct Node* parent)
{
    struct Node* node;
    return node;
}

void NU_Create_Nodes(struct Node* node)
{

}




// -------------------------------
// --- Node Deletion Functions ---
// -------------------------------
void NU_Delete_Node(struct NU_GUI* gui, uint32_t handle)
{
    struct Node* node = NODE(gui, handle);

    // Case 1: Deleting root node (Cannot do this!)
    if (node->layer == 0) return;

    // ----------------------------------------------
    // --- Case 2: Deleting node with no children ---
    // ----------------------------------------------
    if (node->child_count == 0 && node->tag != WINDOW) 
    {
        // ----------------------------------------
        // --- Delete strings tied to node --------
        // ----------------------------------------
        if (node->text_content != NULL) {
            StringArena_Delete(&gui->node_text_arena, node->text_content); // Delete text content
        }
        if (node->id != NULL) {
            String_Map_Delete(&gui->id_node_map, node->id);  // Delete node from id -> handle map
        }
        if (node->event_flags & NU_EVENT_FLAG_ON_CLICK) {
            Hashmap_Delete(&gui->on_click_events, &node);    // Delete on_click event
        }
        if (node->event_flags & NU_EVENT_FLAG_ON_CHANGED) {
            Hashmap_Delete(&gui->on_changed_events, &node);  // Delete on_changed event
        }
        if (node->event_flags & NU_EVENT_FLAG_ON_DRAG) {
            Hashmap_Delete(&gui->on_drag_events, &node);     // Delete on_drag event
        }
        if (node->event_flags & NU_EVENT_FLAG_ON_RELEASED) {
            Hashmap_Delete(&gui->on_released_events, &node); // Delete on_released event
        }
        if (node == gui->hovered_node) {
            gui->hovered_node = NULL;
        } 
        if (node == gui->mouse_down_node) {
            gui->mouse_down_node = NULL;
        }

        // ----------------------------------
        // --- Remove node from tree --------
        // ----------------------------------
        NU_Tree_Delete_Childless(&gui->tree, node);
        if (gui->tree.layers[gui->deepest_layer].node_count == 0) {
            gui->deepest_layer--;
        }

        // Set await draw
        gui->awaiting_draw = true;
        return;
    }

    // Case 3: Deleting node with children (delete children from bottom up)
    return;
}

void NU_Delete_Nodes()
{

}



// ---------------------------
// --- Node Move Functions ---
// ---------------------------
void NU_Reparent_Node(struct Node* node, struct Node* new_parent)
{

}

void NU_Swap_Nodes(struct Node* node_a, struct Node* node_b)
{

}

void NU_Insert_Node(struct Node* node, struct Node* parent, int index)
{

}