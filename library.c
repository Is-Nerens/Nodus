#define NODUS_BUILD_DLL
#include "headers/nodus.h"

// --------------------------
// --- Nodus UI functions ---
// --------------------------
__declspec(dllexport) int NU_Init(void) {
    return NU_Internal_Init();  
}
__declspec(dllexport) void NU_Quit(void) {
    NU_Internal_Quit();
}
__declspec(dllexport) int NU_Running(void) {
    return NU_Internal_Running();
}

__declspec(dllexport) void NU_Unblock(void) {
    NU_Internal_Unblock();
}

__declspec(dllexport) int NU_From_XML(char* filepath) {
    return NU_Internal_From_XML(filepath);
}

// ----------------------------
// --- Stylesheet functions ---
// ----------------------------
__declspec(dllexport) int NU_Stylesheet_Create(NU_Stylesheet* stylesheet, char* css_filepath) {
    return NU_Internal_Stylesheet_Create(stylesheet, css_filepath);
}
__declspec(dllexport) void NU_Stylesheet_Apply(NU_Stylesheet* stylesheet) {
    NU_Internal_Stylesheet_Apply(stylesheet);
}
__declspec(dllexport) void NU_Stylesheet_Free(NU_Stylesheet* stylesheet) {
    NU_Internal_Stylesheet_Free(stylesheet);
}

// ---------------------
// --- DOM functions ---
// ---------------------
__declspec(dllexport) inline struct Node* NU_NODE(uint32_t handle) {
    return NODE(handle);
}
__declspec(dllexport) uint32_t NU_Get_Node_By_Id(char* id) {
    return NU_Internal_Get_Node_By_Id(id);
}
__declspec(dllexport) NU_Nodelist NU_Get_Nodes_By_Class(char* class_name) {
    return NU_Internal_Get_Nodes_By_Class(class_name);
}
__declspec(dllexport) NU_Nodelist NU_Get_Nodes_By_Tag(enum Tag tag) {
    return NU_Internal_Get_Nodes_By_Tag(tag);
}
__declspec(dllexport) uint32_t NU_Create_Node(uint32_t parent_handle, enum Tag tag) {
    return NU_Internal_Create_Node(parent_handle, tag);
}
__declspec(dllexport) void NU_Delete_Node(uint32_t handle) {
    NU_Internal_Delete_Node(handle);
}
__declspec(dllexport) void NU_Set_Class(uint32_t handle, char* class_name) {
    NU_Internal_Set_Class(handle, class_name);
}
__declspec(dllexport) void NU_Hide(uint32_t handle) {
    NU_Internal_Hide(handle);
}
__declspec(dllexport) void NU_Show(uint32_t handle) {
    NU_Internal_Show(handle);
}

// -----------------------
// --- Event functions ---
// -----------------------
__declspec(dllexport) void NU_Register_Event(
  uint32_t node_handle, 
  void* args,
  NU_Callback callback, 
  enum NU_Event event) 
{
  NU_Internal_Register_Event(node_handle, args, callback, event);
}

// -----------------------------
// --- Canvas draw functions ---
// -----------------------------
__declspec(dllexport) void NU_Clear_Canvas(uint32_t canvas_handle) 
{
    NU_Internal_Clear_Canvas(canvas_handle);
}

__declspec(dllexport) void NU_Render() 
{
    NU_Internal_Render();
}
__declspec(dllexport) void NU_Border_Rect(
    uint32_t canvas_handle,
    float x, float y, float w, float h, 
    float thickness,
    NU_RGB* border_col,
    NU_RGB* fill_col) 
{
    NU_Internal_Border_Rect(canvas_handle, x, y, w, h, thickness, border_col, fill_col);
}
__declspec(dllexport) void NU_Line(
    uint32_t canvas_handle,
    float x1, float y1, float x2, float y2,
    float thickness,
    NU_RGB* col) 
{
    NU_Internal_Line(canvas_handle, x1, y1, x2, y2, thickness, col);
}
