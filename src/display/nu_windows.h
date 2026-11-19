#pragma once

#define NU_MAX_WINDOW_COUNT 16

typedef struct {
    int8 nextIndex;
    int8 prevIndex;
} WindowsFree;

typedef union {
    Window window;
    WindowsFree free;
} WindowsBlock;

typedef struct {
    WindowsBlock blocks[NU_MAX_WINDOW_COUNT];
    int8 windowIndices[NU_MAX_WINDOW_COUNT];
    int8 windowIndexPositions[NU_MAX_WINDOW_COUNT];
    uint8 windowCount;
    int8 freeHead;
} Windows;

void Windows_Init(Windows* windows);

int8 Windows_AddWindow(Windows* windows);

void Windows_RemoveWindow(Windows* windows, int8 windowID);

inline Window* Windows_Window(Windows* windows, int8 windowID);

inline Window* Windows_WindowAt(Windows* windows, int index);

inline int8 Windows_IDAt(Windows* windows, int index);

inline int Windows_Count(Windows* windows);
