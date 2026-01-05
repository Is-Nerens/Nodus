#pragma once 

#include <SDL3/SDL.h>
#include <GL/glew.h>

#include "draw/nu_draw.h"

void NU_Create_Main_Window() 
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);
    SDL_SetHint("SDL_MOUSE_FOCUS_CLICKTHROUGH", "1");
    SDL_Window* main_window = SDL_CreateWindow("Wickburner", 1000, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    // Create OpenGL context for the main window
    SDL_GLContext context = SDL_GL_CreateContext(main_window);
    SDL_GL_MakeCurrent(main_window, context);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    SDL_GL_SetSwapInterval(0); // VSYNC ON
    glewInit();

    __NGUI.gl_ctx = context;
    Vector_Push(&__NGUI.windows, &main_window);
    NU_Draw_Init();
}

void NU_Create_Subwindow(struct Node* window_node)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_SetHint("SDL_MOUSE_FOCUS_CLICKTHROUGH", "1");
    SDL_Window* new_window = SDL_CreateWindow("window", 500, 400, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GL_MakeCurrent(new_window, SDL_GL_GetCurrentContext());

    // Assign to window node
    window_node->window = new_window;

    // Push into vectors
    Vector_Push(&__NGUI.windows, &new_window);
}