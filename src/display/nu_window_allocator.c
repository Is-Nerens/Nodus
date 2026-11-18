

void Winalloc_Init(Winalloc* winalloc)
{
    // Zero memory
    memset(winalloc->blocks, 0, sizeof(WinallocBlock) * NU_MAX_WINDOW_COUNT);

    // Create freelist
    winalloc->blocks[NU_MAX_WINDOW_COUNT - 1].free.nextIndex = -1;
    for (int i=0; i<NU_MAX_WINDOW_COUNT - 1; i++) {
        winalloc->blocks[i].free.prevIndex = i - 1;
        winalloc->blocks[i].free.nextIndex = i + 1;
    }

    // Init book keeping
    winalloc->freeHead = 0;
    winalloc->windowCount = 0;
}

int8 Winalloc_AddWindow(Winalloc* winalloc)
{
    if (winalloc->freeHead == -1) return -1;

    // Get free block
    int8 windowID = winalloc->freeHead;

    // Read the freelist link BEFORE zeroing the union — free and window
    // share storage, so the memset below would otherwise clobber nextIndex.
    int8 nextFree = winalloc->blocks[windowID].free.nextIndex;

    // Zero window memory (this wipes the union, including the freelist fields)
    memset(&winalloc->blocks[windowID].window, 0, sizeof(Window));

    // Update freelist using the saved link
    winalloc->freeHead = nextFree;
    if (winalloc->freeHead != -1)
        winalloc->blocks[winalloc->freeHead].free.prevIndex = -1;

    // Update index list
    winalloc->windowIndices[winalloc->windowCount] = windowID;
    winalloc->windowIndexPositions[windowID] = winalloc->windowCount;
    winalloc->windowCount++;
    return windowID;
}

void Winalloc_RemoveWindow(Winalloc* winalloc, int8 windowID)
{
    if (windowID < 0 || windowID >= NU_MAX_WINDOW_COUNT) return;

    // Remove from index list (swap with last)
    int8 position = winalloc->windowIndexPositions[windowID];
    int8 lastWindowID = winalloc->windowIndices[winalloc->windowCount - 1];
    winalloc->windowIndices[position] = lastWindowID;
    winalloc->windowIndexPositions[lastWindowID] = position;

    // Push released block onto free list head
    WinallocFree* freeBlock = &winalloc->blocks[windowID].free;
    freeBlock->prevIndex = -1;
    freeBlock->nextIndex = winalloc->freeHead;

    // Update bookkeeping
    if (winalloc->freeHead != -1) winalloc->blocks[winalloc->freeHead].free.prevIndex = windowID;
    winalloc->windowCount--;
    winalloc->freeHead = windowID;
}

inline Window* Winalloc_Window(Winalloc* winalloc, int8 windowID)
{
    return &winalloc->blocks[windowID].window;
}

inline Window* Winalloc_WindowAt(Winalloc* winalloc, int index)
{
    return &winalloc->blocks[winalloc->windowIndices[index]].window;
}

inline int8 Winalloc_IDAt(Winalloc* winalloc, int index)
{
    return winalloc->windowIndices[index];
}

inline int Winalloc_Count(Winalloc* winalloc)
{
    return winalloc->windowCount;
}
