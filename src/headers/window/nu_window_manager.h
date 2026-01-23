#pragma once 
#include <window/nu_window_manager_structs.h>

void CreateMainWindow(NU_WindowManager* winManager)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_SetHint("SDL_MOUSE_FOCUS_CLICKTHROUGH", "1");
    SDL_Window* window = SDL_CreateWindow("Window", 1000, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(0); // VSYNC ON
    __NGUI.gl_ctx = context;
    Vector_Push(&winManager->windows, &window);
    glewInit();
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
}

void NU_WindowManagerInit(NU_WindowManager* winManager)
{
    Vector_Reserve(&winManager->windows, sizeof(SDL_Window*), 8);
    Vector_Reserve(&winManager->windowNodes, sizeof(uint32_t), 8);
    Vector_Reserve(&winManager->absoluteRootNodes, sizeof(NodeP*), 8);
    Vector_Reserve(&winManager->windowDrawLists, sizeof(NU_WindowDrawlist), 8);
    HashmapInit(&winManager->clipMap, sizeof(uint32_t), sizeof(NU_ClipBounds), 16);
    CreateMainWindow(winManager);
    winManager->hoveredWindow = NULL;
}

void NU_WindowManagerFree(NU_WindowManager* winManager)
{
    Vector_Free(&winManager->windows);
    Vector_Free(&winManager->windowNodes);
    Vector_Free(&winManager->absoluteRootNodes);
    for (uint32_t i=0; i<winManager->windowDrawLists.size; i++) {
        NU_WindowDrawlist* list = Vector_Get(&winManager->windowDrawLists, i);
        Vector_Free(&list->relativeNodes);
        Vector_Free(&list->absoluteNodes);
        Vector_Free(&list->canvasNodes);
        Vector_Free(&list->clippedRelativeNodes);
        Vector_Free(&list->clippedAbsoluteNodes);
        Vector_Free(&list->clippedCanvasNodes);
    }
    Vector_Free(&winManager->windowDrawLists);
    HashmapFree(&winManager->clipMap);
    winManager->hoveredWindow = NULL;
}

inline void GetWindowSize(SDL_Window* window, float* wOut, float* hOut)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    *wOut = (float)w; *hOut = (float)h;
}

void AssignRootWindow(NU_WindowManager* winManager, NodeP* rootNode)
{
    SDL_Window* window = *(SDL_Window**)Vector_Get(&winManager->windows, 0);
    SDL_ShowWindow(window);
    Vector_Push(&winManager->windowNodes, &rootNode->handle);
    rootNode->node.window = window;

    float windowWidth, windowHeight;
    GetWindowSize(window, &windowWidth, &windowHeight);
    rootNode->node.width = windowWidth;
    rootNode->node.height = windowHeight;
    rootNode->node.minWidth = windowWidth;
    rootNode->node.maxWidth = windowWidth;
    rootNode->node.minHeight = windowHeight;
    rootNode->node.maxHeight = windowHeight;

    // create drawlist
    NU_WindowDrawlist* list = Vector_Create_Uninitialised(&winManager->windowDrawLists);
    Vector_Reserve(&list->relativeNodes, sizeof(NodeP*), 512);
    Vector_Reserve(&list->absoluteNodes, sizeof(NodeP*), 64);
    Vector_Reserve(&list->canvasNodes,   sizeof(NodeP*), 8);
    Vector_Reserve(&list->clippedRelativeNodes, sizeof(NodeP*), 64);
    Vector_Reserve(&list->clippedAbsoluteNodes, sizeof(NodeP*), 16);
    Vector_Reserve(&list->clippedCanvasNodes,   sizeof(NodeP*), 4);

    NU_Draw_Init();
}

