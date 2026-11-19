

void Windows_Init(Windows* windows)
{
    // Zero memory
    memset(windows->blocks, 0, sizeof(WindowsBlock) * NU_MAX_WINDOW_COUNT);

    // Create freelist
    windows->blocks[NU_MAX_WINDOW_COUNT - 1].free.nextIndex = -1;
    for (int i=0; i<NU_MAX_WINDOW_COUNT - 1; i++) {
        windows->blocks[i].free.prevIndex = i - 1;
        windows->blocks[i].free.nextIndex = i + 1;
    }

    // Init book keeping
    windows->freeHead = 0;
    windows->windowCount = 0;
}

int8 Windows_AddWindow(Windows* windows)
{
    if (windows->freeHead == -1) return -1;

    // Get free block
    int8 windowID = windows->freeHead;

    // Read the freelist link BEFORE zeroing the union — free and window
    // share storage, so the memset below would otherwise clobber nextIndex.
    int8 nextFree = windows->blocks[windowID].free.nextIndex;

    // Zero window memory (this wipes the union, including the freelist fields)
    memset(&windows->blocks[windowID].window, 0, sizeof(Window));

    // Update freelist using the saved link
    windows->freeHead = nextFree;
    if (windows->freeHead != -1)
        windows->blocks[windows->freeHead].free.prevIndex = -1;

    // Update index list
    windows->windowIndices[windows->windowCount] = windowID;
    windows->windowIndexPositions[windowID] = windows->windowCount;
    windows->windowCount++;
    return windowID;
}

void Windows_RemoveWindow(Windows* windows, int8 windowID)
{
    if (windowID < 0 || windowID >= NU_MAX_WINDOW_COUNT) return;

    // Remove from index list (swap with last)
    int8 position = windows->windowIndexPositions[windowID];
    int8 lastWindowID = windows->windowIndices[windows->windowCount - 1];
    windows->windowIndices[position] = lastWindowID;
    windows->windowIndexPositions[lastWindowID] = position;

    // Push released block onto free list head
    WindowsFree* freeBlock = &windows->blocks[windowID].free;
    freeBlock->prevIndex = -1;
    freeBlock->nextIndex = windows->freeHead;

    // Update bookkeeping
    if (windows->freeHead != -1) windows->blocks[windows->freeHead].free.prevIndex = windowID;
    windows->windowCount--;
    windows->freeHead = windowID;
}

inline Window* Windows_Window(Windows* windows, int8 windowID)
{
    return &windows->blocks[windowID].window;
}

inline Window* Windows_WindowAt(Windows* windows, int index)
{
    return &windows->blocks[windows->windowIndices[index]].window;
}

inline int8 Windows_IDAt(Windows* windows, int index)
{
    return windows->windowIndices[index];
}

inline int Windows_Count(Windows* windows)
{
    return windows->windowCount;
}
