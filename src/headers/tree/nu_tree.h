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
#include "nu_layer.h"
#include "nu_node_table.h"

// This is an implementation of a tree data structure based on layers of slabs
// Invariant: All nodes in a layer are ordered left-to-right
// Invariant: All siblings belonging to a parent are ordered left-to-right and are packed without spaces

// Essential tree functions:
// 1. u32 TreeCreate(Tree* tree, u32 layerInitCapacity, NodeType rootType) // returns a handle of the root node
// 2. TreeFree(Tree* tree)
// 3. u32 TreeAppendNode(Tree* tree, u32 layer) // returns a handle of the appended node
// 4. u32 TreeCreateNode(Tree* tree, u32 parentHandle, NodeType type) // returns a handle of the created node
// 5. TreeDeleteNode(Tree* tree, u32 handle)
// 6. TreeGetNode(Tree* tree, u32 layer, u32 index)
// 7. TreeReparentNode(Tree* tree, u32 handle, u32 newParentHandle)

typedef struct Tree
{
    NodeTable table;
    Layer* layers;
    u32 layerCapacity;
    u32 reserveNodesPerLayer;
    u32 depth;
    u32 nodeCount;
} Tree;

typedef void (*TreeDeleteCallback)(NodeP* node);

NodeP* TreeCreate(Tree* tree, u32 reserveNodesPerLayer, NodeType rootType)
{
    // create node table
    tree->layerCapacity = 4;
    tree->reserveNodesPerLayer = reserveNodesPerLayer;
    NodeTableCreate(&tree->table, reserveNodesPerLayer * tree->layerCapacity);

    // create layers
    tree->layers = malloc(sizeof(Layer) * tree->layerCapacity);
    LayerCreate(&tree->layers[0], 1);
    for (u32 i=1; i<tree->layerCapacity; i++) {
        LayerCreate(&tree->layers[i], reserveNodesPerLayer);
    }

    // member variables
    tree->depth = 1;
    tree->nodeCount = 1;

    // set root layer size = 1
    tree->layers[0].size = 1;

    // create root node
    NodeP* root = &tree->layers[0].nodeArray[0];
    root->type = rootType;
    root->index = 0;
    root->parentHandle = UINT32_MAX;
    root->firstChildIndex = UINT32_MAX;
    root->childCapacity = reserveNodesPerLayer;
    root->childCount = 0;
    root->layer = 0;
    root->state = 1;
    NodeTableAdd(&tree->table, root);
    return root;
}

void TreeFree(Tree* tree)
{
    NodeTableFree(&tree->table);
    for (u32 i=0; i<tree->layerCapacity; i++) {
        LayerFree(&tree->layers[i]);
    }
    free(tree->layers);
    tree->layers = NULL;
    tree->layerCapacity = 0;
    tree->depth = 0;
    tree->reserveNodesPerLayer = 0;
    tree->nodeCount = 0;
}

void TreeGrowLayer(Tree* tree, Layer* layer)
{
    layer->capacity *= 2;
    layer->nodeArray = realloc(layer->nodeArray, sizeof(NodeP) * layer->capacity);

    // update node table
    for (u32 i=0; i<layer->size; i++) {
        NodeP* node = &layer->nodeArray[i];
        if (node->state) NodeTableUpdate(&tree->table, node->handle, node);
    }
}

void TreeGrowDepth(Tree* tree)
{
    u32 prevLayerCap = tree->layerCapacity;
    tree->layerCapacity *= 2;
    tree->layers = realloc(tree->layers, sizeof(Layer) * tree->layerCapacity);
    for (u32 i=0; i<prevLayerCap; i++) {
        LayerCreate(&tree->layers[i + prevLayerCap], tree->reserveNodesPerLayer);
    }
}

NodeP* TreeAppendNode(Tree* tree, u32 layer, NodeType type)
{
    // grow tree depth capacity if necessary
    if (layer == tree->layerCapacity) {
        TreeGrowDepth(tree);
    }

    // grow tree depth if necessary
    if (layer == tree->depth) tree->depth++;

    // get layer
    Layer* appendLayer = &tree->layers[layer];

    // grow layer if necessary
    if (appendLayer->size == appendLayer->capacity) {
        TreeGrowLayer(tree, appendLayer);
    }

    // update layer state
    appendLayer->count++;
    appendLayer->size++;

    // init node
    NodeP* node = &appendLayer->nodeArray[appendLayer->size-1];
    node->type = type;
    node->index = appendLayer->size-1;
    node->firstChildIndex = UINT32_MAX;
    node->childCapacity = 0;
    node->childCount = 0;
    node->layer = layer;
    NodeTableAdd(&tree->table, node);
    return node;
}

