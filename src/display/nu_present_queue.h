#pragma once

typedef struct {
    int8 nextFreeIndex;
    int8 prevFreeIndex;
} PresentQueueFreeItem;

typedef struct {
    Window* window;
    int8 nextWindowIndex;
} PresentQueueWindow;

typedef union {
    PresentQueueWindow queueWindow;
    PresentQueueFreeItem freeItem;
} PresentQueueWindowBlock

typedef struct {
    uint64 presentDeadline;
    uint64 refreshTime;
    SDL_DisplayID displayID;
    int8 firstWindowIndex;
} PresentQueueDisplay;

typedef union {
    PresentQueueDisplay queueDisplay;
    PresentQueueFreeItem freeItem;
    uint64 framePresentDeadline;
} PresentQueueDisplayBlock;

typedef struct {
    Window* windowWaitlist[64]; uint8 waitingCount; // The array that the main thread adds to
    SDL_Mutex windowWaitlistMutex;
    PresentQueueDisplayBlock queueDisplayBlocks[64];
    PresentQueueWindowBlock queueWindowBlocks[64];
    uint8 sortedDisplayIndices[64];
    uint8 queueDisplayCount;
    uint8 queueWindowCount;
    int8 displayFreelistHeadIndex;
    int8 windowFreelistHeadIndex;
} PresentQueue;

void PresentQueue_Init(PresentQueue* queue)
{
    // Init memory
    memset(queue->queueDisplayBlocks, sizeof(PresentQueueDisplayBlock) * 64, 0);
    memset(queue->queueWindowBlocks, sizeof(PresentQueueWindowBlock) * 64, 0);

    // Create display block array free list
    PresentQueueDisplayBlock* lastDisplayBlock = &queue->queueDisplayBlocks[63];
    lastDisplayBlock->freeItem.prevFreeIndex = 62;
    lastDisplayBlock->freeItem.nextFreeIndex = -1;
    for (int i=0; i<63; i++) {
        queue->queueDisplayBlocks[i].freeItem.prevFreeIndex = i - 1;
        queue->queueDisplayBlocks[i].freeItem.nextFreeIndex = i + 1;
    }

    // Create window block array free lists
    PresentQueueWindowBlock* lastWindowBlock = &queue->queueWindowBlocks[63];
    lastWindowBlock->freeItem.prevFreeIndex = 62;
    lastWindowBlock->freeItem.nextFreeIndex = -1;
    for (int i=0; i<63; i++) {
        queue->queueWindowBlocks[i].freeItem.prevFreeIndex = i - 1;
        queue->queueWindowBlocks[i].freeItem.nextFreeIndex = i + 1;
    }

    queue->queueDisplayCount = 0;
    queue->queueWindowCount = 0;
    queue->displayFreelistHeadIndex = 0;
    queue->windowFreelistHeadIndex = 0;
    queue->waitingCount = 0;
}

void PresentQueue_AddWindow(PresentQueue* queue, Window* window)
{
    // Return if window is already in the list
    for (uint8 i=0; i<queue->waitingCount; i++)
    {
        if (queue->windowWaitlist[i] == window) return;
    }

    // Add window to the list
    if (queue->waitingCount == 64) return; // Out of space
    queue->windowWaitlist[queue->waitingCount++] = window;
}

int PresentQueue_AddDisplay(PresentQueue* queue, SDL_DisplayID displayID)
{
    // Reached capacity
    if (queue->queueDisplayCount == 64) return;

    // Add a new display block
    PresentQueueDisplayBlock* newDisplayBlock = &queue->queueDisplayBlocks[queue->displayFreelistHeadIndex];
    queue->displayFreelistHeadIndex = newDisplayBlock->freeItem.nextFreeIndex;
    queue->queueDisplayBlocks[queue->displayFreelistHeadIndex].freeItem.prevFreeIndex = -1;

    // Set display properties
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayID);
    if (mode && mode->refresh_rate_numerator > 0 && mode->refresh_rate_denominator > 0)
    {
        newDisplayBlock->queueDisplay.refreshTime = 1000000000ULL * (uint64)mode->refresh_rate_denominator / (uint64)mode->refresh_rate_numerator;
        newDisplayBlock->queueDisplay.presentDeadline = 0;
    }

    // No windows added yet
    newDisplayBlock->queueDisplay.firstWindowIndex = -1;
}


void PresentQueue_AddReadyWindowsToDisplays(PresentQueue* queue)
{
    for (uint8 i=0; i<queue->waitingCount; i++)
    {
        if (queue->windowWaitlist[i].window->presentDeadline > queue->framePresentDeadline) continue;
        // Add to the correct display bucket
        // ....
        // ....
    }

    // Also sort the displays with the sorted display indices thing
}

PresentQueueDisplay* PresentQueue_GetNextDisplayToWaitFor(PresentQueue* queue)
{
    // uSe the sorted display indices
}
