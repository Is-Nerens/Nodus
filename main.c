#define _CRT_SECURE_NO_WARNINGS 


#include <math.h>
#include <SDL3/SDL.h>
#include <GL/glew.h>

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>
#include <freetype/freetype.h>
#include "headers/nodus.h"


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
        else if (event.type == SDL_EVENT_QUIT) {
            isRunning = 0;
        }
    }

    return isRunning;
}


void on_click(struct NU_GUI* gui, uint32_t node_handle, void *args) {
    NU_Delete_Node(gui, node_handle);
    NU_Calculate(gui);
    printf("Node Clicked! \n");
}

int main()
{
    // Check if SDL initialised
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    // Create GUI
    struct NU_GUI ngui;
    NU_GUI_Init(&ngui);
    NU_Load_Font(&ngui, "./fonts/Inter/Inter_Variable_Weight.ttf");
    if (!NU_From_XML(&ngui, "test.xml")) return -1;

    timer_start();
    start_measurement();

    struct NU_Stylesheet stylesheet;
    NU_Stylesheet_Create(&stylesheet,"test.css"); 
    NU_Stylesheet_Apply(&ngui, &stylesheet);


    end_measurement();



    // struct Node* btn_node = NU_Get_Node_By_Id(&ngui, "charts-btn");
    // if (btn_node != NULL) {
    //     NU_Register_Event(&ngui, btn_node, NULL, on_click, NU_EVENT_ON_CLICK);
    // }


    uint32_t test_delete = NU_Get_Node_By_Id(&ngui, "test-delete");
    NU_Register_Event(&ngui, test_delete, NULL, on_click, NU_EVENT_ON_CLICK);

    uint32_t test_delete2 = NU_Get_Node_By_Id(&ngui, "test-delete2");
    NU_Register_Event(&ngui, test_delete2, NULL, on_click, NU_EVENT_ON_CLICK);
 


    struct NU_Watcher_Data watcher_data = { .ngui = &ngui };
    SDL_AddEventWatch(ResizingEventWatcher, &watcher_data);


    NU_Calculate(&ngui);
    NU_Draw_Nodes(&ngui);
    
    // Application loop
    int isRunning = 1;
    while (isRunning)
    {
        isRunning = ProcessWindowEvents();

        if (ngui.awaiting_draw) {
            NU_Draw_Nodes(&ngui);
            ngui.awaiting_draw = 0;
        }
        SDL_Delay(16);
    }

    // Free Memory
    NU_Tree_Cleanup(&ngui);
    NU_Stylesheet_Free(&stylesheet);

    // Close SDL
    SDL_Quit();
}