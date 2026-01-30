#pragma once


// ---------------------------
// Depth First Search iterator
// ---------------------------
typedef struct DFSFrame {
    NodeP* node;
    NodeP* nextChild;
    uint8_t visited;   // 0 = not returned yet, 1 = returned
} DFSFrame;


typedef struct DepthFirstSearch {
    DFSFrame* stack;
    uint32_t size;
    uint32_t capacity;
} DepthFirstSearch;

DepthFirstSearch DepthFirstSearch_Create(NodeP* root)
{
    DepthFirstSearch dfs;
    dfs.capacity = 64;
    dfs.size = 0;
    dfs.stack = malloc(sizeof(DFSFrame) * dfs.capacity);
    if (root) dfs.stack[dfs.size++] = (DFSFrame){ .node = root, .nextChild = root->firstChild };
    return dfs;
}

int DepthFirstSearch_Next(DepthFirstSearch* dfs, NodeP** nodeOut)
{
    while (dfs->size > 0) {
        DFSFrame* top = &dfs->stack[dfs->size - 1];

        // Return node exactly once
        if (!top->visited) {
            top->visited = 1;
            *nodeOut = top->node;
            return 1;
        }

        // Descend into children
        if (top->nextChild != NULL) {
            NodeP* child = top->nextChild;
            top->nextChild = child->nextSibling;

            if (dfs->size == dfs->capacity) {
                dfs->capacity *= 2;
                dfs->stack = realloc(dfs->stack, sizeof(DFSFrame) * dfs->capacity);
            }

            dfs->stack[dfs->size++] = (DFSFrame){
                .node = child,
                .nextChild = child->firstChild,
                .visited = 0
            };
            continue;
        }

        // Done with this node
        dfs->size--;
    }

    return 0;
}

void DepthFirstSearch_Free(DepthFirstSearch* dfs)
{
    free(dfs->stack);
    dfs->stack = NULL;
    dfs->size = 0;
    dfs->capacity = 0;
}





// -----------------------------
// Breadth First Search iterator
// -----------------------------
typedef struct BFSQueue {
    NodeP** data;
    uint32_t size;
    uint32_t capacity;
    uint32_t front;
} BFSQueue;

typedef struct BreadthFirstSearch {
    BFSQueue queue;
} BreadthFirstSearch;

static void BFSQueue_Init(BFSQueue* q, uint32_t cap) {
    q->data = malloc(sizeof(NodeP*) * cap);
    q->size = 0;
    q->front = 0;
    q->capacity = cap;
}

static void BFSQueue_Free(BFSQueue* q) {
    free(q->data);
    q->data = NULL;
    q->size = q->front = q->capacity = 0;
}

static void BFSQueue_Push(BFSQueue* q, NodeP* node) {
    if (q->size >= q->capacity) {
        q->capacity *= 2;
        q->data = realloc(q->data, sizeof(NodeP*) * q->capacity);
    }
    q->data[q->size++] = node;
}

static NodeP* BFSQueue_Pop(BFSQueue* q) {
    if (q->front >= q->size) return NULL;
    return q->data[q->front++];
}

BreadthFirstSearch BreadthFirstSearch_Create(NodeP* root) {
    BreadthFirstSearch bfs;
    BFSQueue_Init(&bfs.queue, 64);
    if (root) BFSQueue_Push(&bfs.queue, root);
    return bfs;
}

int BreadthFirstSearch_Next(BreadthFirstSearch* bfs, NodeP** nodeOut) {
    NodeP* node = BFSQueue_Pop(&bfs->queue);
    if (!node) return 0;
    *nodeOut = node;

    NodeP* child = node->firstChild;
    while (child) {
        BFSQueue_Push(&bfs->queue, child);
        child = child->nextSibling;
    }

    return 1;
}

void BreadthFirstSearch_Free(BreadthFirstSearch* bfs) {
    BFSQueue_Free(&bfs->queue);
}






// -------------------------------------
// Reverse Breadth First Search iterator
// -------------------------------------
typedef struct ReverseBreadthFirstSearch {
    NodeP** nodes;
    uint32_t count;
    uint32_t index;
} ReverseBreadthFirstSearch;

ReverseBreadthFirstSearch ReverseBreadthFirstSearch_Create(NodeP* root) {
    ReverseBreadthFirstSearch rBFS = {0};
    if (!root) return rBFS;

    // BFS queue
    BFSQueue queue;
    BFSQueue_Init(&queue, 64);
    BFSQueue_Push(&queue, root);

    // temp stack for layers
    NodeP*** layerStack = malloc(sizeof(NodeP**) * 16);
    uint32_t* layerCounts = malloc(sizeof(uint32_t) * 16);
    uint32_t layerStackSize = 0;
    uint32_t layerCapacity = 16;

    while (queue.front < queue.size) {
        uint32_t layerCount = queue.size - queue.front;
        NodeP** layerNodes = malloc(sizeof(NodeP*) * layerCount);

        for (uint32_t i = 0; i < layerCount; i++) {
            NodeP* node = BFSQueue_Pop(&queue);
            layerNodes[i] = node;

            NodeP* child = node->firstChild;
            while (child) {
                BFSQueue_Push(&queue, child);
                child = child->nextSibling;
            }
        }

        if (layerStackSize == layerCapacity) {
            layerCapacity *= 2;
            layerStack = realloc(layerStack, sizeof(NodeP**) * layerCapacity);
            layerCounts = realloc(layerCounts, sizeof(uint32_t) * layerCapacity);
        }

        layerStack[layerStackSize] = layerNodes;
        layerCounts[layerStackSize] = layerCount;
        layerStackSize++;
    }

    BFSQueue_Free(&queue);

    // Count total nodes
    uint32_t total = 0;
    for (uint32_t i = 0; i < layerStackSize; i++) total += layerCounts[i];

    rBFS.nodes = malloc(sizeof(NodeP*) * total);
    rBFS.count = total;
    rBFS.index = 0;

    // Fill rBFS array bottom-up, left-to-right per layer
    uint32_t pos = 0;
    for (int l = layerStackSize - 1; l >= 0; l--) {
        for (uint32_t i = 0; i < layerCounts[l]; i++) {
            rBFS.nodes[pos++] = layerStack[l][i];
        }
        free(layerStack[l]);
    }

    free(layerStack);
    free(layerCounts);

    return rBFS;
}

int ReverseBreadthFirstSearch_Next(ReverseBreadthFirstSearch* rBFS, NodeP** nodeOut) {
    if (rBFS->index >= rBFS->count) {
        rBFS->index = 0; // auto-reset after traversal
        return 0;
    }

    *nodeOut = rBFS->nodes[rBFS->index++];
    return 1;
}

void ReverseBreadthFirstSearch_Free(ReverseBreadthFirstSearch* rBFS) {
    free(rBFS->nodes);
    rBFS->nodes = NULL;
    rBFS->count = 0;
    rBFS->index = 0;
}