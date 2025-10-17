#define _CRT_SECURE_NO_WARNINGS 



#include "performance.h"

#include <math.h>
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <nu_text.h>
#include "headers/nodus.h"




void on_click(uint32_t node_handle, void *args) {
    NU_Delete_Node(node_handle);
    printf("Node Clicked! \n");
}


struct Tab_Select_Data
{
    uint32_t current_tab_btn;
    uint32_t current_tab_container;
};

void charts_tab_select(uint32_t handle, void* args)
{
    struct Tab_Select_Data* data = (struct Tab_Select_Data*)args;

    NU_Set_Class(handle, "toolbar-button-selected");
    uint32_t charts_tab_container = NU_Get_Node_By_Id("charts-tab-container");
    NU_Show(charts_tab_container);

    if (data->current_tab_btn != 0 && data->current_tab_btn != handle) {
        NU_Set_Class(data->current_tab_btn, "toolbar-button");
        NU_Hide(data->current_tab_container);
    }

    // Update selected tab data struct
    data->current_tab_btn = handle;
    data->current_tab_container = charts_tab_container;
}
void editor_tab_select(uint32_t handle, void* args)
{
    struct Tab_Select_Data* data = (struct Tab_Select_Data*)args;

    NU_Set_Class(handle, "toolbar-button-selected");
    uint32_t editor_tab_container = NU_Get_Node_By_Id("editor-tab-container");
    NU_Show(editor_tab_container);

    if (data->current_tab_btn != 0 && data->current_tab_btn != handle) {
        NU_Set_Class(data->current_tab_btn, "toolbar-button");
        NU_Hide(data->current_tab_container);
    }

    // Update selected tab data struct
    data->current_tab_btn = handle;
    data->current_tab_container = editor_tab_container;
}
void perf_tab_select(uint32_t handle, void* args)
{
    struct Tab_Select_Data* data = (struct Tab_Select_Data*)args;

    NU_Set_Class(handle, "toolbar-button-selected");
    uint32_t perf_tab_container = NU_Get_Node_By_Id("perf-tab-container");
    NU_Show(perf_tab_container);

    if (data->current_tab_btn != 0 && data->current_tab_btn != handle) {
        NU_Set_Class(data->current_tab_btn, "toolbar-button");
        NU_Hide(data->current_tab_container);
    }

    // Update selected tab data struct
    data->current_tab_btn = handle;
    data->current_tab_container = perf_tab_container;
}
void cloud_tab_select(uint32_t handle, void* args)
{
    struct Tab_Select_Data* data = (struct Tab_Select_Data*)args;

    NU_Set_Class(handle, "toolbar-button-selected");
    uint32_t cloud_tab_container = NU_Get_Node_By_Id("cloud-tab-container");
    NU_Show(cloud_tab_container);

    if (data->current_tab_btn != 0 && data->current_tab_btn != handle) {
        NU_Set_Class(data->current_tab_btn, "toolbar-button");
        NU_Hide(data->current_tab_container);
    }

    // Update selected tab data struct
    data->current_tab_btn = handle;
    data->current_tab_container = cloud_tab_container;
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
    NU_Text_Renderer_Init();
    if (!NU_From_XML("test.xml")) return -1;
    struct NU_Stylesheet stylesheet;
    NU_Stylesheet_Create(&stylesheet,"test.css"); 
    NU_Stylesheet_Apply(&stylesheet);
    SDL_AddEventWatch(EventWatcher, NULL);


    NU_RGB border_col;
    border_col.r = 1.0f;
    border_col.g = 0.0f;
    border_col.b = 0.0f;

    NU_RGB fill_col;
    fill_col.r = 0.8f;
    fill_col.g = 0.6f;
    fill_col.b = 0.6f;

    uint32_t chart = NU_Get_Node_By_Id("chart");
    Border_Rect(chart, 100, 250, 10, 200, 1, &border_col, &fill_col);
    Border_Rect(chart, 120, 200, 10, 200, 1, &border_col, &fill_col);
    Border_Rect(chart, 140, 230, 10, 200, 1, &border_col, &fill_col);
    Border_Rect(chart, 160, 240, 10, 200, 1, &border_col, &fill_col);
    Line(chart, 20.5, 20.5, 300.5, 300.5, 1, &border_col);



    uint32_t charts_tab_btn = NU_Get_Node_By_Id("charts-tab-btn");
    uint32_t editor_tab_btn = NU_Get_Node_By_Id("editor-tab-btn");
    uint32_t perf_tab_btn = NU_Get_Node_By_Id("perf-tab-btn");
    uint32_t cloud_tab_btn = NU_Get_Node_By_Id("cloud-tab-btn");
    uint32_t charts_tab_container = NU_Get_Node_By_Id("charts-tab-container");
    uint32_t editor_tab_container = NU_Get_Node_By_Id("editor-tab-container");
    uint32_t perf_tab_container = NU_Get_Node_By_Id("perf-tab-container");
    uint32_t cloud_tab_container = NU_Get_Node_By_Id("cloud-tab-container");


    struct Tab_Select_Data major_tab_select;
    major_tab_select.current_tab_btn = charts_tab_btn;
    major_tab_select.current_tab_container = charts_tab_container;

    NU_Register_Event(charts_tab_btn, &major_tab_select, charts_tab_select, NU_EVENT_ON_CLICK);
    NU_Register_Event(editor_tab_btn, &major_tab_select, editor_tab_select, NU_EVENT_ON_CLICK);
    NU_Register_Event(perf_tab_btn, &major_tab_select, perf_tab_select, NU_EVENT_ON_CLICK);
    NU_Register_Event(cloud_tab_btn, &major_tab_select, cloud_tab_select, NU_EVENT_ON_CLICK);



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