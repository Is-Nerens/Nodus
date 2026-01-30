#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#include "nu_node.h"
#include "nu_node_alloc.h"
#include "nu_nodelist.h"
#include "nu_tree_traversal.h"
#include <datastructures/vector.h>

typedef struct Tree
{
    Nalloc* layerAllocs;
    NodeP* root;
    u32 layerAllocsCapacity;
    u32 layerAllocsSize;
    u32 depth;
    u32 nodeCount;
} Tree;

typedef void (*TreeDeleteCallback)(NodeP* node);

NodeP* TreeCreate(Tree* tree, NodeType rootType)
{
    // create layer allocators
    tree->layerAllocsCapacity = 4;
    tree->layerAllocsSize = 4;
    tree->layerAllocs = malloc(sizeof(Nalloc) * tree->layerAllocsCapacity);
    for (int i=0; i<tree->layerAllocsSize; i++)
    {
        if (i == 0) Nalloc_Init(&tree->layerAllocs[i], 1);
        else Nalloc_Init(&tree->layerAllocs[i], 100);
    }

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
    root->childCount = 0;
    root->layer = 0;
    root->state = 1;
    tree->root = root;
    return root;
}

void TreeFree(Tree* tree)
{
    for (int i=0; i<tree->layerAllocsSize; i++) {
        Nalloc_Destroy(&tree->layerAllocs[i]);
    }
    free(tree->layerAllocs);
    tree->depth = 0;
    tree->nodeCount = 0;
}

void TreeAddLayer(Tree* tree)
{
    u32 newCapacity = tree->layerAllocsCapacity * 2;
    tree->layerAllocsCapacity = newCapacity;
    tree->layerAllocs = realloc(tree->layerAllocs, sizeof(Nalloc) * tree->layerAllocsCapacity);
    for (u32 i=tree->layerAllocsSize; i<newCapacity; i++) {
        Nalloc_Init(&tree->layerAllocs[i], 100);
    }
}

NodeP* TreeCreateNode(Tree* tree, NodeP* parent, NodeType type)
{
    // add additional layer allocator if necessary
    if (parent->layer + 1 == tree->layerAllocsSize) {
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
    newNode->childCount = 0;
    newNode->layer = parent->layer + 1;
    newNode->state = 1;
    NU_ApplyNodeDefaults(newNode); // applies defaults to node->node (public node struct)

    // parent has no children
    if (parent->firstChild == NULL) {
        parent->firstChild = newNode;
        parent->lastChild = newNode;
        parent->childCount++;
    }
    else {
        // add new node to the sibling chain
        newNode->prevSibling = parent->lastChild;
        newNode->prevSibling->nextSibling = newNode;
        parent->lastChild = newNode;
    }

    return newNode;
}

void TreeDeleteLeaf(Tree* tree, NodeP* leaf, TreeDeleteCallback deleteCB)
{
    tree->nodeCount--;
    if (leaf->layer == tree->depth - 1) {
        tree->depth--;
    }

    Nalloc* nalloc = &tree->layerAllocs[leaf->layer];

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
        }
        // update nextSibling of leaf's previou sibling
        else {
            leaf->prevSibling->nextSibling = leaf->nextSibling;
        }
        leaf->parent->childCount--;
    }
    
    if (deleteCB != NULL) deleteCB(leaf);
    Nalloc_Free(nalloc, leaf);
}

void TreeDeleteNode(Tree* tree, NodeP* node, TreeDeleteCallback deleteCB)
{
    if (node->firstChild == NULL) {
        TreeDeleteLeaf(tree, node, deleteCB);
    }
    else {
        
        // arrange all subnodes from left to right -> bottom to top
        Vector stack;
        Vector_Reserve(&stack, sizeof(NodeP*), 100);

        Vector_Push(&stack, &node);
        while(stack.size > 0) {

            NodeP* parent = *(NodeP**)Vector_Get(&stack, stack.size-1);

            // if parent is a leaf node
            if (parent->firstChild == NULL) {
                if (deleteCB != NULL) deleteCB(parent);

                // delete leaf node
                Nalloc* nalloc = &tree->layerAllocs[parent->layer];
                Nalloc_Free(nalloc, parent);
                tree->nodeCount--;
            }
            // add children to the stack
            else {
                NodeP* sib = parent->firstChild;
                while (sib != NULL) {
                    Vector_Push(&stack, &sib);
                    sib = sib->nextSibling;
                }
                parent->firstChild = NULL;
            }

            // pop the stack
            stack.size--;
        }

        // finally delete the node
        TreeDeleteLeaf(tree, node, deleteCB);

        // free
        Vector_Free(&stack);
    }
}