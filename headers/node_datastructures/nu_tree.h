#pragma

#include "nu_node_table.h"

typedef struct
{
    struct Node* node_array;
    uint32_t capacity;
    uint32_t node_count;
    uint32_t size;
} NU_Layer;

typedef struct
{
    NU_Layer* layers;
    NU_Node_Table node_table; // maps
    uint16_t layer_capacity;
    uint32_t node_count;
    uint32_t handle_auto_increment;
    uint32_t initial_nodes_per_layer;
} NU_Tree;

// --- Layer functions ---
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

void NU_Layer_Free(NU_Layer* layer)
{
    free(layer->node_array);
}


// --- Tree functions ---
void NU_Tree_Init(NU_Tree* tree, uint32_t nodes_per_layer, uint16_t layer_capacity)
{
    tree->layers = malloc(sizeof(NU_Layer) * layer_capacity); // Create layers array
    NU_Layer_Init(&tree->layers[0], 1); 
    for (uint16_t i=1; i<layer_capacity; i++) {                   // Init each layer
        NU_Layer_Init(&tree->layers[i], nodes_per_layer); 
    }

    // Init node table
    NU_Node_Table_Reserve(&tree->node_table, 512);

    tree->initial_nodes_per_layer = nodes_per_layer;
    tree->layer_capacity = layer_capacity;
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

void NU_Tree_Grow_Layer_Capacity(NU_Tree* tree)
{
    uint32_t prev_capacity = tree->layer_capacity;
    tree->layer_capacity *= 2;
    tree->layers = realloc(tree->layers, sizeof(NU_Layer) * tree->layer_capacity);
    for (uint16_t i=prev_capacity; i<tree->layer_capacity; i++) { // Init each new layer
        NU_Layer_Init(&tree->layers[i], tree->initial_nodes_per_layer); 
    }
}

void NU_Tree_Layer_Grow(NU_Tree* tree, NU_Layer* layer)
{
    layer->capacity *= 2;
    layer->node_array = realloc(layer->node_array, sizeof(struct Node) * layer->capacity);

    // Update the node table
    for (uint32_t i=0; i<layer->size; i++)
    {
        struct Node* node = &layer->node_array[i];
        if (node->node_present) NU_Node_Table_Update(&tree->node_table, node->handle, node);
    }
}

struct Node* NU_Tree_Append(NU_Tree* tree, struct Node* node, uint32_t layer_index)
{
    // Grow the tree's layer capacity if exceeded
    if (layer_index + 1 >= tree->layer_capacity)
    {
        NU_Tree_Grow_Layer_Capacity(tree);
    }

    
    NU_Layer* append_layer = &tree->layers[layer_index];

    // Grow the layer to make more space
    if (append_layer->size == append_layer->capacity) 
    {
        NU_Tree_Layer_Grow(tree, append_layer);
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
