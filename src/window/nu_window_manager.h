#pragma once
#include <window/nu_window_manager_structs.h>
#include <window/cursor.h>

void WindowManager_UpdateWindowRenderScale(WindowManager* winManager, int windowID)
{
    NU_Window* win = Container_Get(&winManager->windows, windowID);
    int winPtsW, winPtsH;
    int winPxW, winPxH;
    SDL_GetWindowSize(win->window, &winPtsW, &winPtsH);
    SDL_GetWindowSizeInPixels(win->window, &winPxW, &winPxH);
    win->renderScale = (float)winPxW / (float)winPtsW;
}

void WindowManager_UpdateWindowScreenDimensions(WindowManager* winManager, int windowID)
{
    NU_Window* win = Container_Get(&winManager->windows, windowID);
    SDL_DisplayID displayID = SDL_GetDisplayForWindow(win->window);
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayID);
    if (mode != NULL) {
        win->screenWidth = mode->w * win->renderScale;
        win->screenHeight = mode->h * win->renderScale;
    }
}

void CreateSubwindow(WindowManager* winManager, Node* node)
{
    // Create NU_Window
    NU_Window win;

    // Recycle an existing SDL window if there are any
    if (winManager->recycledSDLWindows.size > 0) {
        SDL_Window* sdlWin = *(SDL_Window**)Array_Get(&winManager->recycledSDLWindows, 0);
        Array_DeleteBackfill(&winManager->recycledSDLWindows, 0);
        win.window = sdlWin;
    }
    else {
        win.window = SDL_CreateWindow("", 500, 400, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    }
    win.windowNode = node;
    SDL_StartTextInput(win.window);

    // Init drawlist
    NU_WindowDrawlist* list = &win.drawlist;
    Array_Init(&list->drawNodes, sizeof(Node*), 512);
    Array_Init(&list->clippedDrawNodes, sizeof(Node*), 64);
    Array_Init(&list->canvasNodes, sizeof(Node*), 16);

    // Add NU_Window to Window Manager
    node->windowID = Container_Add(&winManager->windows, &win);
    WindowManager_UpdateWindowRenderScale(winManager, node->windowID);
    WindowManager_UpdateWindowScreenDimensions(winManager, node->windowID);
}

void WindowManager_DeleteSubwindow(WindowManager* winManager, Node* node)
{
    NU_Window* nuWin = Container_Get(&winManager->windows, node->windowID);

    // Set last clicked window to NULL if it was destroyed
    if (nuWin->window == winManager->lastClickedWindow) {
        winManager->lastClickedWindow = NULL;
    }

    // Recycle window if there are none or few (max 8) recycled windows
    if (winManager->recycledSDLWindows.size < 8) {
        Array_Push(&winManager->recycledSDLWindows, &nuWin->window);
    }
    else {
        SDL_DestroyWindow(nuWin->window);
    }

    // Free and delete nuWin
    Array_Free(&nuWin->drawlist.drawNodes);
    Array_Free(&nuWin->drawlist.clippedDrawNodes);
    Array_Free(&nuWin->drawlist.canvasNodes);
    Container_Remove(&winManager->windows, node->windowID);
}

void WindowManager_Init(WindowManager* winManager)
{
    // Init datastructures
    winManager->lastClickedWindow = NULL;
    winManager->windows = Container_Create(sizeof(NU_Window));
    Array_Init(&winManager->absoluteRootNodes, sizeof(Node*), 8);
    Array_Init(&winManager->recycledSDLWindows, sizeof(SDL_Window*), 8);
    Hashmap_Init(&winManager->clipMap, sizeof(Node*), sizeof(NU_ClipBounds), 16);

    // Create root NU_Window
    NU_Window win;
    win.window = SDL_CreateWindow("Window", 1000, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    win.windowNode = NULL; // Not assigned yet
    winManager->rootWindowID = Container_Add(&winManager->windows, &win);
    winManager->hoveredWindowNode = NULL;
    SDL_StartTextInput(win.window);

    // Init GL context
    GUI.gl_ctx = SDL_GL_CreateContext(win.window);
    SDL_GL_MakeCurrent(win.window, GUI.gl_ctx);
    SDL_GL_SetSwapInterval(0); // VSYNC ON

    // Init glew
    glewInit();
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glDepthFunc(GL_GEQUAL);
    glClearDepth(0.0);
}

void WindowManager_Free(WindowManager* winManager)
{
    for (uint32_t i=0; i<winManager->windows.size; i++) {
        NU_Window* win = Container_GetAt(&winManager->windows, i);
        Array_Free(&win->drawlist.drawNodes);
        Array_Free(&win->drawlist.clippedDrawNodes);
        Array_Free(&win->drawlist.canvasNodes);
    }
    Container_Free(&winManager->windows);
    Array_Free(&winManager->absoluteRootNodes);
    Array_Free(&winManager->recycledSDLWindows);
    Hashmap_Free(&winManager->clipMap);
    winManager->hoveredWindowNode = NULL;
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

inline NU_Window* GetNodeWindow(WindowManager* winManager, Node* node)
{
    return Container_Get(&winManager->windows, node->windowID);
}

SDL_Window* GetSDL_Window(WindowManager* winManager, int windowID)
{
    NU_Window* win = Container_Get(&winManager->windows, windowID);
    return win->window;
}

Node* GetWindowNodeFromSDL_Window(WindowManager* winManager, SDL_Window* window)
{
    for (int i=0; i<winManager->windows.size; i++) {
        NU_Window* win = Container_GetAt(&winManager->windows, i);
        if (win->window == window) {
            return win->windowNode;
        }
    }
    return NULL;
}

NU_WindowDrawlist* GetDrawlist(WindowManager* winManager, int windowID)
{
    NU_Window* win = Container_Get(&winManager->windows, windowID);
    return &win->drawlist;
}

void AssignRootWindow(WindowManager* winManager, Node* rootNode)
{
    SDL_Window* window = GetSDL_Window(winManager, winManager->rootWindowID);
    SDL_ShowWindow(window);

    int winW, winH;
    SDL_GetWindowSizeInPixels(window, &winW, &winH);
    rootNode->width = (float)winW;
    rootNode->height = (float)winH;
    rootNode->public.generic.minWidth = winW;
    rootNode->public.generic.maxWidth = winW;
    rootNode->public.generic.minHeight = winH;
    rootNode->public.generic.maxHeight = winH;
    rootNode->windowID = winManager->rootWindowID;
    NU_Window* rootWin = Container_Get(&winManager->windows, rootNode->windowID);
    rootWin->windowNode = rootNode;

    WindowManager_UpdateWindowRenderScale(winManager, rootNode->windowID);
    WindowManager_UpdateWindowScreenDimensions(winManager, rootNode->windowID);

    // Initialise drawlist
    NU_WindowDrawlist* list = GetDrawlist(winManager, winManager->rootWindowID);
    Array_Init(&list->drawNodes, sizeof(Node*), 512);
    Array_Init(&list->clippedDrawNodes, sizeof(Node*), 64);
    Array_Init(&list->canvasNodes, sizeof(Node*), 16);

    NU_Draw_Init();
}

void GetLocalMouseCoords(WindowManager* winManager, float* outX, float* outY)
{
    NU_Window* win = Container_Get(&winManager->windows, winManager->hoveredWindowNode->windowID);
    float globalPtsX, globalPtsY;
    int windowPtsX, windowPtsY;
    SDL_GetGlobalMouseState(&globalPtsX, &globalPtsY);
    SDL_GetWindowPosition(win->window, &windowPtsX, &windowPtsY);
    *outX = (globalPtsX - windowPtsX) * win->renderScale;
    *outY = (globalPtsY - windowPtsY) * win->renderScale;
}

void WindowManager_SetHoveredWindow(WindowManager* winManager, SDL_Window* window)
{
    for (int i=0; i<winManager->windows.size; i++) {
        NU_Window* win = Container_GetAt(&winManager->windows, i);
        if (win->window == window) {
            winManager->hoveredWindowNode = win->windowNode;
            return;
        }
    }
    winManager->hoveredWindowNode = NULL;
}

inline void WindowBeginFrame(SDL_Window* window)
{
    int w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    glViewport(0, 0, w, h); glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

inline void SetNodeDrawlist_Draw(WindowManager* winManager, Node* node)
{
    NU_WindowDrawlist* list = GetDrawlist(winManager, node->windowID);
    Array_Push(&list->drawNodes, &node);
}

inline void SetNodeDrawlist_Clipped(WindowManager* winManager, Node* node)
{
    NU_WindowDrawlist* list = GetDrawlist(winManager, node->windowID);
    Array_Push(&list->clippedDrawNodes, &node);
}

inline void SetNodeDrawlist_Canvas(WindowManager* winManager, Node* node)
{
    NU_WindowDrawlist* list = GetDrawlist(winManager, node->windowID);
    Array_Push(&list->canvasNodes, &node);
}
