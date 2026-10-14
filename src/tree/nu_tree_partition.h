#pragma once

#define TREE_PARTITION_MAX 32
#define TREE_PARTITION_MIN_NODES 256

typedef enum {
    TREE_PARTITION_TRAVERSE_DIRECTION_UP,
    TREE_PARTITION_TRAVERSE_DIRECTION_DOWN,
    TREE_PARTITION_TRAVERSE_DIRECTION_NONE
} TreePartitionTraverseDirection;

typedef struct TreePartition {
    Node* root;
    u32 childCount; // Estimated workload
} TreePartition;

typedef struct TreePartitioner {
    TreePartition partitions[TREE_PARTITION_MAX];
    int partitionCount;
    int nextPartition;
} TreePartitioner;

// Advance in preorder without leaving the given subtree.
static Node* TreePartitioner_NextNode(Node* node, Node* root)
{
    if (node->firstChild) return node->firstChild;
    while (node != root) {
        if (node->nextSibling) return node->nextSibling;
        node = node->parent;
    }
    return NULL;
}


// Clear boundaries left over from the previous partitioning.
static void TreePartitioner_ClearFlags(Node* root)
{
    for (Node* node=root; node; node=TreePartitioner_NextNode(node, root)) {
        node->isPartitionRoot = 0;
    }
}


// Add a partition immediately before the final root partition.
static int TreePartitioner_AddPartition(TreePartitioner* part, Node* root)
{
    // Reserve the final slot for the root partition.
    if (part->partitionCount >= TREE_PARTITION_MAX) return 0;
    int index = part->partitionCount - 1;

    // Shift the root partition to the right.
    part->partitions[index + 1] = part->partitions[index];
    part->partitions[index].root = root;
    part->partitions[index].childCount = root->subtreeSize;
    part->partitionCount++;
    root->isPartitionRoot = 1;
    return 1;
}

// Forward declaration for recursion.
// Recursively partition `subtreeRoot`'s children into disjoint partitions.
// Returns the number of partitions created. Appends to `part->partitions[]`.
// Never claims `subtreeRoot` itself — it's a boundary owned by the parent.
static int TreePartitioner_PartitionRecursive(
    TreePartitioner* part,
    Node* subtreeRoot,
    u32 targetSize,
    int remainingSlots)
{
    if (remainingSlots <= 0) return 0;

    // Collect the largest children of subtreeRoot that are candidates
    // for becoming partitions. We only look at *direct children* of
    // subtreeRoot — deeper descendants are handled by recursion.
    Node* candidates[TREE_PARTITION_MAX];
    u32 sizes[TREE_PARTITION_MAX];
    int count = 0;

    for (Node* c = subtreeRoot->firstChild; c; c = c->nextSibling) {
        if (!c->firstChild) continue;  // leaf: can't be a partition root
        u32 size = c->subtreeSize;
        if (size < TREE_PARTITION_MIN_NODES) continue;  // too small to be its own partition

        // Insert in descending order.
        if (count == TREE_PARTITION_MAX && size <= sizes[count - 1]) continue;
        int index = count;
        if (index >= TREE_PARTITION_MAX) index = TREE_PARTITION_MAX - 1;
        else count++;

        while (index > 0 && sizes[index - 1] < size) {
            candidates[index] = candidates[index - 1];
            sizes[index] = sizes[index - 1];
            index--;
        }
        candidates[index] = c;
        sizes[index] = size;
    }

    int made = 0;
    for (int i = 0; i < count && made < remainingSlots; i++) {
        Node* cand = candidates[i];
        u32 size = sizes[i];

        u32 upperBound = targetSize + targetSize / 2;
        u32 lowerBound = targetSize / 2;
        if (lowerBound < TREE_PARTITION_MIN_NODES) lowerBound = TREE_PARTITION_MIN_NODES;

        if (size <= upperBound) {
            // In band (or below upper but >= TREE_PARTITION_MIN_NODES): cut here.
            cand->isPartitionRoot = 1;
            part->partitions[part->partitionCount].root = cand;
            part->partitions[part->partitionCount].childCount = size;
            part->partitionCount++;
            made++;
            continue;
        }

        // size > upperBound: try to descend, but only if the children
        // can themselves form partitions of at least TREE_PARTITION_MIN_NODES.
        // Otherwise cut here even though it's oversized.
        u32 bestChild = 0;
        for (Node* c = cand->firstChild; c; c = c->nextSibling) {
            if (c->subtreeSize > bestChild) bestChild = c->subtreeSize;
        }

        if (bestChild < TREE_PARTITION_MIN_NODES) {
            // Children are all below the minimum. Cut here.
            cand->isPartitionRoot = 1;
            part->partitions[part->partitionCount].root = cand;
            part->partitions[part->partitionCount].childCount = size;
            part->partitionCount++;
            made++;
            continue;
        }

        // Recurse into it. `cand` itself remains a boundary node owned
        // by whichever partition owns subtreeRoot.
        int sub = TreePartitioner_PartitionRecursive(part, cand, targetSize, remainingSlots - made);
        made += sub;
    }

    return made;
}

int TreePartitioner_Partition(TreePartitioner* part, Node* root, int partitionCount)
{
    if (!part || !root || partitionCount < 1) return 0;
    if (partitionCount > TREE_PARTITION_MAX) partitionCount = TREE_PARTITION_MAX;

    part->partitionCount = 0;
    part->nextPartition = 0;

    TreePartitioner_ClearFlags(root);

    u32 totalSize = root->subtreeSize;
    u32 targetSize = totalSize / (u32)partitionCount;
    if (targetSize == 0) targetSize = 1;

    // If we can't even carve out one min-sized partition, don't bother —
    // just return the root partition.
    if (totalSize < TREE_PARTITION_MIN_NODES * 2) {
        part->partitions[0].root = root;
        part->partitions[0].childCount = totalSize;
        root->isPartitionRoot = 1;
        part->partitionCount = 1;
        return 1;
    }

    int made = TreePartitioner_PartitionRecursive(part, root, targetSize, partitionCount - 1);

    // Root partition is always last, owns whatever wasn't claimed.
    u32 claimedSize = 0;
    for (int i = 0; i < made; i++) claimedSize += part->partitions[i].childCount;
    u32 rootRemainder = (totalSize > claimedSize) ? (totalSize - claimedSize) : 1;

    part->partitions[made].root = root;
    part->partitions[made].childCount = rootRemainder;
    root->isPartitionRoot = 1;
    part->partitionCount = made + 1;
    return part->partitionCount;
}

Node* TreePartitioner_GetNextPartition(TreePartitioner* part, TreePartitionTraverseDirection dir)
{
    if (!part) return NULL;
    if (part->nextPartition >= part->partitionCount) {
        part->nextPartition = 0;
        return NULL;
    }
    int index;

    switch (dir)
    {
        // Descendants first, root partition last
        case TREE_PARTITION_TRAVERSE_DIRECTION_UP:
            index = part->partitionCount - 1 - part->nextPartition;
            break;
        // Root partition first
        case TREE_PARTITION_TRAVERSE_DIRECTION_DOWN:
        default:
            index = part->nextPartition;
            break;
    }
    part->nextPartition++;
    return part->partitions[index].root;
}
