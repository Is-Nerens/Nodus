#pragma once
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nu_node.h"
#include "nu_node_alloc.h"
#include "nu_nodelist.h"
#include "nu_tree_traversal.h"

typedef struct Tree
{
    Nalloc* layerAllocs;
    NodeP* root;
    u32 layerAllocsCapacity;
    u32 depth;
    u32 nodeCount;
    Array deleteStack; // preallocated (reduce fragmentation)
    Array deletedButNotFreedNodes;
} Tree;

typedef void (*TreeDeleteCallback)(NodeP* node);

NodeP* TreeCreate(Tree* tree, NodeType rootType)
{
    // create layer allocators
    tree->layerAllocsCapacity = 4;
    tree->layerAllocs = malloc(sizeof(Nalloc) * tree->layerAllocsCapacity);
    for (int i=0; i<tree->layerAllocsCapacity; i++)
    {
        if (i == 0) Nalloc_Init(&tree->layerAllocs[i], 1);
        else Nalloc_Init(&tree->layerAllocs[i], 100);
    }

    ArrayInit(&tree->deletedButNotFreedNodes, sizeof(NodeP*), 25);
    ArrayInit(&tree->deleteStack, sizeof(NodeP*), 100);

    // member variables
    tree->depth = 1;
    tree->nodeCount = 1;

    // create root node
    NodeP* root = Nalloc_Alloc(&tree->layerAllocs[0]);
    root->type = rootType;
    root->parent = NULL;
    root->nextSibling = NULL;
    root->firstChild = NULL;
    root->lastChild = NULL;
    root->clippedAncestor = NULL;
    root->childCount = 0;
    root->layer = 0;
    root->stateFlags = 0;
    NU_ApplyNodeDefaults(root);
    tree->root = root;
    return root;
}

void TreeFree(Tree* tree)
{
    for (int i=0; i<tree->layerAllocsCapacity; i++) {
        Nalloc_Destroy(&tree->layerAllocs[i]);
    }
    free(tree->layerAllocs);
    tree->depth = 0;
    tree->nodeCount = 0;
}

void TreeAddLayer(Tree* tree)
{
    u32 newCapacity = tree->layerAllocsCapacity * 2;
    tree->layerAllocs = realloc(tree->layerAllocs, sizeof(Nalloc) * newCapacity);
    for (u32 i=tree->layerAllocsCapacity; i<newCapacity; i++) {
        Nalloc_Init(&tree->layerAllocs[i], 128);
    }
    tree->layerAllocsCapacity = newCapacity;
}

NodeP* TreeCreateNode(Tree* tree, NodeP* parent, NodeType type)
{
    // add additional layer allocator if necessary
    if (parent->layer + 1 == tree->layerAllocsCapacity) {
        TreeAddLayer(tree);
    }

    // grow tree depth if necessary
    if (parent->layer == tree->depth - 1) tree->depth++;
    tree->nodeCount++;

    // allocate new node
    Nalloc* nalloc = &tree->layerAllocs[parent->layer + 1];
    NodeP* newNode = Nalloc_Alloc(nalloc);
    newNode->type = type;
    newNode->parent = parent;
    newNode->nextSibling = NULL;
    newNode->firstChild = NULL;
    newNode->lastChild = NULL;
    newNode->clippedAncestor = NULL;
    newNode->childCount = 0;
    newNode->layer = parent->layer + 1;
    newNode->stateFlags = 0;
    NU_ApplyNodeDefaults(newNode);

    // parent has no children
    if (parent->firstChild == NULL) {
        parent->firstChild = newNode;
        parent->lastChild = newNode;
    }
    else {
        // add new node to the sibling chain
        newNode->prevSibling = parent->lastChild;
        newNode->prevSibling->nextSibling = newNode;
        parent->lastChild = newNode;
    }
    parent->childCount++;

    return newNode;
}

void TreeShiftNodeInParent(Tree* tree, NodeP* node, int index)
{
    if (!node || !node->parent) return;
    NodeP* parent = node->parent;

    if (parent->childCount <= 1) return;

    // Clamp index
    if (index < 0) index = 0;
    if (index >= parent->childCount) index = parent->childCount - 1;

    // Find current index
    int currentIndex = 0;
    NodeP* it = parent->firstChild;
    while (it && it != node) {
        it = it->nextSibling;
        currentIndex++;
    }

    if (!it) return;                 // not found (shouldn't happen)
    if (currentIndex == index) return;  // already in position

    // Detach from sibling chain 
    if (node->prevSibling) node->prevSibling->nextSibling = node->nextSibling;
    else parent->firstChild = node->nextSibling;

    if (node->nextSibling) node->nextSibling->prevSibling = node->prevSibling;
    else parent->lastChild = node->prevSibling;

    node->prevSibling = NULL;
    node->nextSibling = NULL;

    // Reinsert at new index
    if (index == 0) {
        node->nextSibling = parent->firstChild;
        parent->firstChild->prevSibling = node;
        parent->firstChild = node;
    }
    else {
        NodeP* at = parent->firstChild;
        for (int i=0; i<index; i++) { 
            at = at->nextSibling; 
        }
        node->prevSibling = at->prevSibling;
        node->nextSibling = at;
        at->prevSibling->nextSibling = node;
        at->prevSibling = node;
    }

    if (index == parent->childCount - 1) parent->lastChild = node;
}

