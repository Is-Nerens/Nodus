#define _CRT_SECURE_NO_WARNINGS 



#include "performance.h"

#include <math.h>
#include <SDL3/SDL.h>
#include <GL/glew.h>

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>
#include <freetype/freetype.h>
#include "headers/nodus.h"




void on_click(uint32_t node_handle, void *args) {
    NU_Delete_Node(node_handle);
    printf("Node Clicked! \n");
}

int main()
{
    // Check if SDL initialised
    if (!SDL_Init(SDL_INIT_VIDEO)) 
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }


    // ---------------------------------------
    // --- Create GUI and apply stylesheet ---
    // ---------------------------------------
    NU_Init();
    NU_Load_Font("./fonts/Inter/Inter_Variable_Weight.ttf");
    if (!NU_From_XML("test.xml")) return -1;
    struct NU_Stylesheet stylesheet;
    NU_Stylesheet_Create(&stylesheet,"test.css"); 
    NU_Stylesheet_Apply(&stylesheet);
    SDL_AddEventWatch(EventWatcher, NULL);



    uint32_t test_delete   = NU_Get_Node_By_Id("toolbar");
    // uint32_t test_delete_1 = NU_Get_Node_By_Id("delete-1");
    // uint32_t test_delete_2 = NU_Get_Node_By_Id("delete-2");
    // uint32_t test_delete_3 = NU_Get_Node_By_Id("delete-3");
    // uint32_t test_delete_4 = NU_Get_Node_By_Id("delete-4");
    NU_Register_Event(test_delete  , NULL, on_click, NU_EVENT_ON_CLICK);
    // NU_Register_Event(test_delete_1, NULL, on_click, NU_EVENT_ON_CLICK);
    // NU_Register_Event(test_delete_2, NULL, on_click, NU_EVENT_ON_CLICK);
    // NU_Register_Event(test_delete_3, NULL, on_click, NU_EVENT_ON_CLICK);
    // NU_Register_Event(test_delete_4, NULL, on_click, NU_EVENT_ON_CLICK);


    // uint32_t create_node   = NU_Create_Node(test_delete, RECT);
    // uint32_t create_node_2 = NU_Create_Node(test_delete, RECT);
    // NODE(create_node)->text_content   = "created";
    // NODE(create_node_2)->text_content = "created2";


    NU_Reflow();
    NU_Mouse_Hover();
    NU_Draw();


    // ------------------------
    // --- Application loop ---
    // ------------------------
    while (NU_Running()) 
    {
        SDL_Event event;
        if (SDL_WaitEvent(&event)) {
            EventWatcher(NULL, &event); // you already have this function
        }
    }


    // -------------------
    // --- Free Memory ---
    // -------------------
    NU_Quit();
    SDL_Quit();
    NU_Stylesheet_Free(&stylesheet);
}