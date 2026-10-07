#pragma once
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nu_node.h"
#include "nu_node_alloc.h"
#include "nu_nodelist.h"
#include "nu_tree_traversal.h"

/* Initialises node */
void ApplyNodeDefaults(Nalloc* nalloc, Node* node, Node* parent, u8 type, u16 layer)
{
    // Zero init
    memset(node, 0, sizeof(Node));

    // Init members passed as args to this function
    node->index = NodeIndex(nalloc, node);
    node->parentIndex = NodeIndex(nalloc, parent);
    node->layer = layer;
    node->type = type;

    // Set specific fields to default values
    node->public.generic.maxWidth = node->public.generic.maxHeight = UINT16_MAX;
    node->public.generic.backgroundR = node->public.generic.backgroundG = node->public.generic.backgroundB = 50;
    node->public.generic.borderR = node->public.generic.borderG = node->public.generic.borderB = 100;
    node->public.generic.textR = node->public.generic.textG = node->public.generic.textB = 255;
    node->horizontalTextAlignment = 0;
    node->verticalTextAlignment = 1;
    node->widgetID = -1;

    // Set defaults based on node type
    switch (node->type)
    {
    case NU_TYPE_TABLE:
        node->overrideStyleFlags |= PROPERTY_FLAG_LAYOUT_DIR;
        node->layoutFlags |= LAYOUT_VERTICAL;
        break;
    case NU_TYPE_THEAD:
        node->overrideStyleFlags |= PROPERTY_FLAG_GROW;
        node->layoutFlags |= GROW_HORIZONTAL;
        break;
    case NU_TYPE_ROW:
        node->overrideStyleFlags |= PROPERTY_FLAG_GROW;
        node->layoutFlags |= GROW_HORIZONTAL;
        break;
    case NU_TYPE_INPUT:
        node->special.input.textInputHandle = -1;
        break;
    case NU_TYPE_CANVAS:
        node->special.canvas.ctxHandle = -1;
        break;
    case NU_TYPE_FRAME:
        node->overrideStyleFlags |= PROPERTY_FLAG_HIDE_BACKGROUND;
        node->layoutFlags |= HIDE_BACKGROUND;
        break;
    default:
        break;
    }

    // Inherit parent's window (if node is not a window itself)
    if (node->type != NU_TYPE_WINDOW) {
        node->windowID = NODE_PARENT(node)->windowID;
    }
}

typedef struct Tree
{
    Nalloc nodeAlloc;
    Node* root;
    u32 depth;
    u32 nodeCount;
    Array deleteStack; // preallocated (reduce fragmentation)
    Array deletedButNotFreedNodes;
} Tree;

typedef void (*TreeDeleteCallback)(Node* node);

Node* TreeCreate(Tree* tree)
{
    // create layer allocators
    Nalloc_Init(&tree->nodeAlloc);

    Array_Init(&tree->deletedButNotFreedNodes, sizeof(Node*), 25);
    Array_Init(&tree->deleteStack, sizeof(Node*), 100);

    // member variables
    tree->depth = 1;
    tree->nodeCount = 1;

    // create root node
    Node* root = Nalloc_Alloc(&tree->nodeAlloc);
    ApplyNodeDefaults(&tree->nodeAlloc, root, NULL, NU_TYPE_WINDOW, 0);

    tree->root = root;
    return root;
}

void TreeFree(Tree* tree)
{
    Nalloc_Destroy(&tree->nodeAlloc);
    Array_Free(&tree->deletedButNotFreedNodes);
    Array_Free(&tree->deleteStack);
    tree->depth = 0;
    tree->nodeCount = 0;
}

