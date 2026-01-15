#pragma once 
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <datastructures/vector.h>
#include <datastructures/hashmap.h>
#include <tree/nu_node.h>

typedef struct NU_WindowDrawlist
{
    Vector relativeNodes;
    Vector absoluteNodes;
    Vector canvasNodes;
    Vector clippedRelativeNodes;
    Vector clippedAbsoluteNodes;
    Vector clippedCanvasNodes;
} NU_WindowDrawlist;

// Responsible for all window related functionality
typedef struct NU_WindowManager
{   
    Vector windows;
    Vector windowNodes;
    Vector absoluteRootNodes;
    Vector windowDrawLists;
    Hashmap clipMap;
    SDL_Window* hoveredWindow;
} NU_WindowManager;