#define _CRT_SECURE_NO_WARNINGS 


#include <math.h>
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include "headers/parser.h"
#include "headers/style_parser.h"
#include "headers/layout.h"
#include "headers/resources.h"
#include <nu_draw.h>

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>
#include <cairo.h>
#include <freetype/freetype.h>

int ProcessWindowEvents()
{
    int isRunning = 1; 

    SDL_Event event;
    while (SDL_PollEvent(&event)) 
    {
        // CLOSE WINDOW EVENT
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)  {   
            isRunning = 0;
        }
        else if (event.type == SDL_EVENT_QUIT)               {
            isRunning = 0;
        }
        else if (event.type == SDL_EVENT_WINDOW_MOUSE_ENTER) {
            // hovered_window_ID = event.window.windowID;
        }
        else if (event.type == SDL_EVENT_WINDOW_MOUSE_LEAVE) {
            // hovered_window_ID = 0;
        }
    }

    return isRunning;
}

int main()
{
    // Create an enpty UI tree stack
    struct UI_Tree ui_tree;

    Vector_Reserve(&ui_tree.font_resources, sizeof(struct Font_Resource), 4);
    struct Font_Resource font1;
    struct Font_Resource font2;
    Load_Font_Resource("./fonts/Inter/Inter_Variable_Weight.ttf", &font1);
    Load_Font_Resource("./fonts/Inter/Inter_Variable_Weight.ttf", &font2);
    Vector_Push(&ui_tree.font_resources, &font1);
    Vector_Push(&ui_tree.font_resources, &font2);

    timer_start();
    start_measurement();

    // Parse xml into UI tree
    if (NU_Parse("test.xml", &ui_tree) != 0)
    {
        return -1;
    }

    NU_Parse_CSS("test.css");

    end_measurement();
    timer_stop();

    // Check if SDL initialised
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }


    // Create a vector window, opengl_context and nano_vg_context pointers
    struct Vector windows;
    struct Vector gl_contexts;
    struct Vector nano_vg_contexts;
    Vector_Reserve(&windows, sizeof(SDL_Window*), 16);
    Vector_Reserve(&gl_contexts, sizeof(SDL_GLContext), 16);
    Vector_Reserve(&nano_vg_contexts, sizeof(NVGcontext*), 16);
    Vector_Reserve(&ui_tree.font_registries, sizeof(struct Vector), 16);

    struct NU_Watcher_Data watcher_data = {
        .ui_tree = &ui_tree,
        .windows = &windows,
        .gl_contexts = &gl_contexts,
        .nano_vg_contexts = &nano_vg_contexts
    };

    SDL_AddEventWatch(ResizingEventWatcher, &watcher_data);

    NU_Render(&ui_tree, &windows, &gl_contexts, &nano_vg_contexts);
    
    // Application loop
    int isRunning = 1;
    while (isRunning)
    {
        isRunning = ProcessWindowEvents();
        SDL_Delay(16);
    }

    // Free Memory
    NU_Free_UI_Tree_Memory(&ui_tree);
    Vector_Free(&windows);
    Vector_Free(&gl_contexts);
    Vector_Free(&nano_vg_contexts);

    // Close SDL
    SDL_Quit();
}

