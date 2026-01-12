#pragma once 
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <datastructures/vector.h>

typedef struct NU_WindowDrawlist
{
    Vector relativeNodes;
    Vector clippedRelativeNodes;
    Vector absoluteNodes;
    Vector clippedAbsoluteNodes;
    Vector canvasNodes;
    Vector clippedCanvasNodes;
} NU_WindowDrawlist;

// Responsible for all window related functionality
typedef struct NU_WindowManager
{   
    Vector windows;
    Vector windowDrawLists;
    SDL_Window* hoveredWindow;
} NU_WindowManager;



void CreateRootWindow(NU_WindowManager* winManager, Node* rootNode)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_SetHint("SDL_MOUSE_FOCUS_CLICKTHROUGH", "1");
    SDL_Window* window = SDL_CreateWindow("", 1000, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    SDL_GL_SetSwapInterval(0); // VSYNC ON
    glewInit();
    __NGUI.gl_ctx = context;
    Vector_Push(&winManager->windows, &window);
    NU_Draw_Init();
}

void CreateSubwindow(NU_WindowManager* winManager, Node* node)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_SetHint("SDL_MOUSE_FOCUS_CLICKTHROUGH", "1");
    SDL_Window* window = SDL_CreateWindow("window", 500, 400, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GL_MakeCurrent(window, SDL_GL_GetCurrentContext());
    node->window = window;
    Vector_Push(&winManager->windows, &window);
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

inline int InHoverredWindow(NU_WindowManager* winManager, Node* node)
{
    return node->window == winManager->hoveredWindow;
}

inline int NodeVisibleInWindow(NU_WindowManager* winManager, Node* node)
{
    int windowW, windowH;
    SDL_GetWindowSize(node->window, &windowW, &windowH);
    float right  = node->x + node->width;
    float bottom = node->y  + node->height;
    return !(right < 0 || bottom < 0 || node->x > windowW || node->y > windowH);
}

NU_WindowDrawlist* GetWindowDrawlists(NU_WindowManager* winManager, SDL_Window* window)
{
    for (int i=0; i<winManager->windows.size; i++) {
        SDL_Window* window = *(SDL_Window**) Vector_Get(&winManager->windows, i);
        if (window == window) {
            return i;
        }
    }
    return 0;
}

inline void SetNodeDrawlist_Relative(NU_WindowManager* winManager, Node* node)
{
    NU_WindowDrawlist* list = GetWindowDrawlists(winManager, node->window);
    Vector_Push(&list->relativeNodes, &node);
}

inline void SetNodeDrawlist_Absolute(NU_WindowManager* winManager, Node* node)
{
    NU_WindowDrawlist* list = GetWindowDrawlists(winManager, node->window);
    Vector_Push(&list->absoluteNodes, &node);
}

inline void SetNodeDrawlist_Canvas(NU_WindowManager* winManager, Node* node)
{   
    NU_WindowDrawlist* list = GetWindowDrawlists(winManager, node->window);
    Vector_Push(&list->canvasNodes, &node);
}

inline void SetNodeDrawlist_ClippedRelative(NU_WindowManager* winManager, Node* node)
{
    NU_WindowDrawlist* list = GetWindowDrawlists(winManager, node->window);
    Vector_Push(&list->clippedRelativeNodes, &node);
}   

inline void SetNodeDrawlist_ClippedAbsolute(NU_WindowManager* winManager, Node* node)
{
    NU_WindowDrawlist* list = GetWindowDrawlists(winManager, node->window);
    Vector_Push(&list->clippedAbsoluteNodes, &node);
}

inline void SetNodeDrawlist_ClippedCanvas(NU_WindowManager* winManager, Node* node)
{
    NU_WindowDrawlist* list = GetWindowDrawlists(winManager, node->window);
    Vector_Push(&list->clippedCanvasNodes, &node);
}



