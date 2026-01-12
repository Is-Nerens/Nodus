#pragma once

#include <string.h>
#include "nodus.h"


// -------------
// --- Debug ---
// -------------
static void NU_Verify_Tree() {


    // Check that node indices match location
    for (uint16_t l=0; l<=__NGUI.deepest_layer; l++)
    {
        NU_Layer* layer = &__NGUI.tree.layers[l];
        uint32_t layer_size = 0;
        uint32_t layer_node_count = 0;

        for (uint32_t n=0; n<layer->size; n++)
        {   
            Node* node = NU_Layer_Get(layer, n);
            layer_size++;

            if (!node->nodeState) continue;

            if (node->index != n) {
                printf("VERIFY FAIL: layer %u index mismatch at slot %u (node.index=%u)\n", l, n, node->index);
            }
            layer_node_count++;
        }

        if (layer_size != layer->size) {
            printf("VERIFY FAIL: layer %u size mismatch. (layer.size=%u) counted=%u\n", l, layer->size, layer_size);
        }
        if (layer_node_count != layer->node_count) {
            printf("VERIFY FAIL: layer %u node count mismatch. (layer.node_count=%u) counted=%u\n", l, layer->node_count, layer_node_count);
        }
    }

    // For each layer
    for (uint16_t l=0; l<=__NGUI.deepest_layer; l++)
    {
        NU_Layer* parent_layer = &__NGUI.tree.layers[l];
        NU_Layer* child_layer = &__NGUI.tree.layers[l+1];
        for (int p=0; p<parent_layer->size; p++)
        {       
            Node* parent = NU_Layer_Get(parent_layer, p);
            if (!parent->nodeState) continue;
            printf("[%u]", parent->handle);

            for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
            {
                Node* child = NU_Layer_Get(child_layer, i);

                // Check parent index
                if (child->parentIndex != p) {
                    printf("VERIFY FAIL: layer %u parent index mismatch at slot %u (node.parentIndex=%u)\n", l+1, i, child->parentIndex);
                }
            }
        }
        printf("\n");
    }
}



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
    for (uint16_t l=0; l<=__NGUI.deepest_layer; l++)
    {
        NU_Layer* layer = &__NGUI.tree.layers[l];

        // Iterate over layer
        for (uint32_t n=0; n<layer->size; n++)
        {   
            Node* node = NU_Layer_Get(layer, n);
            if (!node->nodeState) continue;

            if (node->class != NULL && strcmp(class, node->class) == 0) {
                NU_Nodelist_Push(&result, node->handle);
            }
        }
    }

    return result;
}

NU_Nodelist NU_Internal_Get_Nodes_By_Tag(enum Tag tag)
{
    NU_Nodelist result;
    NU_Nodelist_Init(&result, 8);

    // For each layer
    for (uint16_t l=0; l<=__NGUI.deepest_layer; l++)
    {
        NU_Layer* layer = &__NGUI.tree.layers[l];
        
        // Iterate over layer
        for (uint32_t n=0; n<layer->size; n++)
        {   
            Node* node = NU_Layer_Get(layer, n);
            if (!node->nodeState) continue;

            if (node->tag == tag) {
                NU_Nodelist_Push(&result, node->handle);
            }
        }
    }

    return result;
}



