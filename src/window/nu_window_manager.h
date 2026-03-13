#pragma once 
#include <window/nu_window_manager_structs.h>

void InitGlew(NU_WindowManager* winManager)
{
    // Create NU_Window
    NU_Window win;
    win.window = SDL_CreateWindow("Window", 1000, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    // Init gl context
    __NGUI.gl_ctx = SDL_GL_CreateContext(win.window);
    SDL_GL_MakeCurrent(win.window, __NGUI.gl_ctx);
    SDL_GL_SetSwapInterval(0); // VSYNC ON

    // Add NU_Window to Window Manager
    winManager->rootWindowID = Container_Add(&winManager->windows, &win);
    
    // Init glew
    glewInit();
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
}

void CreateSubwindow(NU_WindowManager* winManager, NodeP* node)
{
    // Create NU_Window
    NU_Window win;
    win.window = SDL_CreateWindow("window", 500, 400, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    // Init drawlist
    NU_WindowDrawlist* list = &win.drawlist;
    Vector_Reserve(&list->relativeNodes, sizeof(NodeP*), 512);
    Vector_Reserve(&list->absoluteNodes, sizeof(NodeP*), 64);
    Vector_Reserve(&list->canvasNodes,   sizeof(NodeP*), 8);
    Vector_Reserve(&list->clippedRelativeNodes, sizeof(NodeP*), 64);
    Vector_Reserve(&list->clippedAbsoluteNodes, sizeof(NodeP*), 16);
    Vector_Reserve(&list->clippedCanvasNodes,   sizeof(NodeP*), 4);

    // Add NU_Window to Window Manager
    node->windowID = Container_Add(&winManager->windows, &win);
    Vector_Push(&winManager->windowNodes, &node);
}

void NU_WindowManagerInit(NU_WindowManager* winManager)
{
    winManager->windows = Container_Create(sizeof(NU_Window));
    Vector_Reserve(&winManager->windowNodes, sizeof(NodeP*), 8);
    Vector_Reserve(&winManager->absoluteRootNodes, sizeof(NodeP*), 8);
    HashmapInit(&winManager->clipMap, sizeof(NodeP*), sizeof(NU_ClipBounds), 16);
    InitGlew(winManager);
    winManager->hoveredWindowID = -1;
}

void NU_WindowManagerFree(NU_WindowManager* winManager)
{
    for (uint32_t i=0; i<winManager->windows.size; i++) {
        NU_Window* win = Container_GetAt(&winManager->windows, i);
        Vector_Free(&win->drawlist.relativeNodes);
        Vector_Free(&win->drawlist.absoluteNodes);
        Vector_Free(&win->drawlist.canvasNodes);
        Vector_Free(&win->drawlist.clippedRelativeNodes);
        Vector_Free(&win->drawlist.clippedAbsoluteNodes);
        Vector_Free(&win->drawlist.clippedCanvasNodes);
    }
    Container_Free(&winManager->windows);
    Vector_Free(&winManager->windowNodes);
    Vector_Free(&winManager->absoluteRootNodes);
    HashmapFree(&winManager->clipMap);
    winManager->hoveredWindowID = -1;
}

int GetFrametime()
{
    int frameTimeMs = 16;
    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(display);
    if (mode && mode->refresh_rate > 0) {
        frameTimeMs = 1000 / mode->refresh_rate;
    }
    return frameTimeMs;
}

SDL_Window* GetSDL_Window(NU_WindowManager* winManager, int windowID)
{
    NU_Window* win = Container_Get(&winManager->windows, windowID);
    return win->window;
}

NU_WindowDrawlist* GetDrawlist(NU_WindowManager* winManager, int windowID)
{
    NU_Window* win = Container_Get(&winManager->windows, windowID);
    return &win->drawlist;
}

void AssignRootWindow(NU_WindowManager* winManager, NodeP* rootNode)
{
    SDL_Window* window = GetSDL_Window(winManager, winManager->rootWindowID);
    SDL_ShowWindow(window);

    int winW, winH;
    SDL_GetWindowSize(window, &winW, &winH);
    rootNode->node.width = (float)winW;
    rootNode->node.height = (float)winH;
    rootNode->node.minWidth = winW;
    rootNode->node.maxWidth = winW;
    rootNode->node.minHeight = winH;
    rootNode->node.maxHeight = winH;
    rootNode->windowID = winManager->rootWindowID;
    Vector_Push(&winManager->windowNodes, &rootNode);

    // Initialise drawlist
    NU_WindowDrawlist* list = GetDrawlist(winManager, winManager->rootWindowID);
    Vector_Reserve(&list->relativeNodes, sizeof(NodeP*), 512);
    Vector_Reserve(&list->absoluteNodes, sizeof(NodeP*), 64);
    Vector_Reserve(&list->canvasNodes,   sizeof(NodeP*), 8);
    Vector_Reserve(&list->clippedRelativeNodes, sizeof(NodeP*), 64);
    Vector_Reserve(&list->clippedAbsoluteNodes, sizeof(NodeP*), 16);
    Vector_Reserve(&list->clippedCanvasNodes,   sizeof(NodeP*), 4);

    NU_Draw_Init();
}

void GetLocalMouseCoords(NU_WindowManager* winManager, float* outX, float* outY)
{
    float globalX, globalY;
    int windowX, windowY;
    SDL_GetGlobalMouseState(&globalX, &globalY);
    SDL_Window* hoveredWindow = GetSDL_Window(winManager, winManager->hoveredWindowID);
    SDL_GetWindowPosition(hoveredWindow, &windowX, &windowY);
    *outX = globalX - windowX;
    *outY = globalY - windowY;
}

inline int InHoverredWindow(NU_WindowManager* winManager, NodeP* node)
{
    return node->windowID == winManager->hoveredWindowID;
}

void SetHoveredWindowID(NU_WindowManager* winManager, SDL_Window* window)
{
    for (int i=0; i<winManager->windows.size; i++) {
        NU_Window* win = Container_GetAt(&winManager->windows, i);
        if (win->window == window) {
            winManager->hoveredWindowID = Container_IdAt(&winManager->windows, i);
            break;
        }
    }
}

inline void WindowBeginFrame(SDL_Window* window)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    glViewport(0, 0, w, h); glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

inline void SetNodeDrawlist_Relative(NU_WindowManager* winManager, NodeP* node)
{
    NU_WindowDrawlist* list = GetDrawlist(winManager, node->windowID);
    Vector_Push(&list->relativeNodes, &node);
}

inline void SetNodeDrawlist_Absolute(NU_WindowManager* winManager, NodeP* node)
{
    NU_WindowDrawlist* list = GetDrawlist(winManager, node->windowID);
    Vector_Push(&list->absoluteNodes, &node);
}

inline void SetNodeDrawlist_Canvas(NU_WindowManager* winManager, NodeP* node)
{   
    NU_WindowDrawlist* list = GetDrawlist(winManager, node->windowID);
    Vector_Push(&list->canvasNodes, &node);
}

inline void SetNodeDrawlist_ClippedRelative(NU_WindowManager* winManager, NodeP* node)
{
    NU_WindowDrawlist* list = GetDrawlist(winManager, node->windowID);
    Vector_Push(&list->clippedRelativeNodes, &node);
}   

inline void SetNodeDrawlist_ClippedAbsolute(NU_WindowManager* winManager, NodeP* node)
{
    NU_WindowDrawlist* list = GetDrawlist(winManager, node->windowID);
    Vector_Push(&list->clippedAbsoluteNodes, &node);
}

inline void SetNodeDrawlist_ClippedCanvas(NU_WindowManager* winManager, NodeP* node)
{
    NU_WindowDrawlist* list = GetDrawlist(winManager, node->windowID);
    Vector_Push(&list->clippedCanvasNodes, &node);
}