void TreeReparentNode(Tree* tree, NodeP* node, NodeP* newParent)
{
    if (!node || !newParent) return;
    if (node->parent == newParent) return;
    if (node == newParent) return;

    // Guard: ensure newParent is not a descendant of node
    NodeP* ancestor = newParent->parent;
    while (ancestor != NULL) {
        if (ancestor == node) return;
        ancestor = ancestor->parent;
    }

    NodeP* oldParent = node->parent;

    // Detach from old parent's sibling chain
    if (node->prevSibling) node->prevSibling->nextSibling = node->nextSibling;
    else oldParent->firstChild = node->nextSibling;

    if (node->nextSibling) node->nextSibling->prevSibling = node->prevSibling;
    else oldParent->lastChild = node->prevSibling;

    oldParent->childCount--;

    // Attach to new parent as last child
    node->parent = newParent;
    node->prevSibling = newParent->lastChild;
    node->nextSibling = NULL;
    node->clippedAncestor = NULL;
    node->layer = newParent->layer + 1;
    if (node->type != NU_WINDOW) {
        node->windowID = newParent->windowID;
    }

    if (newParent->firstChild == NULL) {
        newParent->firstChild = node;
        newParent->lastChild = node;
    }
    else {
        newParent->lastChild->nextSibling = node;
        newParent->lastChild = node;
    }
    newParent->childCount++;
}

void TreeDeleteLeaf(Tree* tree, NodeP* leaf, TreeDeleteCallback deleteCB)
{
    tree->nodeCount--;

    // case 1: leaf node has no siblings
    if (leaf->parent->childCount == 1) {
        leaf->parent->firstChild = NULL; 
        leaf->parent->lastChild = NULL; 
        leaf->parent->childCount = 0;
    }
    // case 2: leaf is last child in parent
    else if (leaf->nextSibling == NULL) {
        leaf->parent->lastChild = leaf->prevSibling;
        leaf->prevSibling->nextSibling = NULL;
        leaf->parent->childCount--;
    }
    // case 3: leaf has next siblings in the chain
    else {
        // if leaf is first child
        if (leaf == leaf->parent->firstChild) {
            leaf->parent->firstChild = leaf->nextSibling;
            leaf->nextSibling->prevSibling = NULL;
        }
        // leaf is in the middle
        else {
            leaf->prevSibling->nextSibling = leaf->nextSibling;
            leaf->nextSibling->prevSibling = leaf->prevSibling;
        }
        leaf->parent->childCount--;
    }
    
    if (deleteCB != NULL) deleteCB(leaf);

    // Add to list of deleted but not freed nodes
    ArrayPush(&tree->deletedButNotFreedNodes, &leaf); 
    leaf->stateFlags |= STATE_FLAG_DELETED;
}

void TreeDeleteNode(Tree* tree, NodeP* node, TreeDeleteCallback deleteCB)
{
    if (node->firstChild == NULL) {
        TreeDeleteLeaf(tree, node, deleteCB);
        return;
    }

    // push children only (NOT node itself)
    NodeP* sib = node->firstChild;
    while (sib != NULL) {
        ArrayPush(&tree->deleteStack, &sib);
        sib = sib->nextSibling;
    }

    // detach subtree immediately
    node->firstChild = NULL;
    node->lastChild = NULL;
    node->childCount = 0;

    while (tree->deleteStack.size > 0) {

        NodeP* cur = *(NodeP**)ArrayGet(&tree->deleteStack, tree->deleteStack.size-1);
        tree->deleteStack.size--;

        // push children
        NodeP* c = cur->firstChild;
        while (c != NULL) {
            ArrayPush(&tree->deleteStack, &c);
            c = c->nextSibling;
        }

        if (deleteCB != NULL) deleteCB(cur);

        // Add to list of deleted but not freed nodes
        ArrayPush(&tree->deletedButNotFreedNodes, &cur); cur->stateFlags |= STATE_FLAG_DELETED;
        tree->nodeCount--;
    }

    // finally unlink + delete node itself
    TreeDeleteLeaf(tree, node, deleteCB);
}

void TreeFreeDeleted(Tree* tree)
{
    for (int i=0; i<tree->deletedButNotFreedNodes.size; i++) {
        NodeP* deletedNode = *(NodeP**)ArrayGet(&tree->deletedButNotFreedNodes, i);
        Nalloc* nalloc = &tree->layerAllocs[deletedNode->layer];
        Nalloc_Free(nalloc, deletedNode);
    }
    ArrayClear(&tree->deletedButNotFreedNodes);
}