// -------------------------------
// --- Node Creation Functions ---
// -------------------------------
uint32_t NU_Internal_Create_Node(uint32_t parent_handle, enum Tag tag)
{
    Node* parent = NODE(parent_handle);

    // Moving deeper -> Grow tree layer capacity
    if (parent->layer == __NGUI.deepest_layer) {
        NU_Tree_Grow_Layer_Capacity(&__NGUI.tree);
        __NGUI.deepest_layer++;
    }

    // Get layer references
    NU_Layer* create_node_layer = &__NGUI.tree.layers[parent->layer + 1];
    NU_Layer* parent_layer = &__NGUI.tree.layers[parent->layer];

    create_node_layer->node_count++;
    parent->childCount++;
    if (parent->childCount > parent->childCapacity) { // Parent requires extra slab space

        // If need to expand layer
        if (create_node_layer->size + 5 > create_node_layer->capacity) {
            Node* old_node_array = create_node_layer->node_array;
            size_t old_size = create_node_layer->size;
            NU_Tree_Layer_Grow(&__NGUI.tree, create_node_layer);
        }

        // No child capacity -> Get first child insert index
        if (parent->childCapacity == 0) {
            uint16_t insert_index = 0;

            // Find nearest preceeding parent with children
            if (parent->index > 0) {
                for (int i=parent->index-1; i>=0; i--) {
                    Node* prev_parent = NU_Layer_Get(parent_layer, i);
                    if (prev_parent->nodeState && prev_parent->childCapacity > 0) {
                        insert_index = prev_parent->firstChildIndex + prev_parent->childCapacity;
                        break;
                    }
                }
            }
            parent->firstChildIndex = insert_index;
        }
        uint16_t create_node_index = parent->firstChildIndex + parent->childCount - 1;

        // Update first child indices for proceeding parent nodes
        for (uint16_t i=parent->index+1; i<parent_layer->size; i++) {
            Node* next_parent = NU_Layer_Get(parent_layer, i);
            if (next_parent->nodeState && next_parent->childCapacity > 0) next_parent->firstChildIndex += 5;
        }

        // Update node self indices and node child parent indices for proceeding layer nodes
        uint32_t shift_count = create_node_layer->size - create_node_index;
        memmove(&create_node_layer->node_array[create_node_index+5], &create_node_layer->node_array[create_node_index], shift_count * sizeof(Node));
        create_node_layer->size += 5;
        for (uint16_t i=create_node_index+5; i<create_node_layer->size; i++) {
            Node* node = NU_Layer_Get(create_node_layer, i);
            if (!node->nodeState) continue;

            // Update node index and table mapping (node->handle -> node*)
            node->index = i;
            NU_Node_Table_Update(&__NGUI.tree.node_table, node->handle, node);

            // Update parent index of node's children
            if (node->childCount > 0) {
                NU_Layer* child_layer = &__NGUI.tree.layers[parent->layer + 2];
                for (uint16_t c=node->firstChildIndex; c<node->firstChildIndex + node->childCount; c++) {
                    NU_Layer_Get(child_layer, c)->parentIndex = i;
                }
            }
        }

        // Give the parent more child capacity
        parent->childCapacity += 5;
        
        // Mark parent's new additional slots as free
        for (uint16_t i=create_node_index+1; i<create_node_index + 5; i++) {
            create_node_layer->node_array[i].nodeState = false;
        }

        // Create new node at location
        Node* created_node = &create_node_layer->node_array[create_node_index];
        NU_Apply_Node_Defaults(created_node);
        created_node->index = create_node_index;
        created_node->parentIndex = parent->index;
        created_node->layer = parent->layer + 1;
        created_node->tag = tag;
        NU_Node_Table_Add(&__NGUI.tree.node_table, created_node);
        NU_Apply_Tag_Style_To_Node(created_node, __NGUI.stylesheet);
        return created_node->handle;

    } else { // Instant creation
        Node* created_node = &create_node_layer->node_array[parent->firstChildIndex + parent->childCount - 1];
        NU_Apply_Node_Defaults(created_node);
        created_node->index = parent->firstChildIndex + parent->childCount - 1;
        created_node->parentIndex = parent->index;
        created_node->layer = parent->layer + 1;
        created_node->tag = tag;
        NU_Node_Table_Add(&__NGUI.tree.node_table, created_node);
        NU_Apply_Tag_Style_To_Node(created_node, __NGUI.stylesheet);
        return created_node->handle;
    }
}


void NU_Create_Nodes(Node* node)
{

}










// -------------------------------
// --- Node Deletion Functions ---
// -------------------------------
typedef struct {
    uint16_t start;
    uint16_t count;
    uint16_t capacity;
    uint16_t layer;
} Node_Delete_Range;

