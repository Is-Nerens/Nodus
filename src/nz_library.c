#define NODUS_BUILD_DLL
#include "nodus.h"
#include <stdio.h>

// --------------------------
// --- Nodus UI functions ---
// --------------------------
__declspec(dllexport) int NU_Create_Gui(const char* xml_filepath, const char* css_filepath) {
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
__declspec(dllexport) uint32_t NU_Load_Stylesheet(const char* filepath) {
    return NU_Internal_Load_Stylesheet(filepath);
}
__declspec(dllexport) int NU_Apply_Stylesheet(uint32_t stylesheet_handle) {
    return NU_Internal_Apply_Stylesheet(stylesheet_handle);
}

// ------------------------
// --- Window fucntions ---
// ------------------------
__declspec(dllexport) void NU_Set_Window_Fullscreen(Node* node) {
    NodeP* nodeP = NODEP_OF(node);
    SDL_Window* window = GetSDL_Window(&__NGUI.winManager, nodeP->windowID);
    Uint32 flags = SDL_GetWindowFlags(window);
    if (flags & SDL_WINDOW_FULLSCREEN) return;
    SDL_SetWindowFullscreen(window, true);

    // Clear the window, swap buffers and re-render
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    SDL_GL_SwapWindow(window);
    __NGUI.awaiting_redraw = true;
    NU_Internal_Render();
}

__declspec(dllexport) void NU_Set_Window_Windowed(Node* node) {
    NodeP* nodeP = NODEP_OF(node);
    SDL_Window* window = GetSDL_Window(&__NGUI.winManager, nodeP->windowID);
    Uint32 flags = SDL_GetWindowFlags(window);
    if (!(flags & SDL_WINDOW_FULLSCREEN)) return;
    SDL_SetWindowFullscreen(window, false);

    // Clear the window, swap buffers and re-render
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    SDL_GL_SwapWindow(window);
    __NGUI.awaiting_redraw = true;
    NU_Internal_Render();
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
    NodeP* nodeP = NODEP_OF(node);
    if (nodeP->parent == NULL) return NULL;
    return &nodeP->parent->node;
}
__declspec(dllexport) Node* NU_CHILD(Node* node, uint32_t childIndex) {
    NodeP* nodeP = NODEP_OF(node);
    if (nodeP == NULL || childIndex >= nodeP->childCount) return NULL;
    NodeP* child = nodeP->firstChild;
    u32 i = 0;
    while(child != NULL) {
        if (i == childIndex) return &child->node;
        i++;
        child = child->nextSibling;
    }
    return NULL;
}
__declspec(dllexport) int NU_CHILD_COUNT(Node* node) {
    NodeP* nodeP = NODEP_OF(node);
    return (int)nodeP->childCount;
}
__declspec(dllexport) Node* NU_CREATE_NODE(Node* parent, NodeType type) {
    if (parent == NULL || type == NU_WINDOW) return NULL; // Nodus doesn't yet support window creation
    NodeP* parentP = NODEP_OF(parent);
    NodeP* node = TreeCreateNode(&__NGUI.tree, parentP, type);
    NU_Apply_Stylesheet_To_Node(node, __NGUI.stylesheet);
    return &node->node;
}
__declspec(dllexport) void NU_DELETE_NODE(Node* node) {
    NodeP* nodeP = NODEP_OF(node);
    return TreeDeleteNode(&__NGUI.tree, nodeP, NU_DissociateNode);
}
__declspec(dllexport) void NU_SHIFT_NODE_IN_PARENT(Node* node, int index) {
    NodeP* nodeP = NODEP_OF(node);
    TreeShiftNodeInParent(&__NGUI.tree, nodeP, index);
}
__declspec(dllexport) const char* NU_INPUT_TEXT_CONTENT(Node* node) {
    NodeP* nodeP = NODEP_OF(node);
    if (nodeP->type != NU_INPUT) return NULL;
    return nodeP->typeData.input.inputText.buffer;
}
__declspec(dllexport) Node* NU_HOVERED_NODE() {
    if (!__NGUI.hovered_node) return NULL;
    return &__NGUI.hovered_node->node;
}
__declspec(dllexport) void NU_HIDE(Node* node) {
    NodeP* nodeP = NODEP_OF(node);
    nodeP->layoutFlags |= HIDDEN;
}
__declspec(dllexport) void NU_SHOW(Node* node) {
    NodeP* nodeP = NODEP_OF(node);
    nodeP->layoutFlags &= ~HIDDEN;
}
__declspec(dllexport) Node* NU_Get_Node_By_Id(const char* id) {
    void* found = StringmapGet(&__NGUI.id_node_map, id);
    if (found == NULL) return NULL;
    NodeP* node = *(NodeP**)found;
    return &node->node;
}
__declspec(dllexport) NU_Nodelist NU_Get_Nodes_By_Class(const char* class) {
    
    NU_Nodelist_Internal result;
    NU_Nodelist_Init(&result, 8);
    DepthFirstSearch dfs = DepthFirstSearch_Create(__NGUI.tree.root);
    NodeP* node;
    while(DepthFirstSearch_Next(&dfs, &node)) {
        if (node->class != NULL && strcmp(class, node->class) == 0) {
            NU_Nodelist_Push(&result, &node->node);
        }
    }
    DepthFirstSearch_Free(&dfs);
    return result.nodelist;
}
__declspec(dllexport) NU_Nodelist NU_Get_Nodes_By_Tag(NodeType type) {
    NU_Nodelist_Internal result;
    NU_Nodelist_Init(&result, 8);
    DepthFirstSearch dfs = DepthFirstSearch_Create(__NGUI.tree.root);
    NodeP* node;
    while(DepthFirstSearch_Next(&dfs, &node)) {
        if (node->type == type) {
            NU_Nodelist_Push(&result, &node->node);
        }
    }
    DepthFirstSearch_Free(&dfs);
    return result.nodelist;
}
__declspec(dllexport) NU_Nodelist NU_Get_Children(Node* node) {
    NodeP* nodeP = NODEP_OF(node);
    NU_Nodelist_Internal result;
    NU_Nodelist_Init(&result, nodeP->childCount);
    NodeP* child = nodeP->firstChild;
    while(child != NULL) {
        NU_Nodelist_Push(&result, &child->node);
        child = child->nextSibling; // move to the next child
    }
    return result.nodelist;
}
__declspec(dllexport) Node* NU_Get_First_Descendent_With_Class(Node* node, const char* class) {
    NodeP* nodeP = NODEP_OF(node);
    Node* result = NULL;
    DepthFirstSearch dfs = DepthFirstSearch_Create(nodeP);
    NodeP* dfsNode;
    while(DepthFirstSearch_Next(&dfs, &dfsNode)) {
        if (strcmp(dfsNode->class, class) == 0) {
            result = &dfsNode->node;
            break;
        }
    }
    DepthFirstSearch_Free(&dfs);
    return result;
}
__declspec(dllexport) void NU_Nodelist_Free(NU_Nodelist* nodelist) {
    free(nodelist->nodes);
    nodelist->count = 0;
}
__declspec(dllexport) void NU_Set_Class(Node* node, const char* class) {
    NU_Internal_Set_Class(node, class);
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
__declspec(dllexport) int64_t NU_Get_Canvas_Ctx(Node* canvasNode)
{
    return NU_Internal_Get_Canvas_Context(canvasNode);
}

__declspec(dllexport) void NU_Clear_Canvas(int contextID) 
{
    NU_Internal_Clear_Canvas(contextID);
}

__declspec(dllexport) void NU_Render() 
{
    NU_Internal_Render();
}

__declspec(dllexport) NU_RGB NU_RGB_From_Hex(const char* hex)
{
    NU_RGB col = {1.0f, 1.0f, 1.0f};
    if (hex == NULL) return col;
    if (hex[0] == '#') hex++;
    unsigned int r, g, b;
    if (sscanf(hex, "%02x%02x%02x", &r, &g, &b) != 3) return col;
    col.r = (float)r / 255.0f;
    col.g = (float)g / 255.0f;
    col.b = (float)b / 255.0f;
    return col;
}

__declspec(dllexport) void NU_Border_Rect(
    int contextID,
    float x, float y, float w, float h, 
    float thickness,
    NU_RGB border_col,
    NU_RGB fill_col) 
{
    NU_Internal_Border_Rect(contextID, x, y, w, h, thickness, border_col, fill_col);
}
__declspec(dllexport) void NU_Line(
    int contextID,
    float x1, float y1, float x2, float y2,
    float thickness,
    NU_RGB col) 
{
    NU_Internal_Line(contextID, x1, y1, x2, y2, thickness, col);
}

__declspec(dllexport) void NU_Dashed_Line(
    int contextID,
    float x1, float y1, float x2, float y2,
    float thickness,
    uint8_t* dash_pattern,
    uint32_t dash_pattern_len,
    NU_RGB col) 
{
    NU_Internal_Dashed_Line(
        contextID, 
        x1, y1, x2, y2, 
        thickness, 
        dash_pattern,
        dash_pattern_len,
        col);
}

__declspec(dllexport) void NU_Set_Canvas_Font(
    int contextID,
    const char* font_name)
{
    NU_Internal_Set_Canvas_Font(contextID, font_name);
}

__declspec(dllexport) void NU_Text(
    int contextID,
    float x, float y, float wrapWidth,
    NU_RGB col, const char* string)
{
    NU_Internal_Text(contextID, x, y, wrapWidth, col, string);
}

__declspec(dllexport) float NU_Text_Height(
    int contextID,
    float wrapWidth,
    const char* string)
{
    return NU_Internal_Text_Height(contextID, wrapWidth, string);
}

__declspec(dllexport) float NU_Text_Width(
    int contextID,
    const char* string)
{
    return NU_Internal_Text_Width(contextID, string);
}

__declspec(dllexport) float NU_Text_Line_Height(
    int contextID)
{
    return NU_Internal_Text_Line_Height(contextID);
}