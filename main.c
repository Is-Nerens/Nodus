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
    // Check if SDL initialised
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    // Create an enpty UI tree stack
    struct UI_Tree ui_tree;
    NU_Tree_Init(&ui_tree);

    struct Font_Resource font1;
    struct Font_Resource font2;
    Load_Font_Resource("./fonts/Inter/Inter_Variable_Weight.ttf", &font1);
    Load_Font_Resource("./fonts/Inter/Inter_Variable_Weight.ttf", &font2);
    Vector_Push(&ui_tree.font_resources, &font1);
    Vector_Push(&ui_tree.font_resources, &font2);


    // Parse xml into UI tree
    // timer_start();
    if (NU_Parse("test.xml", &ui_tree) != 0)
    {
        return -1;
    }   
    // timer_stop();

    timer_start();
    start_measurement();
    NU_Set_Style(&ui_tree, "test.css");

    end_measurement();

    struct NU_Watcher_Data watcher_data = {
        .ui_tree = &ui_tree
    };

    SDL_AddEventWatch(ResizingEventWatcher, &watcher_data);

    NU_Calculate(&ui_tree);
    NU_Draw_Nodes(&ui_tree);
    
    // Application loop
    int isRunning = 1;
    while (isRunning)
    {
        isRunning = ProcessWindowEvents();
        SDL_Delay(16);
    }

    // Free Memory
    NU_Tree_Cleanup(&ui_tree);

    // Close SDL
    SDL_Quit();
}

