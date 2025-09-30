#pragma

#include "nu_node_table.h"

typedef struct
{
    struct Node* node_array;
    uint32_t capacity;
    uint32_t node_count;
    uint32_t size;
} NU_Layer;

void NU_Layer_Init(NU_Layer* layer, uint32_t capacity)
{
    layer->node_array = malloc(sizeof(struct Node) * capacity);
    layer->capacity = capacity;
    layer->node_count = 0;
    layer->size = 0;
}

struct Node* NU_Layer_Get(NU_Layer* layer, uint32_t index)
{
    return &layer->node_array[index];
}

struct Node* NU_Layer_Top(NU_Layer* layer)
{
    return &layer->node_array[layer->size - 1];
}

void NU_Layer_Free(NU_Layer* layer)
{
    free(layer->node_array);
}








typedef struct
{
    NU_Layer* layers;
    NU_Node_Table node_table; // maps
    uint16_t layer_capacity;
    uint16_t layer_count;
    uint32_t node_count;
    uint32_t handle_auto_increment;
} NU_Tree;

void NU_Tree_Init(NU_Tree* tree, uint32_t nodes_per_layer, uint16_t layer_capacity)
{
    tree->layers = malloc(sizeof(NU_Layer) * layer_capacity); // Create layers array
    NU_Layer_Init(&tree->layers[0], 1); 
    for (uint16_t i=1; i<layer_capacity; i++) {                   // Init each layer
        NU_Layer_Init(&tree->layers[i], nodes_per_layer); 
    }

    // Init node table
    NU_Node_Table_Reserve(&tree->node_table, 512);

    tree->layer_capacity = layer_capacity;
    tree->layer_count = 0;
    tree->node_count = 0;
    tree->handle_auto_increment = 0;
}

void NU_Tree_Free(NU_Tree* tree)
{
    for (uint16_t i=0; i<tree->layer_capacity; i++) {
        free(tree->layers[i].node_array);
    }
    free(tree->layers);
    NU_Node_Table_Free(&tree->node_table);
}

struct Node* NU_Tree_Append(NU_Tree* tree, struct Node* node, uint32_t layer_index)
{
    NU_Layer* append_layer = &tree->layers[layer_index];

    // Grow the layer to make more space
    if (append_layer->size == append_layer->capacity) 
    {
        append_layer->capacity *= 2;
        append_layer->node_array = realloc(append_layer->node_array, sizeof(struct Node) * append_layer->capacity);

        // Update the node table
        for (uint32_t i=0; i<append_layer->size; i++)
        {
            struct Node* node = &append_layer->node_array[i];
            if (node->node_present) NU_Node_Table_Update(&tree->node_table, node->handle, node);
        }
    }

    append_layer->node_array[append_layer->size] = *node;
    append_layer->node_count += 1;
    append_layer->size += 1;
    
    struct Node* out = &append_layer->node_array[append_layer->size - 1];
    out->layer = layer_index;
    out->index = append_layer->size - 1;
    tree->node_count += 1;

    NU_Node_Table_Add(&tree->node_table, out);
    return out;
}

struct Node* NU_Tree_Get(NU_Tree* tree, uint32_t layer_index, uint32_t node_index)
{
    return &tree->layers[layer_index].node_array[node_index];
}
