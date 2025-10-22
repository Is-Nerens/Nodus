
#include "performance.h"
#include "headers/nodus.h"




void on_click(uint32_t node_handle, void *args) {
    NU_Delete_Node(node_handle);
    printf("Node Clicked! \n");
}


struct Tab_Manager {
    uint32_t current_tab_btn;
    uint32_t current_tab_container;
};

struct Tab_Select_Data {
    struct Tab_Manager* manager; // shared state
    uint32_t tab_btn;
    uint32_t tab_container;
};



void tab_select(uint32_t handle, void* args)
{
    struct Tab_Select_Data* data = (struct Tab_Select_Data*)args;
    struct Tab_Manager* mgr = data->manager;

    // Deactivate previous tab
    NU_Set_Class(mgr->current_tab_btn, "tab-button");
    NU_Hide(mgr->current_tab_container);

    // Activate the clicked one
    NU_Set_Class(data->tab_btn, "tab-button-selected");
    NU_Show(data->tab_container);

    // Update manager state
    mgr->current_tab_btn = data->tab_btn;
    mgr->current_tab_container = data->tab_container;
}


int main()
{
    // ---------------------------------------
    // --- Create GUI and apply stylesheet ---
    // ---------------------------------------
    if (!NU_Init()) return -1;
    if (!NU_From_XML("test.xml")) return -1;
    struct NU_Stylesheet stylesheet;
    NU_Stylesheet_Create(&stylesheet,"test.css"); 
    NU_Stylesheet_Apply(&stylesheet);


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


    // --- Main Tab Selection ---
    uint32_t charts_tab_btn = NU_Get_Node_By_Id("charts-tab-btn");
    uint32_t editor_tab_btn = NU_Get_Node_By_Id("editor-tab-btn");
    uint32_t perf_tab_btn = NU_Get_Node_By_Id("perf-tab-btn");
    uint32_t cloud_tab_btn = NU_Get_Node_By_Id("cloud-tab-btn");
    uint32_t charts_tab_container = NU_Get_Node_By_Id("charts-tab-container");
    uint32_t editor_tab_container = NU_Get_Node_By_Id("editor-tab-container");


    uint32_t perf_tab_container = NU_Get_Node_By_Id("perf-tab-container");
    uint32_t cloud_tab_container = NU_Get_Node_By_Id("cloud-tab-container");
    struct Tab_Manager main_tabs = { .current_tab_btn = charts_tab_btn, .current_tab_container = charts_tab_container };
    struct Tab_Select_Data charts_tab = { &main_tabs, charts_tab_btn, charts_tab_container };
    struct Tab_Select_Data editor_tab = { &main_tabs, editor_tab_btn, editor_tab_container };
    struct Tab_Select_Data perf_tab   = { &main_tabs, perf_tab_btn,  perf_tab_container };
    struct Tab_Select_Data cloud_tab  = { &main_tabs, cloud_tab_btn, cloud_tab_container };
    NU_Register_Event(charts_tab_btn, &charts_tab, tab_select, NU_EVENT_ON_CLICK);
    NU_Register_Event(editor_tab_btn, &editor_tab, tab_select, NU_EVENT_ON_CLICK);
    NU_Register_Event(perf_tab_btn,   &perf_tab,   tab_select, NU_EVENT_ON_CLICK);
    NU_Register_Event(cloud_tab_btn,  &cloud_tab,  tab_select, NU_EVENT_ON_CLICK);

    // --- Trade Table Selection ---
    uint32_t positions_table_selector = NU_Get_Node_By_Id("positions-table-selector");
    uint32_t orders_table_selector = NU_Get_Node_By_Id("orders-table-selector");
    uint32_t trades_table_selector = NU_Get_Node_By_Id("trades-table-selector");
    uint32_t positions_table = NU_Get_Node_By_Id("positions-table");
    uint32_t orders_table = NU_Get_Node_By_Id("orders-table");
    uint32_t trades_table = NU_Get_Node_By_Id("trades-table");
    struct Tab_Manager trade_table_tabs = {.current_tab_btn = positions_table_selector, .current_tab_container = positions_table };
    struct Tab_Select_Data positions_table_tab = { &trade_table_tabs, positions_table_selector, positions_table };
    struct Tab_Select_Data orders_table_tab = { &trade_table_tabs, orders_table_selector, orders_table };
    struct Tab_Select_Data trades_table_tab = { &trade_table_tabs, trades_table_selector, trades_table };
    NU_Register_Event(positions_table_selector, &positions_table_tab, tab_select, NU_EVENT_ON_CLICK);
    NU_Register_Event(orders_table_selector, &orders_table_tab, tab_select, NU_EVENT_ON_CLICK);
    NU_Register_Event(trades_table_selector, &trades_table_tab, tab_select, NU_EVENT_ON_CLICK);

    // ------------------------
    // --- Application loop ---
    // ------------------------
    NU_Mainloop();

    // -------------------
    // --- Free Memory ---
    // -------------------
    NU_Quit();
    NU_Stylesheet_Free(&stylesheet);
}