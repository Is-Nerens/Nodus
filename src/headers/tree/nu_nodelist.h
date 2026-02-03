#pragma once
#define MAX(a, b) ((a) > (b) ? (a) : (b))


typedef struct NU_Nodelist
{
    uint32_t capacity;
    uint32_t size;
    Node** data;
} NU_Nodelist;

void NU_Nodelist_Init(NU_Nodelist* nodelist, uint32_t capacity)
{
    capacity = MAX(capacity, 5);
    nodelist->capacity = capacity;
    nodelist->size = 0;
    nodelist->data = malloc(capacity * sizeof(Node*));
}

void NU_Nodelist_Free(NU_Nodelist* nodelist)
{
    free(nodelist->data);
    nodelist->capacity = 0;
    nodelist->size = 0;
}

void NU_Nodelist_Grow(NU_Nodelist* nodelist)
{
    nodelist->capacity = MAX(nodelist->capacity * 2, 2);
    nodelist->data = realloc(nodelist->data, nodelist->capacity * sizeof(Node*));
}

void NU_Nodelist_Push(NU_Nodelist* nodelist, Node* node)
{
    if (nodelist->size == nodelist->capacity) {
        NU_Nodelist_Grow(nodelist);
    }
    void* dst = (char*)nodelist->data + nodelist->size * sizeof(Node*);
    memcpy(dst, &node, sizeof(Node*));
    nodelist->size += 1;
}

inline Node* NU_Nodelist_Get(NU_Nodelist* nodelist, uint32_t index)
{
    return nodelist->data[index];
}

void NU_Nodelist_Set(NU_Nodelist* nodelist, uint32_t index, Node* node)
{
    // Out of bounds
    if (index >= nodelist->size) return;
    void* dst = (char*)nodelist->data + index * sizeof(Node*);
    memcpy(dst, &node, sizeof(Node*));
}

void NU_Nodelist_Insert(NU_Nodelist* nodelist, uint32_t index, Node* node)
{
    // Out of bounds — can only insert at or before size
    if (index > nodelist->size) return;

    // Grow if needed
    if (nodelist->size == nodelist->capacity) {
        NU_Nodelist_Grow(nodelist);
    }

    // Shift elements forward from 'index' to 'size - 1'
    if (index < nodelist->size) {
        void* dest = (char*)nodelist->data + ((index + 1) * sizeof(Node*));
        void* src  = (char*)nodelist->data + (index * sizeof(Node*));
        size_t bytes_to_move = (nodelist->size - index) * sizeof(Node*);
        memmove(dest, src, bytes_to_move);
    }

    // Insert the new value
    void* dst = (char*)nodelist->data + (index * sizeof(Node*));
    memcpy(dst, &node, sizeof(Node*));

    // Update size
    nodelist->size += 1;
}

void NU_Nodelist_Swap(NU_Nodelist* nodelist, uint32_t index_a, uint32_t index_b)
{
    // Out of bounds or same index — do nothing
    if (index_a >= nodelist->size || index_b >= nodelist->size || index_a == index_b) {
        return;
    }

    Node* temp = nodelist->data[index_a];
    nodelist->data[index_a] = nodelist->data[index_b];
    nodelist->data[index_b] = temp;
}

void NU_Nodelist_Delete_Backfill(NU_Nodelist* nodelist, uint32_t index)
{   
    // Out of bounds
    if (index >= nodelist->size) {
        return;
    }

    // Copy last element into the deleted slot
    uint32_t last_index = nodelist->size - 1;
    if (index != last_index) {
        void* dst = (char*)nodelist->data + index * sizeof(Node*);
        void* src = (char*)nodelist->data + last_index * sizeof(Node*);
        memcpy(dst, src, sizeof(Node*));
    }

    // Reduce size 
    nodelist->size -= 1;
}

void NU_Nodelist_Delete_Backshift(NU_Nodelist* nodelist, uint32_t index)
{
    // Out of bounds
    if (index >= nodelist->size) {
        return;
    }

    // Shift items back to fill overwrite deleted
    if (index < nodelist->size-1)
    {
        void* dest = (char*)nodelist->data + (index * sizeof(Node*));
        void* src  = (char*)nodelist->data + ((index + 1) * sizeof(Node*));
        size_t num_bytes_to_move = (nodelist->size - index - 1) * sizeof(Node*);
        memmove(dest, src, num_bytes_to_move);
    }

    // Reduce size 
    nodelist->size--;
}