u32 TreeCreateNode(Tree* tree, u32 parentHandle, NodeType type)
{
    NodeP* parent = NodeTableGet(&tree->table, parentHandle);
    if (parent == NULL) return UINT32_MAX;

    // grow tree depth capacity if necessary
    if (parent->layer == tree->layerCapacity - 1) {
        TreeGrowDepth(tree);
    }

    // grow tree depth if necessary
    if (parent->layer == tree->depth - 1) tree->depth++;

    // get layers
    Layer* parentLayer = &tree->layers[parent->layer];
    Layer* nodeLayer = &tree->layers[parent->layer + 1];

    tree->nodeCount++;
    nodeLayer->count++;
    parent->childCount++;
    u32 createNodeIndex;
    if (parent->childCount > parent->childCapacity) // parent requires extra slab space
    {
        // expand layer if necessary
        if (nodeLayer->size + 5 >= nodeLayer->capacity) {
            TreeGrowLayer(tree, nodeLayer); 
            if (parent->layer == 0) { // root node has the whole layer as its child capacity
                parent->childCapacity = nodeLayer->capacity;
            }
        }

        // no child capacity -> preemtively set parent's firstChildIndex
        if (parent->childCapacity == 0) {
            parent->firstChildIndex = 0;

            // find nearest preceeding parent with children
            if (parent->index > 0) {
                for (int i=parent->index-1; i>=0; i--) {
                    NodeP* prevParent = &parentLayer->nodeArray[i];
                    if (prevParent->state && prevParent->childCapacity > 0) {
                        parent->firstChildIndex = prevParent->firstChildIndex + prevParent->childCapacity;
                        break;
                    }
                }
            }
        }
        createNodeIndex = parent->firstChildIndex + parent->childCount - 1;

        // update first child indices for proceeding parent nodes
        for (u32 i=parent->index+1; i<parentLayer->size; i++) {
            NodeP* nextParent = &parentLayer->nodeArray[i];
            if (nextParent->state && nextParent->childCapacity > 0) {
                nextParent->firstChildIndex += 5;
            }
        }

        // shift nodes up to make room then
        // update node's self indices and nodes's chilren's 
        // parent indices for proceeding nodes
        u32 shiftCount = nodeLayer->size - createNodeIndex;
        memmove(
            &nodeLayer->nodeArray[createNodeIndex+5], 
            &nodeLayer->nodeArray[createNodeIndex], 
            shiftCount * sizeof(NodeP)
        );
        nodeLayer->size += 5;

        // update shifted node indices and table mappings
        for (u32 i=createNodeIndex+5; i<nodeLayer->size; i++) {
            NodeP* node = &nodeLayer->nodeArray[i];
            if (node->state == 0) continue;
            node->index = i;
            NodeTableUpdate(&tree->table, node->handle, node);
        }

        // give the parent more slab capacity
        parent->childCapacity += 5;
        for (u32 i=createNodeIndex+1; i<createNodeIndex+5; i++) {
            nodeLayer->nodeArray[i].state = 0;
        }
    }
    else {
        createNodeIndex = parent->firstChildIndex + parent->childCount;
    }

    // create node at location
    NodeP* createdNode = &nodeLayer->nodeArray[createNodeIndex];
    createdNode->type = type;
    createdNode->index = createNodeIndex;
    createdNode->parentHandle = parent->handle;
    createdNode->firstChildIndex = 0;
    createdNode->childCapacity = 0;
    createdNode->childCount = 0;
    createdNode->layer = parent->layer + 1;
    NodeTableAdd(&tree->table, createdNode);
    return createdNode->handle;
}

void TreeBackshiftChildren(Tree* tree, NodeP* parent, u32 dist)
{
    if (!parent) return;
    if (parent->childCount == 0 || parent->childCapacity == 0) return;
    if (dist == 0) return;

    // cannot backshift past 0 → reclaim slab instead
    if (parent->firstChildIndex < dist) {
        parent->firstChildIndex = UINT32_MAX;
        parent->childCapacity   = 0;
        parent->childCount      = 0;
        return;
    }

    Layer* layer = &tree->layers[parent->layer + 1];
    u32 newStart = parent->firstChildIndex - dist;

    memmove(
        &layer->nodeArray[newStart],
        &layer->nodeArray[parent->firstChildIndex],
        parent->childCount * sizeof(NodeP)
    );

    parent->firstChildIndex = newStart;

    // clear freed slots
    for (u32 i=0; i<dist; i++) {
        layer->nodeArray[newStart + parent->childCount + i].state = 0;
    }

    // fix indices
    for (u32 i=0; i<parent->childCount; i++) {
        NodeP* c = &layer->nodeArray[newStart + i];
        c->index = newStart + i;
        NodeTableUpdate(&tree->table, c->handle, c);
    }
}

void TreeBackshiftNextSiblings(Tree* tree, NodeP* node, NodeP* parent)
{
    if (!parent || parent->childCount == 0) return;

    Layer* layer = &tree->layers[parent->layer + 1];

    u32 last = parent->firstChildIndex + parent->childCount - 1;

    // deleting last child → no shift
    if (node->index == last) {
        node->state = 0;
        parent->childCount--;
    }
    else {
        u32 moveCount = last - node->index;

        memmove(
            &layer->nodeArray[node->index],
            &layer->nodeArray[node->index + 1],
            moveCount * sizeof(NodeP)
        );

        parent->childCount--;

        // clear freed slot
        layer->nodeArray[parent->firstChildIndex + parent->childCount].state = 0;

        // fix indices
        for (u32 i=0; i<moveCount; i++) {
            NodeP* s = &layer->nodeArray[node->index + i];
            s->index--;
            NodeTableUpdate(&tree->table, s->handle, s);
        }
    }

    // slab empty → reclaim
    if (parent->childCount == 0) {
        parent->firstChildIndex = UINT32_MAX;
        parent->childCapacity   = 0;
    }
}