static void NU_Dissociate_Node(Node* node)
{
    if (node->tag == WINDOW) {
        SDL_DestroyWindow(node->window);
    }
    if (node->textContent != NULL) {
        StringArena_Delete(&__NGUI.node_text_arena, node->textContent); // Delete text content
    }
    if (node->id != NULL) {
        StringmapDelete(&__NGUI.id_node_map, node->id);  // Delete node from id -> handle map
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_CLICK) {
        HashmapDelete(&__NGUI.on_click_events, &node->handle);    // Delete on_click event
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_CHANGED) {
        HashmapDelete(&__NGUI.on_changed_events, &node->handle);  // Delete on_changed event
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_DRAG) {
        HashmapDelete(&__NGUI.on_drag_events, &node->handle);     // Delete on_drag event
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_RELEASED) {
        HashmapDelete(&__NGUI.on_released_events, &node->handle); // Delete on_released event
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_RESIZE) {
        HashmapDelete(&__NGUI.on_resize_events, &node->handle); // Delete on_resize event
        HashmapDelete(&__NGUI.node_resize_tracking, &node->handle); 
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_DOWN) {
        HashmapDelete(&__NGUI.on_mouse_down_events, &node->handle); // Delete on_mouse_down event
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_UP) {
        HashmapDelete(&__NGUI.on_mouse_up_events, &node->handle); // Delete on_mouse_up event
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_MOVED) {
        HashmapDelete(&__NGUI.on_mouse_move_events, &node->handle); // Delete on_mouse_move event
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_OUT) {
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
    if (node->tag == CANVAS) {
        HashmapDelete(&__NGUI.canvas_contexts, &node->handle); // Delete canvas context
    }
}

static void NU_Delete_Childless(Node* node)
{
    // --------------------------
    // --- Dissociate -----------
    // --------------------------
    NU_Dissociate_Node(node);
    NU_Node_Table_Delete(&__NGUI.tree.node_table, node->handle);

    uint32_t node_index = node->index;
    uint32_t node_layer = node->layer;
    NU_Layer* layer = &__NGUI.tree.layers[node_layer]; 
    NU_Layer* parent_layer = &__NGUI.tree.layers[node_layer-1];
    Node* parent = NU_Layer_Get(parent_layer, node->parentIndex);
    node->nodeState = false;
    uint32_t layer_node_count = layer->node_count;
    layer->node_count--;

    // Case 1: Is the only node in the layer
    if (layer_node_count == 1) {
        layer->node_count = 0;
        parent->childCount = 0;
        parent->firstChildIndex = UINT16_MAX;
        __NGUI.deepest_layer = node_layer - 1;
        return;
    }

    // Case 2: has siblings -> shift siblings that come after the node backwards to fill gap
    if (parent->childCount > 1) 
    {
        // Calculate number of siblings to shift backwards
        uint32_t node_index_in_parent = node_index - parent->firstChildIndex;
        uint32_t siblings_to_shift = parent->childCount - node_index_in_parent - 1;

        // Shift siblings one slot left
        for (uint32_t i=0; i<siblings_to_shift; i++) {
            Node* sib = &layer->node_array[node_index + i + 1];    // node being moved
            Node* dest = &layer->node_array[node_index + i];       // destination
            *dest = *sib;                                                 // Copy contents
            dest->index = node_index + i;                                 // Update index
            sib->nodeState = false;                                    // Update old position presence
            dest->nodeState = true;                                    // Update new position presence
            NU_Node_Table_Set(&__NGUI.tree.node_table, dest->handle, dest); // Update table mapping

            // Update the parent index of sibling's children
            NU_Layer* sibling_child_layer = &__NGUI.tree.layers[node_layer+1];
            if (dest->childCount > 0) {
                for (uint32_t c=dest->firstChildIndex; c<dest->firstChildIndex + dest->childCount; c++) {
                    NU_Layer_Get(sibling_child_layer, c)->parentIndex = dest->index;
                }
            }
        }
        parent->childCount--;
        return;
    }

    // Case 3: No siblings -> shift next node group back to claim space
    // Probe until find next sibling group
    uint32_t next_group_first_child_index = parent->firstChildIndex + parent->childCapacity;
    Node* next_group_first_node = NU_Layer_Get(layer, next_group_first_child_index);
    Node* next_group_parent = NU_Layer_Get(parent_layer, next_group_first_node->parentIndex);

    // If next group is NOT last group in layer -> allocate extra capacity to next group
    if (!(next_group_parent->firstChildIndex + next_group_parent->childCount == layer_node_count)) {
        next_group_parent->childCapacity += parent->childCapacity;
        layer->size -= 1;
    }
    next_group_parent->firstChildIndex = node_index;

    // Shift next sibling group backwards to fill gap
    for (uint16_t s=0; s<next_group_parent->childCount; s++) {
        Node* sib = NU_Layer_Get(layer, next_group_first_child_index + s); // node being moved
        Node* dest = NU_Layer_Get(layer, node_index + s);                  // destination
        *dest = *sib;                                                             // Copy contents
        dest->index = node_index + s;                                             // Update index
        dest->nodeState = true;                                                // Update new position presence
        sib->nodeState = false;                                                // Update old position presence
        NU_Node_Table_Set(&__NGUI.tree.node_table, dest->handle, dest);             // Update table mapping

        // Update the parent index of this sibling's children
        NU_Layer* sibling_child_layer = &__NGUI.tree.layers[node_layer+1];
        if (dest->childCount > 0) {
            for (uint16_t c=dest->firstChildIndex; c<dest->firstChildIndex + dest->childCount; c++) {
                NU_Layer_Get(sibling_child_layer, c)->parentIndex = dest->index;
            }
        }
    }
    parent->childCount = 0;
}

static void NU_Delete_Node_Branch(Node* node)
{
    // -------------------------------------------------------------------------------
    // --- Traverse node branch and construct list of ranges (bottom-up) -------------
    // -------------------------------------------------------------------------------
    struct Vector delete_ranges;
    struct Vector visit_stack;
    Vector_Reserve(&delete_ranges, sizeof(Node_Delete_Range), 64);
    Vector_Reserve(&visit_stack, sizeof(Node_Delete_Range), 32);
    NU_Layer* parent_layer = &__NGUI.tree.layers[node->layer-1];
    Node* parent = NU_Layer_Get(parent_layer, node->parentIndex);
    Node_Delete_Range root_range = { node->index, 1, parent->childCapacity, node->layer };
    Vector_Push(&visit_stack, &root_range);
    while (visit_stack.size > 0) 
    {
        Node_Delete_Range range = *(Node_Delete_Range*)Vector_Get(&visit_stack, visit_stack.size - 1);
        visit_stack.size--;
        bool has_children = false;
        for (uint16_t i=0; i<range.count; i++) 
        {
            Node* current_node = NU_Tree_Get(&__NGUI.tree, range.layer, range.start + i);
            if (current_node->childCount > 0) 
            {
                Node_Delete_Range child_range = {
                    current_node->firstChildIndex,
                    current_node->childCount,
                    current_node->childCapacity,
                    current_node->layer + 1
                };
                Vector_Push(&visit_stack, &child_range);
                has_children = true;
            }
        }
        Vector_Push(&delete_ranges, &range);
    }

    // -----------------------------------------------
    // --- Dissociate and free nodes in ranges -------
    // -----------------------------------------------
    for (uint32_t i=1; i<delete_ranges.size; i++)
    {
        Node_Delete_Range range = *(Node_Delete_Range*)Vector_Get(&delete_ranges, i);
        for (uint32_t n=range.start; n<range.start + range.count; n++)
        {
            Node* delete_node = NU_Tree_Get(&__NGUI.tree, range.layer, n);
            delete_node->nodeState = false;
            NU_Dissociate_Node(delete_node);
            NU_Node_Table_Delete(&__NGUI.tree.node_table, delete_node->handle);
        }
    }

    // -----------------------------------------------------
    // --- Allocate spaces to adjacent group parents -------
    // -----------------------------------------------------
    for (uint32_t i=delete_ranges.size; i-->1; )
    {
        Node_Delete_Range range = *(Node_Delete_Range*)Vector_Get(&delete_ranges, i);
        NU_Layer* layer = &__NGUI.tree.layers[range.layer];

        // Case 1: Is only sibling group in layer
        if (layer->node_count == range.count) {
            __NGUI.deepest_layer = range.layer - 1;
            layer->node_count = 0;
            layer->size = 0;
            continue;
        }

        // Case 2: Is last sibling group in layer -> reclaim space at end of layer
        if (range.start + range.capacity == layer->size) {
            layer->size -= range.capacity;
            layer->node_count -= range.count;
            continue;
        }

        // Case 3: There is a sibling group to the right
        uint32_t next_group_first_idx = range.start + range.capacity;
        Node* next_group_first_node = NU_Layer_Get(layer, next_group_first_idx);
        NU_Layer* next_group_parent_layer = &__NGUI.tree.layers[range.layer - 1];
        Node* next_group_parent = NU_Layer_Get(next_group_parent_layer, next_group_first_node->parentIndex);

        // Reduce the layer size if next group is the final group
        if (next_group_parent->firstChildIndex + next_group_parent->childCapacity == layer->size) {
            layer->size -= range.count;
        }

        // Move next group backwards to fill the gap
        next_group_parent->firstChildIndex = range.start;
        next_group_parent->childCapacity += range.capacity;
        for (uint32_t s = 0; s < next_group_parent->childCount; s++) {
            uint32_t src_idx = next_group_first_idx + s;
            uint32_t dst_idx = range.start + s;
            Node* sib = NU_Layer_Get(layer, src_idx);
            Node* dest = NU_Layer_Get(layer, dst_idx);
            *dest = *sib;                   // Copy the struct
            dest->index = dst_idx;          // Fix index (FIX: use dst_idx, not range.start + s)
            dest->nodeState = true;
            sib->nodeState = false;
            NU_Node_Table_Set(&__NGUI.tree.node_table, dest->handle, dest);

            // Update the parent index of this sibling's children
            NU_Layer* sibling_child_layer = &__NGUI.tree.layers[range.layer + 1];
            if (dest->childCount > 0 && sibling_child_layer->size > 0) {
                for (uint16_t c = dest->firstChildIndex; c < dest->firstChildIndex + dest->childCount; c++) {
                    Node* child = NU_Layer_Get(sibling_child_layer, c);
                    if (child) {
                        child->parentIndex = dst_idx;  // FIX: use dst_idx
                    }
                }
            }
        }

        // Update the following range items in this layer
        for (uint32_t j=i; j-->1; ) {
            Node_Delete_Range* other = (Node_Delete_Range*)Vector_Get(&delete_ranges, j);
            if (other->layer == range.layer && other->start > range.start) {
                other->start -= range.capacity;
            }
        }

        layer->node_count -= range.count;
    }

    // ---------------------------------------
    // --- Delete the root delete node -------
    // ---------------------------------------
    NU_Delete_Childless(node);

    // --------------------
    // --- Free vectors ---
    // --------------------
    Vector_Free(&visit_stack);
    Vector_Free(&delete_ranges);
}

void NU_Internal_Delete_Node(uint32_t handle)
{
    Node* node = NODE(handle);

    // Case 1: Deleting root node (Cannot do this!)
    if (node->layer == 0) return;


    __NGUI.awaiting_redraw = true;
    

    // ----------------------------------------------
    // --- Case 2: Deleting node with no children ---
    // ----------------------------------------------
    if (node->childCount == 0) 
    {
        // ----------------------------------
        // --- Remove node from tree --------
        // ----------------------------------
        NU_Delete_Childless(node);
        if (__NGUI.tree.layers[__NGUI.deepest_layer].node_count == 0) {
            __NGUI.deepest_layer--;
        }
        return;
    }

    // ----------------------------------------------
    // --- Case 2: Deleting node with children ---
    // ----------------------------------------------
    NU_Delete_Node_Branch(node);
    return;
}


// ---------------------------
// --- Node Move Functions ---
// ---------------------------
void NU_Reparent_Node(Node* node, Node* new_parent)
{

}

void NU_Reorder_In_Parent(Node* node, uint32_t index) 
{

}






// -------------------------------------
// --- Node special property updates ---
// -------------------------------------
void NU_Internal_Set_Class(uint32_t handle, char* class)
{
    Node* node = NODE(handle);
     node->class = NULL;

    // Look for class in gui class string set
    char* gui_class_get = StringsetGet(&__NGUI.class_string_set, class);
    if (gui_class_get == NULL) { // Not found? Look in the stylesheet
        char* style_class_get = LinearStringsetGet(&__NGUI.stylesheet->class_string_set, class);

        // If found in the stylesheet -> add it to the gui class set
        if (style_class_get) {
            node->class = StringsetAdd(&__NGUI.class_string_set, class);
        }
    } 
    else {
        node->class = gui_class_get; 
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