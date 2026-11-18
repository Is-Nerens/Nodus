#pragma once

#define NU_MAX_WINDOW_COUNT 16

typedef struct {
    int8 nextIndex;
    int8 prevIndex;
} WinallocFree;

typedef union {
    Window window;
    WinallocFree free;
} WinallocBlock;

typedef struct {
    WinallocBlock blocks[NU_MAX_WINDOW_COUNT];
    int8 windowIndices[NU_MAX_WINDOW_COUNT];
    int8 windowIndexPositions[NU_MAX_WINDOW_COUNT];
    uint8 windowCount;
    int8 freeHead;
} Winalloc;

void Winalloc_Init(Winalloc* winalloc);

int8 Winalloc_AddWindow(Winalloc* winalloc);

void Winalloc_RemoveWindow(Winalloc* winalloc, int8 windowID);

inline Window* Winalloc_Window(Winalloc* winalloc, int8 windowID);

inline Window* Winalloc_WindowAt(Winalloc* winalloc, int index);

inline int8 Winalloc_IDAt(Winalloc* winalloc, int index);

inline int Winalloc_Count(Winalloc* winalloc);
