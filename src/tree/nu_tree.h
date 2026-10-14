#pragma once
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nu_node.h"
#include "nu_node_alloc.h"
#include "nu_nodelist.h"
#include "nu_tree_partition.h"
#include "nu_tree_traversal.h"

/* Initialises node */

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

    // create root node
    Node* root = Nalloc_Alloc(&tree->nodeAlloc);
    ApplyNodeDefaults(root, NULL, NU_TYPE_WINDOW, 0);

    // member variables
    tree->depth = 1;
    tree->nodeCount = 1;
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

static inline void Tree_AdjustSubtreeSizes(Node* node, int32_t delta)
{
    while (node) {
        node->subtreeSize += delta;
        node = node->parent;
    }
}

Node* TreeCreateNode(Tree* tree, Node* parent, u8 type)
{
    // grow tree depth if necessary
    if (parent->layer == tree->depth - 1) tree->depth++;
    tree->nodeCount++;

    // allocate new node
    Node* newNode = Nalloc_Alloc(&tree->nodeAlloc);
    ApplyNodeDefaults(newNode, parent, type, parent->layer + 1);
    Tree_AdjustSubtreeSizes(parent, 1);

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

void TreeShiftNodeInParent(Tree* tree, Node* node, int index)
{
    if (!node || !node->parent) return;
    Node* parent = node->parent;

    if (parent->childCount <= 1) return;

    // Clamp index
    if (index < 0) index = 0;
    if (index >= parent->childCount) index = parent->childCount - 1;

    // Find current index
    int currentIndex = 0;
    Node* it = parent->firstChild;
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
        Node* at = parent->firstChild;
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

void TreeReparentNode(Tree* tree, Node* node, Node* newParent)
{
    if (!node || !newParent) return;
    if (node->parent == newParent) return;
    if (node == newParent) return;

    // Guard: ensure newParent is not a descendant of node
    Node* ancestor = newParent->parent;
    while (ancestor != NULL) {
        if (ancestor == node) return;
        ancestor = ancestor->parent;
    }

    Node* oldParent = node->parent;

    // Detach from old parent's sibling chain
    if (node->prevSibling) node->prevSibling->nextSibling = node->nextSibling;
    else oldParent->firstChild = node->nextSibling;

    if (node->nextSibling) node->nextSibling->prevSibling = node->prevSibling;
    else oldParent->lastChild = node->prevSibling;

    oldParent->childCount--;
    Tree_AdjustSubtreeSizes(oldParent, -(int)node->subtreeSize);

    // Attach to new parent as last child
    node->parent = newParent;
    node->prevSibling = newParent->lastChild;
    node->nextSibling = NULL;
    node->clippedAncestor = NULL;
    node->layer = newParent->layer + 1;
    if (node->type != NU_TYPE_WINDOW) {
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
    Tree_AdjustSubtreeSizes(newParent, node->subtreeSize);
}

void TreeDeleteLeaf(Tree* tree, Node* leaf, TreeDeleteCallback deleteCB)
{
    tree->nodeCount--;

    // case 1: leaf node has no siblings
    if (leaf->parent->childCount == 1) {
        leaf->parent->firstChild = NULL;
        leaf->parent->lastChild = NULL;
        leaf->parent->childCount = 0;
    }
    // case 2: leaf is last child in parent
    else if (!leaf->nextSibling) {
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
    Array_Push(&tree->deletedButNotFreedNodes, &leaf);
    leaf->stateFlags |= STATE_FLAG_DELETED;
}

void TreeDeleteNode(Tree* tree, Node* node, TreeDeleteCallback deleteCB)
{
    if (node->firstChild == NULL) {
        TreeDeleteLeaf(tree, node, deleteCB);
        return;
    }

    // push children only (NOT node itself)
    Node* sib = node->firstChild;
    while (sib) {
        Array_Push(&tree->deleteStack, &sib);
        sib = sib->nextSibling;
    }

    // detach subtree immediately
    node->firstChild = NULL;
    node->lastChild = NULL;
    node->childCount = 0;
    Tree_AdjustSubtreeSizes(node->parent, -(int)node->subtreeSize);

    while (tree->deleteStack.size > 0) {

        Node* curr = *(Node**)Array_Get(&tree->deleteStack, tree->deleteStack.size-1);
        tree->deleteStack.size--;

        // push children
        Node* c = curr->firstChild;
        while (c) {
            Array_Push(&tree->deleteStack, &c);
            c = c->nextSibling;
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
