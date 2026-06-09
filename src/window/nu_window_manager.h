#pragma once 
#include <window/nu_window_manager_structs.h>
#include <window/cursor.h>

void InitGlew(WindowManager* winManager)
{
    // Create NU_Window
    NU_Window win;
    win.window = SDL_CreateWindow("Window", 1000, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    // Init gl context
    GUI.gl_ctx = SDL_GL_CreateContext(win.window);
    SDL_GL_MakeCurrent(win.window, GUI.gl_ctx);
    SDL_GL_SetSwapInterval(0); // VSYNC ON

    // Add NU_Window to Window Manager
    winManager->rootWindowID = Container_Add(&winManager->windows, &win);
    
    // Init glew
    glewInit();
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glDepthFunc(GL_GEQUAL);
    glClearDepth(0.0);
}

void CreateSubwindow(WindowManager* winManager, NodeP* node)
{
    // Create NU_Window
    NU_Window win;
    win.window = SDL_CreateWindow("window", 500, 400, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    // Init drawlist
    NU_WindowDrawlist* list = &win.drawlist;
    Array_Init(&list->drawNodes, sizeof(NodeP*), 512);
    Array_Init(&list->clippedDrawNodes, sizeof(NodeP*), 64);

    // Add NU_Window to Window Manager
    node->windowID = Container_Add(&winManager->windows, &win);
    Array_Push(&winManager->windowNodes, &node);
}

void WindowManager_Init(WindowManager* winManager)
{
    winManager->windows = Container_Create(sizeof(NU_Window));
    Array_Init(&winManager->windowNodes, sizeof(NodeP*), 8);
    Array_Init(&winManager->absoluteRootNodes, sizeof(NodeP*), 8);
    Hashmap_Init(&winManager->clipMap, sizeof(NodeP*), sizeof(NU_ClipBounds), 16);
    InitGlew(winManager);
    winManager->hoveredWindowID = -1;
}

void WindowManager_Free(WindowManager* winManager)
{
    for (uint32_t i=0; i<winManager->windows.size; i++) {
        NU_Window* win = Container_GetAt(&winManager->windows, i);
        Array_Free(&win->drawlist.drawNodes);
        Array_Free(&win->drawlist.clippedDrawNodes);
    }
    Container_Free(&winManager->windows);
    Array_Free(&winManager->windowNodes);
    Array_Free(&winManager->absoluteRootNodes);
    Hashmap_Free(&winManager->clipMap);
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

SDL_Window* GetSDL_Window(WindowManager* winManager, int windowID)
{
    NU_Window* win = Container_Get(&winManager->windows, windowID);
    return win->window;
}

NU_WindowDrawlist* GetDrawlist(WindowManager* winManager, int windowID)
{
    NU_Window* win = Container_Get(&winManager->windows, windowID);
    return &win->drawlist;
}

void AssignRootWindow(WindowManager* winManager, NodeP* rootNode)
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
    Array_Push(&winManager->windowNodes, &rootNode);

    // Initialise drawlist
    NU_WindowDrawlist* list = GetDrawlist(winManager, winManager->rootWindowID);
    Array_Init(&list->drawNodes, sizeof(NodeP*), 512);
    Array_Init(&list->clippedDrawNodes, sizeof(NodeP*), 64);

    NU_Draw_Init();
}

void GetLocalMouseCoords(WindowManager* winManager, float* outX, float* outY)
{
    float globalX, globalY;
    int windowX, windowY;
    SDL_GetGlobalMouseState(&globalX, &globalY);
    SDL_Window* hoveredWindow = GetSDL_Window(winManager, winManager->hoveredWindowID);
    SDL_GetWindowPosition(hoveredWindow, &windowX, &windowY);
    *outX = globalX - windowX;
    *outY = globalY - windowY;
}

inline int InHoverredWindow(WindowManager* winManager, NodeP* node)
{
    return node->windowID == winManager->hoveredWindowID;
}

void SetHoveredWindowID(WindowManager* winManager, SDL_Window* window)
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

inline void SetNodeDrawlist_Draw(WindowManager* winManager, NodeP* node)
{
    NU_WindowDrawlist* list = GetDrawlist(winManager, node->windowID);
    Array_Push(&list->drawNodes, &node);
}

inline void SetNodeDrawlist_Clipped(WindowManager* winManager, NodeP* node)
{
    NU_WindowDrawlist* list = GetDrawlist(winManager, node->windowID);
    Array_Push(&list->clippedDrawNodes, &node);
}   