void CreateSubwindow(NU_WindowManager* winManager, NodeP* node)
{
    SDL_Window* window = SDL_CreateWindow("window", 500, 400, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    node->node.window = window;
    Vector_Push(&winManager->windows, &window);
    Vector_Push(&winManager->windowNodes, &node->handle);

    // create drawlist
    NU_WindowDrawlist* list = Vector_Create_Uninitialised(&winManager->windowDrawLists);
    Vector_Reserve(&list->relativeNodes, sizeof(NodeP*), 512);
    Vector_Reserve(&list->absoluteNodes, sizeof(NodeP*), 64);
    Vector_Reserve(&list->canvasNodes,   sizeof(NodeP*), 8);
    Vector_Reserve(&list->clippedRelativeNodes, sizeof(NodeP*), 64);
    Vector_Reserve(&list->clippedAbsoluteNodes, sizeof(NodeP*), 16);
    Vector_Reserve(&list->clippedCanvasNodes,   sizeof(NodeP*), 4);
}

void GetLocalMouseCoords(NU_WindowManager* winManager, float* outX, float* outY)
{
    float globalX, globalY;
    int windowX, windowY;
    SDL_GetGlobalMouseState(&globalX, &globalY);
    SDL_GetWindowPosition(winManager->hoveredWindow, &windowX, &windowY);
    *outX = globalX - windowX;
    *outY = globalY - windowY;
}

inline int InHoverredWindow(NU_WindowManager* winManager, NodeP* node)
{
    return node->node.window == winManager->hoveredWindow;
}

inline int NodeVisibleInWindow(NU_WindowManager* winManager, NodeP* node)
{
    int windowW, windowH;
    SDL_GetWindowSize(node->node.window, &windowW, &windowH);
    float right  = node->node.x + node->node.width;
    float bottom = node->node.y + node->node.height;
    return !(right < 0 || bottom < 0 || node->node.x > windowW || node->node.y > windowH);
}

NU_WindowDrawlist* GetWindowDrawlist(NU_WindowManager* winManager, SDL_Window* window)
{
    for (uint32_t i=0; i<winManager->windows.size; i++) {
        SDL_Window* windowStored = *(SDL_Window**)Vector_Get(&winManager->windows, i);
        if (window == windowStored) {
            return (NU_WindowDrawlist*)Vector_Get(&winManager->windowDrawLists, i);
        }
    }
    return NULL;
}

inline void WindowStartNewFrame(SDL_Window* window)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    float w_fl = (float)w; float h_fl = (float)h;
    glViewport(0, 0, w, h); glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void PrepareDrawlists(NU_WindowManager* winManager)
{
    for (uint32_t i=0; i<winManager->windowDrawLists.size; i++) 
    {
        NU_WindowDrawlist* list = Vector_Get(&winManager->windowDrawLists, i);
        Vector_Clear(&list->relativeNodes);
        Vector_Clear(&list->absoluteNodes);
        Vector_Clear(&list->canvasNodes);
        Vector_Clear(&list->clippedRelativeNodes);
        Vector_Clear(&list->clippedAbsoluteNodes);
        Vector_Clear(&list->clippedCanvasNodes);
    }

    HashmapClear(&winManager->clipMap);
    Vector_Clear(&winManager->absoluteRootNodes);
}

inline void SetNodeDrawlist_Relative(NU_WindowManager* winManager, NodeP* node)
{
    NU_WindowDrawlist* list = GetWindowDrawlist(winManager, node->node.window);
    Vector_Push(&list->relativeNodes, &node);
}

inline void SetNodeDrawlist_Absolute(NU_WindowManager* winManager, NodeP* node)
{
    NU_WindowDrawlist* list = GetWindowDrawlist(winManager, node->node.window);
    Vector_Push(&list->absoluteNodes, &node);
}

inline void SetNodeDrawlist_Canvas(NU_WindowManager* winManager, NodeP* node)
{   
    NU_WindowDrawlist* list = GetWindowDrawlist(winManager, node->node.window);
    Vector_Push(&list->canvasNodes, &node);
}

inline void SetNodeDrawlist_ClippedRelative(NU_WindowManager* winManager, NodeP* node)
{
    NU_WindowDrawlist* list = GetWindowDrawlist(winManager, node->node.window);
    Vector_Push(&list->clippedRelativeNodes, &node);
}   

inline void SetNodeDrawlist_ClippedAbsolute(NU_WindowManager* winManager, NodeP* node)
{
    NU_WindowDrawlist* list = GetWindowDrawlist(winManager, node->node.window);
    Vector_Push(&list->clippedAbsoluteNodes, &node);
}

inline void SetNodeDrawlist_ClippedCanvas(NU_WindowManager* winManager, NodeP* node)
{
    NU_WindowDrawlist* list = GetWindowDrawlist(winManager, node->node.window);
    Vector_Push(&list->clippedCanvasNodes, &node);
}