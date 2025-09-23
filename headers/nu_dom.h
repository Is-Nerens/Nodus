#pragma once

#include <string.h>
#include "nodus.h"


// --------------------------------
// --- Node Retrieval Functions ---
// --------------------------------
struct Node* NU_Get_Node_By_Id(struct NU_GUI* gui, char* id)
{
    struct Node** found = String_Map_Get(&gui->id_node_map, id);
    return found ? *found : NULL;
}

struct Vector NU_Get_Nodes_By_Class(struct NU_GUI* gui, char* class)
{
    struct Vector result;
    Vector_Reserve(&result, sizeof(struct Node*), 8);

    // For each layer
    for (int l=0; l<=gui->deepest_layer; l++)
    {
        struct Vector* layer = &gui->tree_stack[l];
        for (int n=0; n<layer->size; n++)
        {   
            struct Node* node = Vector_Get(layer, n);
            if (node->class != NULL && strcmp(class, node->class) == 0) {
                Vector_Push(&result, &node);
            }
        }
    }

    return result;
}

struct Vector NU_Get_Nodes_By_Tag(struct NU_GUI* gui, enum Tag tag)
{
    struct Vector result;
    Vector_Reserve(&result, sizeof(struct Node*), 8);

    // For each layer
    for (int l=0; l<=gui->deepest_layer; l++)
    {
        struct Vector* layer = &gui->tree_stack[l];
        for (int n=0; n<layer->size; n++)
        {   
            struct Node* node = Vector_Get(layer, n);
            if (node->tag == tag) {
                Vector_Push(&result, &node);
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
void NU_Delete_Node(uint32_t Handle)
{

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