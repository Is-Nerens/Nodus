#define NODUS_BUILD_DLL
#include "headers/nodus.h"
#include <stdio.h>

// --------------------------
// --- Nodus UI functions ---
// --------------------------
__declspec(dllexport) int NU_Create_Gui(char* xml_filepath, char* css_filepath) {
    return NU_Internal_Create_Gui(xml_filepath, css_filepath);
}

__declspec(dllexport) void NU_Quit(void) {
    NU_Internal_Quit();
}
__declspec(dllexport) int NU_Running(void) {
    return NU_Internal_Running();
}

// ----------------------------
// --- Stylesheet functions ---
// ----------------------------
__declspec(dllexport) uint32_t NU_Load_Stylesheet(char* filepath) {
    return NU_Internal_Load_Stylesheet(filepath);
}
__declspec(dllexport) int NU_Apply_Stylesheet(uint32_t stylesheet_handle) {
    return NU_Internal_Apply_Stylesheet(stylesheet_handle);
}

// ------------------------
// --- Cursor functions ---
// ------------------------
__declspec(dllexport) void NU_Set_Cursor_Default(void)
{
    SDL_SetCursor(__NGUI.cursorDefault);
}

__declspec(dllexport) void NU_Set_Cursor_Pointer(void)
{
    SDL_SetCursor(__NGUI.cursorPointer);
}

__declspec(dllexport) void NU_Set_Cursor_Text(void)
{
    SDL_SetCursor(__NGUI.cursorText);
}

__declspec(dllexport) void NU_Set_Cursor_Wait(void)
{
    SDL_SetCursor(__NGUI.cursorWait);
}

__declspec(dllexport) void NU_Set_Cursor_Crosshair(void)
{
    SDL_SetCursor(__NGUI.cursorCrosshair);
}

__declspec(dllexport) void NU_Set_Cursor_Move(void)
{
    SDL_SetCursor(__NGUI.cursorMove);
}

__declspec(dllexport) void NU_Set_Cursor_NsResize(void)
{
    SDL_SetCursor(__NGUI.cursorNsResize);
}

__declspec(dllexport) void NU_Set_Cursor_EwResize(void)
{
    SDL_SetCursor(__NGUI.cursorEwResize);
}

__declspec(dllexport) void NU_Set_Cursor_NwseResize(void)
{
    SDL_SetCursor(__NGUI.cursorNwseResize);
}

__declspec(dllexport) void NU_Set_Cursor_NeswResize(void)
{
    SDL_SetCursor(__NGUI.cursorNeswResize);
}

// ---------------------
// --- DOM functions ---
// ---------------------
__declspec(dllexport) Node* NU_PARENT(Node* node) {
    return PARENT(node);
}
__declspec(dllexport) Node* NU_CHILD(Node* node, uint32_t childIndex) {
    return CHILD(node, childIndex);
}
__declspec(dllexport) uint32_t NU_CHILD_COUNT(Node* node) {
    return CHILD_COUNT(node);
}
__declspec(dllexport) int NU_DEPTH(Node* node) {
    return DEPTH(node);
}
__declspec(dllexport) Node* NU_CREATE_NODE(Node* parent, NodeType type) {
    return CREATE_NODE(parent, type);
}
__declspec(dllexport) void NU_DELETE_NODE(Node* node) {
    DELETE_NODE(node);
}
__declspec(dllexport) const char* NU_INPUT_TEXT_CONTENT(Node* node) {
    return INPUT_TEXT_CONTENT(node);
}
__declspec(dllexport) void NU_HIDE(Node* node) {
    HIDE(node);
}
__declspec(dllexport) void NU_SHOW(Node* node) {
    SHOW(node);
}
__declspec(dllexport) Node* NU_Get_Node_By_Id(char* id) {
    return NU_Internal_Get_Node_By_Id(id);
}
__declspec(dllexport) NU_Nodelist NU_Get_Nodes_By_Class(char* class_name) {
    return NU_Internal_Get_Nodes_By_Class(class_name);
}
__declspec(dllexport) NU_Nodelist NU_Get_Nodes_By_Tag(NodeType type) {
    return NU_Internal_Get_Nodes_By_Tag(type);
}
__declspec(dllexport) void NU_Set_Class(Node* node, char* class_name) {
    NU_Internal_Set_Class(node, class_name);
}
// -----------------------
// --- Event functions ---
// -----------------------
__declspec(dllexport) void NU_Register_Event(
  Node* node, 
  void* args,
  NU_Callback callback, 
  enum NU_Event_Type event_type) 
{
  NU_Internal_Register_Event(node, args, callback, event_type);
}

// -----------------------------
// --- Canvas API functions ---
// -----------------------------
__declspec(dllexport) void NU_Clear_Canvas(Node* canvas) 
{
    NU_Internal_Clear_Canvas(canvas);
}

__declspec(dllexport) void NU_Render() 
{
    NU_Internal_Render();
}
__declspec(dllexport) void NU_Border_Rect(
    Node* canvas,
    float x, float y, float w, float h, 
    float thickness,
    NU_RGB* border_col,
    NU_RGB* fill_col) 
{
    NU_Internal_Border_Rect(canvas, x, y, w, h, thickness, border_col, fill_col);
}
__declspec(dllexport) void NU_Line(
    Node* canvas,
    float x1, float y1, float x2, float y2,
    float thickness,
    NU_RGB* col) 
{
    NU_Internal_Line(canvas, x1, y1, x2, y2, thickness, col);
}

__declspec(dllexport) void NU_Dashed_Line(
    Node* canvas,
    float x1, float y1, float x2, float y2,
    float thickness,
    uint8_t* dash_pattern,
    uint32_t dash_pattern_len,
    NU_RGB* col) 
{
    NU_Internal_Dashed_Line(
        canvas, 
        x1, y1, x2, y2, 
        thickness, 
        dash_pattern,
        dash_pattern_len,
        col);
}

__declspec(dllexport) void NU_Set_Canvas_Font(
    Node* canvas,
    const char* font_name)
{
    NU_Internal_Set_Canvas_Font(canvas, font_name);
}

__declspec(dllexport) void NU_Text(
    Node* canvas,
    float x, float y, float wrapWidth,
    NU_RGB col, const char* string)
{
    NU_Internal_Text(canvas, x, y, wrapWidth, col, string);
}

__declspec(dllexport) float NU_Text_Height(
    Node* canvas,
    float wrapWidth,
    const char* string)
{
    return NU_Internal_Text_Height(canvas, wrapWidth, string);
}

__declspec(dllexport) float NU_Text_Width(
    Node* canvas,
    const char* string)
{
    return NU_Internal_Text_Width(canvas, string);
}

__declspec(dllexport) float NU_Text_Line_Height(
    Node* canvas)
{
    return NU_Internal_Text_Line_Height(canvas);
}