Node* TreeCreateNode(Tree* tree, Node* parent, u8 type)
{
    // grow tree depth if necessary
    if (parent->layer == tree->depth - 1) tree->depth++;
    tree->nodeCount++;

    // allocate new node
    Node* newNode = Nalloc_Alloc(&tree->nodeAlloc);
    ApplyNodeDefaults(&tree->nodeAlloc, newNode, parent, type, parent->layer + 1);

    // parent has no children
    if (parent->firstChildIndex == INVALID_NODE_INDEX) {
        parent->firstChildIndex = newNode->index;
        parent->lastChildIndex = newNode->index;
    }
    else {
        // add new node to the sibling chain
        newNode->prevSiblingIndex = parent->lastChildIndex;
        NODE_PREV_SIBLING(newNode)->nextSiblingIndex = newNode->index;
        parent->lastChildIndex = newNode->index;
    }
    parent->childCount++;

    return newNode;
}

void TreeShiftNodeInParent(Tree* tree, Node* node, int index)
{
    if (!node || node->parentIndex == INVALID_NODE_INDEX) return;
    Node* parent = NODE_PARENT(node);

    if (parent->childCount <= 1) return;

    // Clamp index
    if (index < 0) index = 0;
    if (index >= parent->childCount) index = parent->childCount - 1;

    // Find current index
    int currentIndex = 0;
    Node* it = NODE_FIRST_CHILD(parent);
    while (it && it != node) {
        it = NODE_NEXT_SIBLING(it);
        currentIndex++;
    }

    if (!it) return;                 // not found (shouldn't happen)
    if (currentIndex == index) return;  // already in position

    // Detach from sibling chain
    if (node->prevSiblingIndex != INVALID_NODE_INDEX) NODE_PREV_SIBLING(node)->nextSiblingIndex = node->nextSiblingIndex;
    else parent->firstChildIndex = node->nextSiblingIndex;

    if (node->nextSiblingIndex != INVALID_NODE_INDEX) NODE_NEXT_SIBLING(node)->prevSiblingIndex = node->prevSiblingIndex;
    else parent->lastChildIndex = node->prevSiblingIndex;

    node->prevSiblingIndex = INVALID_NODE_INDEX;
    node->nextSiblingIndex = INVALID_NODE_INDEX;

    // Reinsert at new index
    if (index == 0) {
        node->nextSiblingIndex = parent->firstChildIndex;
        NODE_FIRST_CHILD(parent)->prevSiblingIndex = node->index;
        parent->firstChildIndex = node->index;
    }
    else {
        Node* at = NODE_FIRST_CHILD(parent);
        for (int i=0; i<index; i++) {
            at = NODE_NEXT_SIBLING(at);
        }
        node->prevSiblingIndex = at->prevSiblingIndex;
        node->nextSiblingIndex = at->index;
        NODE_PREV_SIBLING(at)->nextSiblingIndex = node->index;
        at->prevSiblingIndex = node->index;
    }

    if (index == parent->childCount - 1) parent->lastChildIndex = node->index;
}

void TreeReparentNode(Tree* tree, Node* node, Node* newParent)
{
    if (!node || !newParent) return;
    if (node->parentIndex == newParent->index) return;
    if (node == newParent) return;

    // Guard: ensure newParent is not a descendant of node
    Node* ancestor = NODE_PARENT(newParent);
    while (ancestor != NULL) {
        if (ancestor == node) return;
        ancestor = NODE_PARENT(ancestor);
    }

    Node* oldParent = NODE_PARENT(node);

    // Detach from old parent's sibling chain
    if (node->prevSiblingIndex != INVALID_NODE_INDEX) NODE_PREV_SIBLING(node)->nextSiblingIndex = node->nextSiblingIndex;
    else oldParent->firstChildIndex = node->nextSiblingIndex;

    if (node->nextSiblingIndex != INVALID_NODE_INDEX) NODE_NEXT_SIBLING(node)->prevSiblingIndex = node->prevSiblingIndex;
    else oldParent->lastChildIndex = node->prevSiblingIndex;

    oldParent->childCount--;

    // Attach to new parent as last child
    node->parentIndex = newParent->index;
    node->prevSiblingIndex = newParent->lastChildIndex;
    node->nextSiblingIndex = INVALID_NODE_INDEX;
    node->clippedAncestorIndex = INVALID_NODE_INDEX;
    node->layer = newParent->layer + 1;
    if (node->type != NU_TYPE_WINDOW) {
        node->windowID = newParent->windowID;
    }

    if (newParent->firstChildIndex != INVALID_NODE_INDEX) {
        newParent->firstChildIndex = node->index;
        newParent->lastChildIndex = node->index;
    }
    else {
        NODE_LAST_CHILD(newParent)->nextSiblingIndex = node->index;
        newParent->lastChildIndex = node->index;
    }
    newParent->childCount++;
}

