#pragma once 
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <tree/nu_node.h>

typedef struct NU_WindowDrawlist
{
    Array drawNodes;
    Array clippedDrawNodes;
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
    Array windowNodes;
    Array absoluteRootNodes;
    Hashmap clipMap;
    int hoveredWindowID;
    int rootWindowID;
} NU_WindowManager;