typedef struct DFSFrame {
    NodeP* node;
    NodeP* nextChild;
} DFSFrame;

typedef struct DFS
{
    DFSFrame* stack;
    uint32_t size;
    uint32_t capacity;
    uint8_t finished;
} DFS;

DFS DFS_Create(NodeP* root)
{
    DFS dfs;
    dfs.capacity = 128;
    dfs.size = 1;
    dfs.finished = 0;
    dfs.stack = malloc(sizeof(DFSFrame) * dfs.capacity);
    dfs.stack[0].node = root;
    dfs.stack[0].nextChild = root->firstChild;
    return dfs;
}

inline NodeP* DFS_Next(DFS* dfs)
{
    if (dfs->finished) return NULL;
    while (dfs->size > 0) {
        DFSFrame* top = &dfs->stack[dfs->size - 1];
        NodeP* node = top->node;

        // first visit
        if (top->nextChild == node->firstChild) return node;

        // descend
        if (top->nextChild != NULL) {
            NodeP* child = top->nextChild;
            top->nextChild = child->nextSibling;

            if (dfs->size == dfs->capacity) {
                dfs->capacity *= 2;
                dfs->stack = realloc(
                    dfs->stack,
                    sizeof(DFSFrame) * dfs->capacity
                );
            }

            dfs->stack[dfs->size++] = (DFSFrame){
                .node = child,
                .nextChild = child->firstChild
            };
            continue;
        }
        dfs->size--;
    }
    free(dfs->stack);
    dfs->stack = NULL;
    dfs->finished = 1;
    return NULL;
}






typedef struct BFS {
    NodeP** nodes;     // array of nodes in BFS order
    uint32_t count;    // total number of nodes in the array
    uint32_t index;    // current position for iteration
} BFS;

// --------------------
// Internal dynamic vector for queue
typedef struct BFSQueue {
    NodeP** data;
    uint32_t size;
    uint32_t capacity;
} BFSQueue;

static void BFSQueue_Init(BFSQueue* q, uint32_t cap) {
    q->data = malloc(sizeof(NodeP*) * cap);
    q->size = 0;
    q->capacity = cap;
}

static void BFSQueue_Free(BFSQueue* q) {
    free(q->data);
}

static void BFSQueue_Push(BFSQueue* q, NodeP* node) {
    if (q->size >= q->capacity) {
        q->capacity *= 2;
        q->data = realloc(q->data, sizeof(NodeP*) * q->capacity);
    }
    q->data[q->size++] = node;
}

static NodeP* BFSQueue_Pop(BFSQueue* q, uint32_t* front) {
    if (*front >= q->size) return NULL;
    return q->data[(*front)++];
}

// --------------------
// Create BFS traversal (top-down)
BFS BFS_Create(NodeP* root) {
    BFS bfs = {0};

    if (!root) return bfs;

    // temporary queue
    BFSQueue queue;
    BFSQueue_Init(&queue, 64);

    // estimate node count (you can replace with actual tree->nodeCount)
    uint32_t capacity = 1024;
    bfs.nodes = malloc(sizeof(NodeP*) * capacity);
    bfs.count = 0;
    bfs.index = 0;

    uint32_t front = 0;
    BFSQueue_Push(&queue, root);

    while (NodeP* node = BFSQueue_Pop(&queue, &front)) {
        // grow array if needed
        if (bfs.count >= capacity) {
            capacity *= 2;
            bfs.nodes = realloc(bfs.nodes, sizeof(NodeP*) * capacity);
        }
        bfs.nodes[bfs.count++] = node;

        // push children into queue
        NodeP* child = node->firstChild;
        while (child) {
            BFSQueue_Push(&queue, child);
            child = child->nextSibling;
        }
    }

    BFSQueue_Free(&queue);
    return bfs;
}

// --------------------
// Create reverse BFS (bottom-up)
BFS BFS_Create_Reverse(NodeP* root) {
    BFS bfs = BFS_Create(root);
    // reverse the array
    for (uint32_t i = 0; i < bfs.count / 2; i++) {
        NodeP* tmp = bfs.nodes[i];
        bfs.nodes[i] = bfs.nodes[bfs.count - 1 - i];
        bfs.nodes[bfs.count - 1 - i] = tmp;
    }
    bfs.index = 0;
    return bfs;
}

// --------------------
// Iterate
inline NodeP* BFS_Next(BFS* bfs) {
    if (!bfs || bfs->index >= bfs->count) return NULL;
    return bfs->nodes[bfs->index++];
}

// --------------------
// Reset iteration to start
inline void BFS_Reset(BFS* bfs) {
    if (!bfs) return;
    bfs->index = 0;
}

// --------------------
// Free BFS memory
inline void BFS_Destroy(BFS* bfs) {
    if (!bfs) return;
    free(bfs->nodes);
    bfs->nodes = NULL;
    bfs->count = 0;
    bfs->index = 0;
}