void TreeDeleteChildlessNode(Tree* tree, NodeP* node, TreeDeleteCallback deleteCB)
{
    if (!node) return;

    deleteCB(node); // dissociate node
    NodeP* parent = NodeTableGet(&tree->table, node->parentHandle);
    Layer* layer  = &tree->layers[node->layer];

    NodeTableDelete(&tree->table, node->handle);
    node->state = 0;

    layer->count--;
    tree->nodeCount--;

    // only node in layer
    if (layer->count == 0) {

        // decrease tree depth
        tree->depth--;

        if (parent) {
            parent->childCount = 0;
            parent->childCapacity = 0;
            parent->firstChildIndex = UINT32_MAX;
        }
        layer->size = 0;
        return;
    }

    if (parent && parent->childCount > 0) {
        TreeBackshiftNextSiblings(tree, node, parent);
    }
}

typedef struct {
    u32 layer;
    u32 start;
    u32 capacity;
    u32 count;
    u32 parentHandle;
} Slab;

static void CollectSlabs(Tree* tree, NodeP* root, Slab* slabs, u32* slabCount)
{
    u32 stackCap = 256;
    u32 sp = 0;
    NodeP** stack = malloc(stackCap * sizeof(NodeP*));
    stack[sp++] = root;
    while (sp)
    {
        NodeP* node = stack[--sp];
        if (node->childCount > 0) {
            Layer* childLayer = &tree->layers[node->layer + 1];

            for (u32 i=0; i<node->childCount; i++) {
                if (sp == stackCap) {
                    stackCap *= 2;
                    stack = realloc(stack, stackCap * sizeof(NodeP*));
                }
                stack[sp++] = &childLayer->nodeArray[node->firstChildIndex + i];
            }
            Slab* s = &slabs[(*slabCount)++];
            s->layer        = node->layer + 1;
            s->start        = node->firstChildIndex;
            s->capacity     = node->childCapacity;
            s->count        = node->childCount;
            s->parentHandle = node->handle;
        }
    }
    free(stack);
}

static void DeleteSlab(Tree* tree, Slab* slab, TreeDeleteCallback deleteCB)
{
    Layer* layer = &tree->layers[slab->layer];

    // remove nodes from table
    for (u32 i=0; i<slab->count; i++) {
        NodeP* node = &layer->nodeArray[slab->start + i];
        deleteCB(node); // dissociate node
        NodeTableDelete(&tree->table, node->handle);
        node->state = 0;
        tree->nodeCount--;
    }

    // case 1: slab at end
    if (slab->start + slab->capacity == layer->size) {
        layer->size  -= slab->capacity;
        layer->count -= slab->count;
    }
    // case 2: slab has right neighbor → shift it left
    else {
        u32 nextStart = slab->start + slab->capacity;
        NodeP* nextNode = &layer->nodeArray[nextStart];
        NodeP* nextParent = NodeTableGet(&tree->table, nextNode->parentHandle);
        TreeBackshiftChildren(tree, nextParent, slab->capacity);
        layer->count -= slab->count;
    }

    // decrease tree depth if necessary
    if (slab->layer == tree->depth-1 && layer->count == 0) {
        tree->depth--;
    }
}

void TreeDeleteBranch(Tree* tree, NodeP* node, TreeDeleteCallback deleteCB)
{
    Slab slabs[128];
    u32 slabCount = 0;

    NodeP* parent = NodeTableGet(&tree->table, node->parentHandle);

    // collect child slabs
    CollectSlabs(tree, node, slabs, &slabCount);

    // sort slabs by descending layer (simple bubble, slabCount is small)
    for (u32 i=0; i<slabCount; i++) {
        for (u32 j=i+1; j<slabCount; j++) {
            if (slabs[i].layer < slabs[j].layer) {
                Slab tmp = slabs[i];
                slabs[i] = slabs[j];
                slabs[j] = tmp;
            }
        }
    }

    // delete slabs bottom-up
    for (u32 i=0; i<slabCount; i++) {
        DeleteSlab(tree, &slabs[i], deleteCB);
    }

    // finally delete the branch root itself
    node->childCount = 0;
    node->childCapacity = 0;
    node->firstChildIndex = UINT32_MAX;
    TreeDeleteChildlessNode(tree, node, deleteCB);
}

void TreeDeleteNode(Tree* tree, u32 nodeHandle, TreeDeleteCallback deleteCB)
{
    NodeP* node = NodeTableGet(&tree->table, nodeHandle);
    if (!node || node->layer == 0) return;

    if (node->childCount > 0) 
        TreeDeleteBranch(tree, node, deleteCB);
    else
        TreeDeleteChildlessNode(tree, node, deleteCB);
}