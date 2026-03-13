#pragma once 
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <datastructures/vector.h>
#include <datastructures/hashmap.h>
#include <datastructures/container.h>
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

typedef struct NU_Window
{
    SDL_Window* window;
    NU_WindowDrawlist drawlist;
} NU_Window;

// Responsible for all window related functionality
typedef struct NU_WindowManager
{   
    Container windows;
    Vector windowNodes;
    Vector absoluteRootNodes;
    Hashmap clipMap;
    int hoveredWindowID;
    int rootWindowID;
} NU_WindowManager;