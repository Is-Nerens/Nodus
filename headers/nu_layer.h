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

void NU_Layer_Free(NU_Layer* layer)
{
    free(layer->node_array);
}

struct Node* NU_Layer_Get(NU_Layer* layer, uint32_t index)
{
    return &layer->node_array[index];
}

struct Node* NU_Layer_Top(NU_Layer* layer)
{
    return &layer->node_array[layer->size - 1];
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

// This function is exclusively used when constructing a GUI from am XML file
// As it assumes nodes are packed tightly in the layers and the layers are appended to only (the layers are acting like linear allocators)
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

void NU_Tree_Delete_Childless(NU_Tree* tree, struct Node* node)
{
    // Remove node from table
    NU_Node_Table_Delete(&tree->node_table, node->handle);

    uint32_t node_layer = node->layer;
    NU_Layer* parent_layer = &tree->layers[node->layer-1];
    NU_Layer* layer = &tree->layers[node_layer]; 
    struct Node* parent  = NU_Layer_Get(parent_layer, node->parent_index);
    uint32_t delete_idx  = node->index;
    uint32_t first_idx   = parent->first_child_index;
    uint32_t child_idx   = delete_idx - first_idx;
    uint32_t siblings_to_shift = parent->child_count - child_idx - 1;

    // Shift siblings one slot left
    for (uint32_t i=0; i<siblings_to_shift; i++) {
        struct Node* old_sibling = &layer->node_array[delete_idx + i + 1]; // node being moved
        struct Node* new_sibling = &layer->node_array[delete_idx + i];     // destination
        uint32_t sibling_handle = old_sibling->handle;        // save handle before overwrite
        *new_sibling = *old_sibling;                          // move struct
        new_sibling->index = delete_idx + i;                  // fix index
        NU_Node_Table_Update(&tree->node_table, sibling_handle, new_sibling);
    }
    struct Node* last_slot = &layer->node_array[delete_idx + siblings_to_shift];
    last_slot->node_present = 0;
    parent->child_count--;

    // Update layer state
    layer->node_count--;
    if (last_slot->index == layer->size) { // shrink layer size if deleted node belonged to last sibling group in layer
        layer->size--;
    }
}