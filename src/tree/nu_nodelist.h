#pragma once
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct NU_Nodelist {
    size_t count;
    Node** nodes;
} NU_Nodelist;

typedef struct NU_Nodelist_Internal
{
    NU_Nodelist nodelist;
    size_t capacity;
} NU_Nodelist_Internal;

void NU_Nodelist_Init(NU_Nodelist_Internal* nodelist, size_t capacity)
{
    capacity = MAX(capacity, 5);
    nodelist->capacity = capacity;
    nodelist->nodelist.count = 0;
    nodelist->nodelist.nodes = malloc(capacity * sizeof(Node*));
}

void NU_Nodelist_Grow(NU_Nodelist_Internal* nodelist)
{
    nodelist->capacity = MAX(nodelist->capacity * 2, 2);
    nodelist->nodelist.nodes = realloc(
        nodelist->nodelist.nodes,
        nodelist->capacity * sizeof(Node*)
    );
}

void NU_Nodelist_Push(NU_Nodelist_Internal* nodelist, Node* node)
{
    if (nodelist->nodelist.count == nodelist->capacity) {
        NU_Nodelist_Grow(nodelist);
    }

    void* dst = (char*)nodelist->nodelist.nodes
        + nodelist->nodelist.count * sizeof(Node*);

    memcpy(dst, &node, sizeof(Node*));
    nodelist->nodelist.count += 1;
}

inline Node* NU_Nodelist_Get(NU_Nodelist* nodelist, size_t index)
{
    return nodelist->nodes[index];
}

void NU_Nodelist_Set(NU_Nodelist* nodelist, size_t index, Node* node)
{
    // Out of bounds
    if (index >= nodelist->count) return;

    void* dst = (char*)nodelist->nodes + index * sizeof(Node*);
    memcpy(dst, &node, sizeof(Node*));
}

void NU_Nodelist_Insert(NU_Nodelist_Internal* nodelist, size_t index, Node* node)
{
    // Out of bounds — can only insert at or before size
    if (index > nodelist->nodelist.count) return;

    // Grow if needed
    if (nodelist->nodelist.count == nodelist->capacity) {
        NU_Nodelist_Grow(nodelist);
    }

    // Shift elements forward from 'index' to 'size - 1'
    if (index < nodelist->nodelist.count) {
        void* dest = (char*)nodelist->nodelist.nodes
            + ((index + 1) * sizeof(Node*));
        void* src  = (char*)nodelist->nodelist.nodes
            + (index * sizeof(Node*));

        size_t bytes_to_move =
            (nodelist->nodelist.count - index) * sizeof(Node*);

        memmove(dest, src, bytes_to_move);
    }

    // Insert the new value
    void* dst = (char*)nodelist->nodelist.nodes
        + (index * sizeof(Node*));

    memcpy(dst, &node, sizeof(Node*));

    // Update size
    nodelist->nodelist.count += 1;
}

void NU_Nodelist_Swap(NU_Nodelist* nodelist, size_t index_a, size_t index_b)
{
    // Out of bounds or same index — do nothing
    if (index_a >= nodelist->count || index_b >= nodelist->count || index_a == index_b) {
        return;
    }

    Node* temp = nodelist->nodes[index_a];
    nodelist->nodes[index_a] = nodelist->nodes[index_b];
    nodelist->nodes[index_b] = temp;
}

void NU_Nodelist_Delete_Backfill(NU_Nodelist* nodelist, size_t index)
{   
    // Out of bounds
    if (index >= nodelist->count) {
        return;
    }

    // Copy last element into the deleted slot
    size_t last_index = nodelist->count - 1;
    if (index != last_index) {
        void* dst = (char*)nodelist->nodes + index * sizeof(Node*);
        void* src = (char*)nodelist->nodes + last_index * sizeof(Node*);
        memcpy(dst, src, sizeof(Node*));
    }

    // Reduce size 
    nodelist->count -= 1;
}

void NU_Nodelist_Delete_Backshift(NU_Nodelist* nodelist, size_t index)
{
    // Out of bounds
    if (index >= nodelist->count) {
        return;
    }

    // Shift items back to fill overwrite deleted
    if (index < nodelist->count - 1)
    {
        void* dest = (char*)nodelist->nodes + (index * sizeof(Node*));
        void* src  = (char*)nodelist->nodes + ((index + 1) * sizeof(Node*));

        size_t num_bytes_to_move =
            (nodelist->count - index - 1) * sizeof(Node*);

        memmove(dest, src, num_bytes_to_move);
    }

    // Reduce size 
    nodelist->count--;
}