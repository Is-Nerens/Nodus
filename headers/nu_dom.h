#pragma once

#include <string.h>
#include "nodus.h"

struct Node* NU_Get_Node_By_Id(struct NU_GUI* gui, char* id)
{
    return *(struct Node**)String_Map_Get(&gui->id_node_map, id);
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
            if (node->class != NULL && strcmp(class, node->id) == 0) {
                Vector_Push(&result, &node);
            }
        }
    }

    return result;
}

struct Vector NU_Get_Nodes_By_Tag(struct NU_GUI* gui, enum Tag tag)
{
    struct Vector result;
    Vector_Reserve(&result, sizeof(enum Tag), 8);

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