void TreeDeleteLeaf(Tree* tree, Node* leaf, TreeDeleteCallback deleteCB)
{
    tree->nodeCount--;

    // case 1: leaf node has no siblings
    if (NODE_PARENT(leaf)->childCount == 1) {
        NODE_PARENT(leaf)->firstChildIndex = INVALID_NODE_INDEX;
        NODE_PARENT(leaf)->lastChildIndex = INVALID_NODE_INDEX;
        NODE_PARENT(leaf)->childCount = 0;
    }
    // case 2: leaf is last child in parent
    else if (leaf->nextSiblingIndex == INVALID_NODE_INDEX) {
        NODE_PARENT(leaf)->lastChildIndex = leaf->prevSiblingIndex;
        NODE_PREV_SIBLING(leaf)->nextSiblingIndex = INVALID_NODE_INDEX;
        NODE_PARENT(leaf)->childCount--;
    }
    // case 3: leaf has next siblings in the chain
    else {
        // if leaf is first child
        if (leaf->index == NODE_PARENT(leaf)->firstChildIndex) {
            NODE_PARENT(leaf)->firstChildIndex = leaf->nextSiblingIndex;
            NODE_NEXT_SIBLING(leaf)->prevSiblingIndex = INVALID_NODE_INDEX;
        }
        // leaf is in the middle
        else {
            NODE_PREV_SIBLING(leaf)->nextSiblingIndex = leaf->nextSiblingIndex;
            NODE_NEXT_SIBLING(leaf)->prevSiblingIndex = leaf->prevSiblingIndex;
        }
        NODE_PARENT(leaf)->childCount--;
    }

    if (deleteCB != NULL) deleteCB(leaf);

    // Add to list of deleted but not freed nodes
    Array_Push(&tree->deletedButNotFreedNodes, &leaf);
    leaf->stateFlags |= STATE_FLAG_DELETED;
}

void TreeDeleteNode(Tree* tree, Node* node, TreeDeleteCallback deleteCB)
{
    if (node->firstChildIndex == INVALID_NODE_INDEX) {
        TreeDeleteLeaf(tree, node, deleteCB);
        return;
    }

    // push children only (NOT node itself)
    Node* sib = NODE_FIRST_CHILD(node);
    while (sib != NULL) {
        Array_Push(&tree->deleteStack, &sib);
        sib = NODE_NEXT_SIBLING(sib);
    }

    // detach subtree immediately
    node->firstChildIndex = INVALID_NODE_INDEX;
    node->lastChildIndex = INVALID_NODE_INDEX;
    node->childCount = 0;

    while (tree->deleteStack.size > 0) {

        Node* curr = *(Node**)Array_Get(&tree->deleteStack, tree->deleteStack.size-1);
        tree->deleteStack.size--;

        // push children
        Node* c = NODE_FIRST_CHILD(curr);
        while (c != NULL) {
            Array_Push(&tree->deleteStack, &c);
            c = NODE_NEXT_SIBLING(c);
        }

        if (deleteCB != NULL) deleteCB(curr);

        // Add to list of deleted but not freed nodes
        Array_Push(&tree->deletedButNotFreedNodes, &curr); curr->stateFlags |= STATE_FLAG_DELETED;
        tree->nodeCount--;
    }

    // finally unlink + delete node itself
    TreeDeleteLeaf(tree, node, deleteCB);
}

void TreeFreeDeleted(Tree* tree)
{
    for (int i=0; i<tree->deletedButNotFreedNodes.size; i++) {
        Node* deletedNode = *(Node**)Array_Get(&tree->deletedButNotFreedNodes, i);
        Nalloc_Free(&tree->nodeAlloc, deletedNode);
    }
    Array_Clear(&tree->deletedButNotFreedNodes);
}
