
#include "utils/nu_int.h"
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <ft2build.h>
#include <stdio.h>
#include <nu_gui.h>
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H

// Export macro
#if defined(IPC_STATIC)
    #define _EXPORT
#elif defined(_WIN64)
    #define _EXPORT __declspec(dllexport)
#elif defined(__APPLE__) || defined(__linux__)
    #define _EXPORT __attribute__((visibility("default")))
#else
    #define _EXPORT
#endif

// --------------------------
// --- Nodus UI functions ---
// --------------------------
_EXPORT int NU_CREATE_GUI(const char* xml_filepath, const char* css_filepath) {
    return NU_Internal_Create_Gui(xml_filepath, css_filepath);
}

_EXPORT void NU_QUIT(void) {
    NU_Internal_Quit();
}

_EXPORT void NU_RUN(void) {
    if (GUI.running) return;
    GUI.running = true;

    while(GUI.running) {

        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {}

        // Trigger timer callbacks
        u64 currentTimeMs = SDL_GetTicks();
        for (int i=0; i<GUI.timers.size; i++) {
            NU_Timer* timer = Container_GetAt(&GUI.timers, i);
            if (!timer->willBeRemoved) {
                u64 timeSinceLastFired = currentTimeMs - timer->lastFiredTime;
                if (timeSinceLastFired >= timer->intervalMs) {
                    timer->lastFiredTime = currentTimeMs;
                    timer->callback(timer->args);
                }
            }
        }

        // Safely delete removed timers
        for (int i=GUI.timers.size-1; i>=0; i--) {
            NU_Timer* timer = Container_GetAt(&GUI.timers, i);
            int id = Container_IdAt(&GUI.timers, i);
            if (timer->willBeRemoved) Container_Remove(&GUI.timers, id);
        }

        // Safely unregister deleted nodes from iterated hashmaps and free node memory
        for (int i=0; i<GUI.tree.deletedButNotFreedNodes.size; i++) {
            Node* node = *(Node**)Array_Get(&GUI.tree.deletedButNotFreedNodes, i);
            NU_Unregister_All_Iterated_Events(node);
        }
        TreeFreeDeleted(&GUI.tree);

        // Re-render UI if necessary
        if (GUI.awaiting_redraw)
        {
            NU_Layout();
            NU_Mouse_Hover();
            NU_Draw();
            CheckForResizeEvents();
        }

        // Wait for next event, with timeout to save CPU
        SDL_WaitEventTimeout(&event, GetFrametime());
    }
}

_EXPORT int NU_ADD_TIMER(u32 intervalMs, NU_Callback callback, void* args)
{
    NU_Timer timer;
    timer.intervalMs = intervalMs;
    timer.lastFiredTime = 0;
    timer.callback = callback;
    timer.args = args;
    timer.willBeRemoved = false;
    return Container_Add(&GUI.timers, &timer);
}

_EXPORT void NU_REMOVE_TIMER(int timer)
{
    NU_Timer* _timer = Container_Get(&GUI.timers, timer);
    if (_timer) {
        _timer->willBeRemoved = true;
    }
}

// -----------------------
// --- Error functions ---
// -----------------------
_EXPORT void NU_CLEAR_ERRORS() {
    ErrorSystem_Clear(&GUI.errorSystem);
}

_EXPORT const char* NU_GET_NEXT_ERROR() {
    return ErrorSystem_GetNextError(&GUI.errorSystem);
}

_EXPORT NU_RGB NU_HEX_TO_RGB(const char* hex)
{
    NU_RGB col = {1.0f, 1.0f, 1.0f};
    if (hex == NULL) return col;
    if (hex[0] == '#') hex++;
    unsigned int r, g, b;
    if (sscanf(hex, "%02x%02x%02x", &r, &g, &b) != 3) return col;
    col.r = (u8)r;
    col.g = (u8)g;
    col.b = (u8)b;
    return col;
}

_EXPORT NU_RGB_FL NU_HEX_TO_RGB_FL(const char* hex)
{
    NU_RGB_FL col = {1.0f, 1.0f, 1.0f};
    if (hex == NULL) return col;
    if (hex[0] == '#') hex++;
    unsigned int r, g, b;
    if (sscanf(hex, "%02x%02x%02x", &r, &g, &b) != 3) return col;
    col.r = (float)r / 255.0f;
    col.g = (float)g / 255.0f;
    col.b = (float)b / 255.0f;
    return col;
}

// ------------------------------
// --- Screen Query Functions ---
// ------------------------------
_EXPORT void NU_GET_SCREEN_DIMENSIONS(const NuNode* node, int* screenWidthOut, int* screenHeightOut)
{
    Node* nodeP = (Node*)(node);
    NU_Window* win = GetNodeWindow(&GUI.winManager, nodeP);
    *screenWidthOut = win->screenWidth;
    *screenHeightOut = win->screenHeight;
}


// ------------------------
// --- Cursor functions ---
// ------------------------
_EXPORT void NU_SET_CURSOR_DEFAULT(void)
{
    SDL_SetCursor(GUI.cursorDefault);
}

_EXPORT void NU_SET_CURSOR_POINTER(void)
{
    SDL_SetCursor(GUI.cursorPointer);
}

_EXPORT void NU_SET_CURSOR_TEXT(void)
{
    SDL_SetCursor(GUI.cursorText);
}

_EXPORT void NU_SET_CURSOR_WAIT(void)
{
    SDL_SetCursor(GUI.cursorWait);
}

_EXPORT void NU_SET_CURSOR_CROSSHAIR(void)
{
    SDL_SetCursor(GUI.cursorCrosshair);
}

_EXPORT void NU_SET_CURSOR_MOVE(void)
{
    SDL_SetCursor(GUI.cursorMove);
}

_EXPORT void NU_SET_CURSOR_NS_RESIZE(void)
{
    SDL_SetCursor(GUI.cursorNsResize);
}

_EXPORT void NU_SET_CURSOR_EW_RESIZE(void)
{
    SDL_SetCursor(GUI.cursorEwResize);
}

_EXPORT void NU_SET_CURSOR_NWSE_RESIZE(void)
{
    SDL_SetCursor(GUI.cursorNwseResize);
}

_EXPORT void NU_SET_CURSOR_NESW_RESIZE(void)
{
    SDL_SetCursor(GUI.cursorNeswResize);
}

// ---------------------
// --- DOM functions ---
// ---------------------
_EXPORT NuNode* NU_PARENT(const NuNode* node) {
    Node* nodeP = (Node*)(node);
    if (nodeP->parent == NULL) return NULL;
    return (NuNode*)(nodeP->parent);
}

_EXPORT NuNode* NU_CHILD(const NuNode* node, u32 childIndex) {
    Node* nodeP = (Node*)(node);
    if (nodeP == NULL || childIndex >= nodeP->childCount) return NULL;
    Node* child = nodeP->firstChild;
    u32 i = 0;
    while(child != NULL) {
        if (i == childIndex) return (NuNode*)(child);
        i++;
        child = child->nextSibling;
    }
    return NULL;
}

_EXPORT u8 NU_TYPE(const NuNode* node) {
    Node* nodeP = (Node*)(node);
    return nodeP->type;
}

_EXPORT int NU_CHILD_COUNT(const NuNode* node) {
    Node* nodeP = (Node*)(node);
    return (int)nodeP->childCount;
}

_EXPORT NuNode* NU_NEXT_SIBLING(const NuNode* node) {
    Node* nodeP = (Node*)(node);
    return (NuNode*)(nodeP->nextSibling);
}

_EXPORT NuNode* NU_PREVIOUS_SIBLING(const NuNode* node) {
    Node* nodeP = (Node*)(node);
    return (NuNode*)(nodeP->prevSibling);
}

_EXPORT NuWindow* NU_WINDOW(NuNode* parent) {
    if (parent == NULL) return NULL;
    Node* parentP = (Node*)(parent);
    Node* node = TreeCreateNode(&GUI.tree, parentP, NU_TYPE_WINDOW);
    CreateSubwindow(&GUI.winManager, node);
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, node);
    return (NuWindow*)(node);
}

_EXPORT NuBox* NU_BOX(NuNode* parent) {
    if (parent == NULL) return NULL;
    Node* parentP = (Node*)(parent);
    Node* node = TreeCreateNode(&GUI.tree, parentP, NU_TYPE_BOX);
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, node);
    return (NuBox*)(node);
}

_EXPORT NuFrame* NU_FRAME(NuNode* parent) {
    if (parent == NULL) return NULL;
    Node* parentP = (Node*)(parent);
    Node* node = TreeCreateNode(&GUI.tree, parentP, NU_TYPE_FRAME);
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, node);
    return (NuFrame*)(node);
}

_EXPORT NuButton* NU_BUTTON(NuNode* parent) {
    if (parent == NULL) return NULL;
    Node* parentP = (Node*)(parent);
    Node* node = TreeCreateNode(&GUI.tree, parentP, NU_TYPE_BUTTON);
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, node);
    return (NuButton*)(node);
}

_EXPORT NuInput* NU_INPUT(NuNode* parent) {
    if (parent == NULL) return NULL;
    Node* parentP = (Node*)(parent);
    Node* node = TreeCreateNode(&GUI.tree, parentP, NU_TYPE_INPUT);
    InputText inputText;
    InputText_Init(&inputText);
    node->special.input.textInputHandle = Container_Add(&GUI.textInputs, &inputText);
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, node);
    return (NuInput*)(node);
}

_EXPORT NuCanvas* NU_CANVAS(NuNode* parent) {
    if (parent == NULL) return NULL;
    Node* parentP = (Node*)(parent);
    Node* node = TreeCreateNode(&GUI.tree, parentP, NU_TYPE_CANVAS);
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, node);
    return (NuCanvas*)(node);
}

_EXPORT NuImage* NU_IMAGE(NuNode* parent) {
    if (parent == NULL) return NULL;
    Node* parentP = (Node*)(parent);
    Node* node = TreeCreateNode(&GUI.tree, parentP, NU_TYPE_IMAGE);
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, node);
    return (NuImage*)(node);
}

_EXPORT NuTable* NU_TABLE(NuNode* parent) {
    if (parent == NULL) return NULL;
    Node* parentP = (Node*)(parent);
    Node* node = TreeCreateNode(&GUI.tree, parentP, NU_TYPE_TABLE);
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, node);
    return (NuTable*)(node);
}

_EXPORT NuThead* NU_THEAD(NuNode* parent) {
    if (parent == NULL) return NULL;
    Node* parentP = (Node*)(parent);
    Node* node = TreeCreateNode(&GUI.tree, parentP, NU_TYPE_THEAD);
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, node);
    return (NuThead*)(node);
}

_EXPORT NuRow* NU_ROW(NuNode* parent) {
    if (parent == NULL) return NULL;
    Node* parentP = (Node*)(parent);
    Node* node = TreeCreateNode(&GUI.tree, parentP, NU_TYPE_ROW);
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, node);
    return (NuRow*)(node);
}

_EXPORT void NU_DELETE_NODE(NuNode* node) {
    Node* nodeP = (Node*)(node);
    return TreeDeleteNode(&GUI.tree, nodeP, NU_DissociateNode);
}

_EXPORT const NuWindowAccess* NU_WINDOW_GET(NuWindow* window) {
    Node* nodeP = (Node*)window;
    return &nodeP->public.window;
}

_EXPORT const NuBoxAccess* NU_BOX_GET(NuBox* box) {
    Node* nodeP = (Node*)box;
    return &nodeP->public.box;
}

_EXPORT const NuFrameAccess* NU_FRAME_GET(NuFrame* frame) {
    Node* nodeP = (Node*)frame;
    return &nodeP->public.frame;
}

_EXPORT const NuButtonAccess* NU_BUTTON_GET(NuButton* button) {
    Node* nodeP = (Node*)button;
    return &nodeP->public.button;
}

_EXPORT const NuInputAccess* NU_INPUT_GET(NuInput* input) {
    Node* nodeP = (Node*)input;
    return &nodeP->public.input;
}

_EXPORT const NuCanvasAccess* NU_CANVAS_GET(NuCanvas* canvas) {
    Node* nodeP = (Node*)canvas;
    return &nodeP->public.canvas;
}

_EXPORT const NuImageAccess* NU_IMAGE_GET(NuImage* image) {
    Node* nodeP = (Node*)image;
    return &nodeP->public.image;
}

_EXPORT const NuTableAccess* NU_TABLE_GET(NuTable* table) {
    Node* nodeP = (Node*)table;
    return &nodeP->public.table;
}

_EXPORT const NuTheadAccess* NU_THEAD_GET(NuThead* thead) {
    Node* nodeP = (Node*)thead;
    return &nodeP->public.thead;
}

_EXPORT const NuRowAccess* NU_ROW_GET(NuRow* row) {
    Node* nodeP = (Node*)row;
    return &nodeP->public.row;
}

_EXPORT void NU_SET_PREFERRED_WIDTH(NuNode* node, uint16_t preferredWidth) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.preferredWidth = preferredWidth;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_PREFERRED_HEIGHT(NuNode* node, uint16_t preferredHeight) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.preferredHeight = preferredHeight;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_MIN_WIDTH(NuNode* node, uint16_t minWidth) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.minWidth = minWidth;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_MAX_WIDTH(NuNode* node, uint16_t maxWidth) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.maxWidth = maxWidth;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_MIN_HEIGHT(NuNode* node, uint16_t minHeight) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.minHeight = minHeight;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_MAX_HEIGHT(NuNode* node, uint16_t maxHeight) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.maxHeight = maxHeight;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_LEFT(NuNode* node, uint16_t left) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.left = left;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_RIGHT(NuNode* node, uint16_t right) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.right = right;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_TOP(NuNode* node, uint16_t top) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.top = top;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_BOTTOM(NuNode* node, uint16_t bottom) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.bottom = bottom;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_GAP(NuNode* node, uint8_t gap) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.gap = gap;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_PAD_TOP(NuNode* node, uint8_t padTop) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.padTop = padTop;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_PAD_BOTTOM(NuNode* node, uint8_t padBottom) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.padBottom = padBottom;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_PAD_LEFT(NuNode* node, uint8_t padLeft) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.padLeft = padLeft;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_PAD_RIGHT(NuNode* node, uint8_t padRight) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.padRight = padRight;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_BORDER_TOP(NuNode* node, uint8_t borderTop) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.borderTop = borderTop;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_BORDER_BOTTOM(NuNode* node, uint8_t borderBottom) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.borderBottom = borderBottom;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_BORDER_LEFT(NuNode* node, uint8_t borderLeft) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.borderLeft = borderLeft;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_BORDER_RIGHT(NuNode* node, uint8_t borderRight) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.borderRight = borderRight;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_BORDER_RADIUS_TL(NuNode* node, uint8_t borderRadiusTl) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.borderRadiusTl = borderRadiusTl;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_BORDER_RADIUS_TR(NuNode* node, uint8_t borderRadiusTr) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.borderRadiusTr = borderRadiusTr;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_BORDER_RADIUS_BL(NuNode* node, uint8_t borderRadiusBl) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.borderRadiusBl = borderRadiusBl;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_BORDER_RADIUS_BR(NuNode* node, uint8_t borderRadiusBr) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.borderRadiusBr = borderRadiusBr;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_BACKGROUND_RGB(NuNode* node, u8 r, u8 g, u8 b) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.backgroundR = r;
    nodeP->public.common.backgroundG = g;
    nodeP->public.common.backgroundB = b;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_BORDER_RGB(NuNode* node, u8 r, u8 g, u8 b) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.borderR = r;
    nodeP->public.common.borderG = g;
    nodeP->public.common.borderB = b;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_TEXT_RGB(NuNode* node, u8 r, u8 g, u8 b) {
    Node* nodeP = (Node*)node;
    nodeP->public.common.textR = r;
    nodeP->public.common.textG = g;
    nodeP->public.common.textB = b;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_BACKGROUND_HEX(NuNode* node, const char* hex) {
    NU_RGB rgb = NU_HEX_TO_RGB(hex);
    Node* nodeP = (Node*)node;
    nodeP->public.common.backgroundR = rgb.r / 255;
    nodeP->public.common.backgroundG = rgb.g / 255;
    nodeP->public.common.backgroundB = rgb.b / 255;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_BORDER_HEX(NuNode* node, const char* hex) {
    NU_RGB rgb = NU_HEX_TO_RGB(hex);
    Node* nodeP = (Node*)node;
    nodeP->public.common.borderR = rgb.r / 255;
    nodeP->public.common.borderG = rgb.g / 255;
    nodeP->public.common.borderB = rgb.b / 255;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SET_TEXT_HEX(NuNode* node, const char* hex) {
    NU_RGB rgb = NU_HEX_TO_RGB(hex);
    Node* nodeP = (Node*)node;
    nodeP->public.common.textR = rgb.r;
    nodeP->public.common.textG = rgb.g;
    nodeP->public.common.textB = rgb.b;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_WINDOW_SET_TEXT_CONTENT(NuWindow* window, char* string)
{
    Node* nodeP = (Node*)window;
    nodeP->public.window.textContent = string;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_WINDOW_SET_TITLE(NuWindow* window, const char* string) {
    Node* nodeP = (Node*)(window);
    SDL_Window* SDLwindow = GetSDL_Window(&GUI.winManager, nodeP->windowID);
    SDL_SetWindowTitle(SDLwindow, string);
}

_EXPORT void NU_WINDOW_SET_FULLSCREEN(NuWindow* window) {
    Node* nodeP = (Node*)(window);
    SDL_Window* SDLwindow = GetSDL_Window(&GUI.winManager, nodeP->windowID);
    Uint32 flags = SDL_GetWindowFlags(SDLwindow);
    if (flags & SDL_WINDOW_FULLSCREEN) return;
    SDL_SetWindowFullscreen(SDLwindow, true);

    // Clear the window, swap buffers and re-render
    int w, h;
    SDL_GetWindowSize(SDLwindow, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    SDL_GL_SwapWindow(SDLwindow);
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_WINDOW_SET_WINDOWED(NuWindow* window) {
    Node* nodeP = (Node*)(window);
    SDL_Window* SDLwindow = GetSDL_Window(&GUI.winManager, nodeP->windowID);
    Uint32 flags = SDL_GetWindowFlags(SDLwindow);
    if (!(flags & SDL_WINDOW_FULLSCREEN)) return;
    SDL_SetWindowFullscreen(SDLwindow, false);

    // Clear the window, swap buffers and re-render
    int w, h;
    SDL_GetWindowSize(SDLwindow, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    SDL_GL_SwapWindow(SDLwindow);
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_BOX_SET_TEXT_CONTENT(NuBox* box, char* string)
{
    Node* nodeP = (Node*)box;
    nodeP->public.box.textContent = string;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_BUTTON_SET_TEXT_CONTENT(NuButton* button, char* string)
{
    Node* nodeP = (Node*)button;
    nodeP->public.button.textContent = string;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_INPUT_SET_TEXT_CONTENT(NuNode* node, const char* string) {
    Node* nodeP = (Node*)(node);
    NU_Font* font = Stylesheet_Get_Font(&GUI.stylesheet, nodeP->fontIdx);
    InputText* inputText = Container_Get(&GUI.textInputs, nodeP->special.input.textInputHandle);
    InputText_SetText(inputText, nodeP, font, string);
    TriggerOnInputChangedEvent(nodeP, "");
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_INPUT_FOCUS(NuInput* input) {
    Node* nodeP = (Node*)(input);

    Node* prevFocusedNode = GUI.focused_node;
    GUI.focused_node = nodeP;


    if (GUI.focused_node != prevFocusedNode)
    {
        // Defocus prev focused input node
        if (prevFocusedNode != NULL)
        {
            InputText* inputText = Container_Get(&GUI.textInputs, prevFocusedNode->special.input.textInputHandle);
            InputText_Defocus(inputText);

            // Trigger defocus event
            TriggerOnInputDefocusEvent(prevFocusedNode);

            // Remove focus style prev focused node
            Stylesheet_ApplyStyleToNode(&GUI.stylesheet, prevFocusedNode);
            if (prevFocusedNode == GUI.hovered_node) {
                Stylesheet_ApplyPseudoStyleToNode(&GUI.stylesheet, GUI.focused_node, PSEUDO_HOVER);
            }
            else if (prevFocusedNode == GUI.mouse_down_node) {
                Stylesheet_ApplyPseudoStyleToNode(&GUI.stylesheet, GUI.focused_node, PSEUDO_PRESS);
            }
        }

        // Focus on input node
        Stylesheet_ApplyPseudoStyleToNode(&GUI.stylesheet, GUI.focused_node, PSEUDO_FOCUS);
        NU_Font* font = Stylesheet_Get_Font(&GUI.stylesheet, GUI.focused_node->fontIdx);
        InputText* inputText = Container_Get(&GUI.textInputs, GUI.focused_node->special.input.textInputHandle);
        InputText_MousePlaceCursor(inputText, GUI.focused_node, font, 1000000.0f);
        SDL_StartTextInput(GetSDL_Window(&GUI.winManager, GUI.focused_node->windowID));

        // Defer offsets calculation
        inputText->updateOffsetsPostLayout = true;

        // Trigger focus event
        TriggerOnInputFocusEvent(nodeP);
        GUI.awaiting_redraw = true;
    }
}

_EXPORT const char* NU_INPUT_GET_TEXT_CONTENT(const NuInput* input) {
    Node* nodeP = (Node*)(input);
    InputText* inputText = Container_Get(&GUI.textInputs, nodeP->special.input.textInputHandle);
    return inputText->buffer;
}

_EXPORT void NU_SHIFT_NODE_IN_PARENT(NuNode* node, int index) {
    Node* nodeP = (Node*)(node);
    TreeShiftNodeInParent(&GUI.tree, nodeP, index);
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_REPARENT_NODE(NuNode* node, Node* newParent) {
    Node* nodeP = (Node*)(node);
    Node* newParentP = (Node*)(newParent);
    TreeReparentNode(&GUI.tree, nodeP, newParentP);
    GUI.awaiting_redraw = true;
}

_EXPORT NuNode* NU_HOVERED_NODE() {
    return (NuNode*)(GUI.hovered_node);
}

_EXPORT void NU_HIDE(NuNode* node) {
    Node* nodeP = (Node*)(node);
    nodeP->layoutFlags |= HIDDEN;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_SHOW(NuNode* node) {
    Node* nodeP = (Node*)(node);
    nodeP->layoutFlags &= ~HIDDEN;
    GUI.awaiting_redraw = true;
}

_EXPORT int NU_IS_SHOWN(const NuNode* node) {
    Node* nodeP = (Node*)(node);
    return !(nodeP->layoutFlags & HIDDEN);
}

_EXPORT void NU_GROW_VERTICAL(NuNode* node) {
    Node* nodeP = (Node*)(node);
    nodeP->layoutFlags |= GROW_VERTICAL;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_GROW_HORIZONTAL(NuNode* node) {
    Node* nodeP = (Node*)(node);
    nodeP->layoutFlags |= GROW_HORIZONTAL;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_GROW_BOTH(NuNode* node) {
    Node* nodeP = (Node*)(node);
    nodeP->layoutFlags |= GROW_VERTICAL;
    nodeP->layoutFlags |= GROW_HORIZONTAL;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_GROW_NONE(NuNode* node) {
    Node* nodeP = (Node*)(node);
    nodeP->layoutFlags &= ~GROW_VERTICAL;
    nodeP->layoutFlags &= ~GROW_HORIZONTAL;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_GROW_IGNORE_STYLESHEET(NuNode* node) {
    Node* nodeP = (Node*)(node);
    nodeP->overrideStyleFlags |= PROPERTY_FLAG_GROW;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_GROW_ENABLE_STYLESHEET(NuNode* node) {
    Node* nodeP = (Node*)(node);
    nodeP->overrideStyleFlags &= ~PROPERTY_FLAG_GROW;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_BACKGROUND_VISIBILITY_SHOWN(NuNode* node) {
    Node* nodeP = (Node*)(node);
    nodeP->layoutFlags &= ~HIDE_BACKGROUND;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_BACKGROUND_VISIBILITY_HIDDEN(NuNode* node) {
    Node* nodeP = (Node*)(node);
    nodeP->layoutFlags |= HIDE_BACKGROUND;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_BACKGROUND_VISIBILITY_IGNORE_STYLESHEET(NuNode* node) {
    Node* nodeP = (Node*)(node);
    nodeP->overrideStyleFlags |= PROPERTY_FLAG_HIDE_BACKGROUND;
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_BACKGROUND_VISIBILITY_ENABLE_STYLESHEET(NuNode* node) {
    Node* nodeP = (Node*)(node);
    nodeP->overrideStyleFlags &= ~PROPERTY_FLAG_HIDE_BACKGROUND;
    GUI.awaiting_redraw = true;
}

_EXPORT NuNode* NU_NODE_WITH_ID(const char* id) {
    void* found = Stringmap_Get(&GUI.id_node_map, id);
    if (found == NULL) return NULL;
    Node* node = *(Node**)found;
    return (NuNode*)(node);
}

_EXPORT NU_Nodelist NU_NODES_WITH_CLASS(const char* class) {

    NU_Nodelist_Internal result;
    NU_Nodelist_Init(&result, 8);
    DepthFirstSearch dfs = DepthFirstSearch_Create(GUI.tree.root);
    Node* node;
    while(DepthFirstSearch_Next(&dfs, &node)) {
        if (node->class != NULL && strcmp(class, node->class) == 0) {
            NU_Nodelist_Push(&result, (NuNode*)(node));
        }
    }
    DepthFirstSearch_Free(&dfs);
    return result.nodelist;
}

_EXPORT NU_Nodelist NU_NODES_WITH_TYPE(const u8 type) {
    NU_Nodelist_Internal result;
    NU_Nodelist_Init(&result, 8);
    DepthFirstSearch dfs = DepthFirstSearch_Create(GUI.tree.root);
    Node* node;
    while(DepthFirstSearch_Next(&dfs, &node)) {
        if (node->type == type) {
            NU_Nodelist_Push(&result, (NuNode*)(node));
        }
    }
    DepthFirstSearch_Free(&dfs);
    return result.nodelist;
}

_EXPORT NU_Nodelist NU_DESCENDENTS_WITH_CLASS(const NuNode* node, const char* class) {
    Node* nodeP = (Node*)(node);
    NU_Nodelist_Internal result;
    NU_Nodelist_Init(&result, 8);
    DepthFirstSearch dfs = DepthFirstSearch_Create(nodeP);
    Node* currNode;
    while(DepthFirstSearch_Next(&dfs, &currNode)) {
        if (currNode->class != NULL && strcmp(class, currNode->class) == 0) {
            NU_Nodelist_Push(&result, (NuNode*)(currNode));
        }
    }
    DepthFirstSearch_Free(&dfs);
    return result.nodelist;
}

_EXPORT NU_Nodelist NU_DESCENDENTS_WITH_TYPE(const NuNode* node, u8 type) {
    Node* nodeP = (Node*)(node);
    NU_Nodelist_Internal result;
    NU_Nodelist_Init(&result, 8);
    DepthFirstSearch dfs = DepthFirstSearch_Create(nodeP);
    Node* currNode;
    while(DepthFirstSearch_Next(&dfs, &currNode)) {
        if (currNode->type == type) {
            NU_Nodelist_Push(&result, (NuNode*)(currNode));
        }
    }
    DepthFirstSearch_Free(&dfs);
    return result.nodelist;
}

_EXPORT NU_Nodelist NU_CHILDREN(const NuNode* node) {
    Node* nodeP = (Node*)(node);
    NU_Nodelist_Internal result;
    NU_Nodelist_Init(&result, nodeP->childCount);
    Node* child = nodeP->firstChild;
    while(child != NULL) {
        NU_Nodelist_Push(&result, (NuNode*)(child));
        child = child->nextSibling; // move to the next child
    }
    return result.nodelist;
}

_EXPORT NuNode* NU_FIRST_DESCENDENT_WITH_CLASS(const NuNode* node, const char* class) {
    Node* nodeP = (Node*)(node);
    NuNode* result = NULL;
    BreadthFirstSearch_Reset(&GUI.bfs, nodeP);
    Node* bfsNode;
    while(BreadthFirstSearch_Next(&GUI.bfs, &bfsNode)) {
        if (strcmp(bfsNode->class, class) == 0) {
            result = (NuNode*)(bfsNode);
            break;
        }
    }
    return result;
}

_EXPORT NuNode* NU_FIRST_DESCENDENT_WITH_TYPE(const NuNode* node, u8 type) {
    Node* nodeP = (Node*)(node);
    NuNode* result = NULL;
    BreadthFirstSearch_Reset(&GUI.bfs, nodeP);
    Node* bfsNode;
    while(BreadthFirstSearch_Next(&GUI.bfs, &bfsNode)) {
        if (bfsNode->type == type) {
            result = (NuNode*)(bfsNode);
            break;
        }
    }
    return result;
}

_EXPORT int NU_DOES_DESCEND(const NuNode* node, NuNode* ancestor) {
    if (node == NULL || ancestor == NULL) return 0;
    Node* nodeP = (Node*)(node);
    Node* ancestorP = (Node*)(ancestor);
    Node* currP = nodeP->parent;
    while(currP != NULL) {
        if (currP == ancestorP) return 1;
        currP = currP->parent;
    }
    return 0;
}

_EXPORT void NU_NODELIST_FREE(NU_Nodelist* nodelist) {
    free(nodelist->nodes);
    nodelist->count = 0;
}

_EXPORT void NU_SET_CLASS(NuNode* node, const char* class) {
    Node* nodeP = (Node*)(node);
    if (class == nodeP->class) return;

    nodeP->class = NULL;

    // Look for class in gui class string set
    char* gui_class_get = Stringset_Get(&GUI.class_string_set, class);
    if (gui_class_get) {
        nodeP->class = gui_class_get;
    }
    // Not found? Look in the stylesheet
    else {
        void* found = LinearStringmap_Get(&GUI.stylesheet.classIdMap, class);
        if (found) {
            // Add it to the gui class set
            nodeP->class = Stringset_Add(&GUI.class_string_set, class);
        }
        // Class does not exist
        else {
            return;
        }
    }

    // Update styling
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, nodeP);
    if (nodeP == GUI.scroll_mouse_down_node) {
        Stylesheet_ApplyPseudoStyleToNode(&GUI.stylesheet, nodeP, PSEUDO_PRESS);
    } else if (nodeP == GUI.hovered_node) {
        Stylesheet_ApplyPseudoStyleToNode(&GUI.stylesheet, nodeP, PSEUDO_HOVER);
    }
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_REMOVE_CLASS(NuNode* node)
{
    Node* nodeP = (Node*)(node);
    if (nodeP->class == NULL) return;
    nodeP->class = NULL;

    // Update styling
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, nodeP);
    if (nodeP == GUI.scroll_mouse_down_node) {
        Stylesheet_ApplyPseudoStyleToNode(&GUI.stylesheet, nodeP, PSEUDO_PRESS);
    } else if (nodeP == GUI.hovered_node) {
        Stylesheet_ApplyPseudoStyleToNode(&GUI.stylesheet, nodeP, PSEUDO_HOVER);
    }
    GUI.awaiting_redraw = true;
}

_EXPORT void NU_BIND_EVENT(
  NuNode* node,
  NU_EventCallback callback,
  void* args,
  enum NU_EventType event_type,
  int widgetID)
{
    NU_Internal_Bind_Event(node, args, callback, event_type, widgetID);
}



_EXPORT int NU_WIDGET(NuNode* widgetRoot,
    NU_WidgetCreateCallback createCB,
    NU_WidgetUpdateCallback updateCB,
    void* createCB_Args,
    void* updateCB_Args)
{
    // Widget already assigned to root
    if (((Node*)(widgetRoot))->widgetID != -1) return ((Node*)(widgetRoot))->widgetID;

    NU_Widget widget;
    widget.root = (Node*)(widgetRoot);
    widget.updateCB = updateCB;
    widget.updateCB_Args = updateCB_Args;
    int widgetID = Container_Add(&GUI.widgets, &widget);
    createCB(widgetRoot, widgetID, createCB_Args);

    return widgetID;
}

_EXPORT void NU_WIDGET_UPDATE(int widgetID)
{
    NU_Widget* widget = Container_Get(&GUI.widgets, widgetID);
    widget->updateCB((NuNode*)(widget), widget->updateCB_Args, NULL);
}

_EXPORT void NU_WIDGET_DELETE(int widgetID, int deleteRoot)
{
    NU_Widget* widget = Container_Get(&GUI.widgets, widgetID);
    widget->root->widgetID = -1; // unassign
    if (deleteRoot) TreeDeleteNode(&GUI.tree, widget->root, NU_DissociateNode);
    Container_Remove(&GUI.widgets, widgetID);
}

// -----------------------------
// --- Canvas API functions ---
// -----------------------------
_EXPORT int64_t NU_CANVAS_CTX(const NuNode* canvasNode)
{
    return NU_Internal_Get_Canvas_Context(canvasNode);
}

_EXPORT void NU_CANVAS_CLEAR(int contextID)
{
    NU_Internal_Clear_Canvas(contextID);
}

_EXPORT void NU_CANVAS_BORDER_RECT(
    int contextID,
    float x, float y, float w, float h,
    float thickness,
    NU_RGB_FL border_col,
    NU_RGB_FL fill_col)
{
    NU_Internal_Border_Rect(contextID, x, y, w, h, thickness, border_col, fill_col);
}

_EXPORT void NU_CANVAS_DRAW_TRIANGLE(
    int contextID,
    float x1, float y1,
    float x2, float y2,
    float x3, float y3,
    float thickness,
    NU_RGB_FL border_col,
    NU_RGB_FL fill_col)
{
    NU_Internal_Triangle(contextID, x1, y1, x2, y2, x3, y3, thickness, border_col, fill_col);
}

_EXPORT void NU_CANVAS_DRAW_VLINE(
    int contextID,
    float x, float y, float height,
    float thickness,
    NU_RGB_FL col)
{
    NU_Internal_Vline(contextID, x, y, height, thickness, col);
}

_EXPORT void NU_CANVAS_DRAW_HLINE(
    int contextID,
    float x, float y, float width,
    float thickness,
    NU_RGB_FL col)
{
    NU_Internal_Hline(contextID, x, y, width, thickness, col);
}

_EXPORT void NU_CANVAS_DRAW_LINE(
    int contextID,
    float x1, float y1, float x2, float y2,
    float thickness,
    NU_RGB_FL col)
{
    NU_Internal_Line(contextID, x1, y1, x2, y2, thickness, col);
}

_EXPORT void NU_CANVAS_DRAW_DASHED_LINE(
    int contextID,
    float x1, float y1, float x2, float y2,
    float thickness,
    u8* dash_pattern,
    u32 dash_pattern_len,
    NU_RGB_FL col)
{
    NU_Internal_Dashed_Line(
        contextID,
        x1, y1, x2, y2,
        thickness,
        dash_pattern,
        dash_pattern_len,
        col);
}

_EXPORT void NU_CANVAS_SET_FONT(
    int contextID,
    const char* font_name)
{
    NU_Internal_Set_Canvas_Font(contextID, font_name);
}

_EXPORT void NU_CANVAS_DRAW_TEXT(
    int contextID,
    float x, float y, float wrapWidth,
    NU_RGB col, const char* string)
{
    NU_Internal_Text(contextID, x, y, wrapWidth, col, string);
}

_EXPORT float NU_CANVAS_TEXT_HEIGHT(
    int contextID,
    float wrapWidth,
    const char* string)
{
    return NU_Internal_Text_Height(contextID, wrapWidth, string);
}

_EXPORT float NU_CANVAS_TEXT_WIDTH(
    int contextID,
    const char* string)
{
    return NU_Internal_Text_Width(contextID, string);
}

_EXPORT void NU_CANVAS_DRAW_LEN_TEXT(
    int contextID,
    float x, float y, float wrapWidth,
    NU_RGB col, const char* string, size_t stringLen)
{
    NU_Internal_LText(contextID, x, y, wrapWidth, col, string, stringLen);
}

_EXPORT float NU_CANVAS_LEN_TEXT_HEIGHT(
    int contextID,
    float wrapWidth,
    const char* string,
    size_t stringLen)
{
    return NU_Internal_LText_Height(contextID, wrapWidth, string, stringLen);
}

_EXPORT float NU_CANVAS_LEN_TEXT_WIDTH(
    int contextID,
    const char* string,
    size_t stringLen)
{
    return NU_Internal_LText_Width(contextID, string, stringLen);
}

_EXPORT float NU_CANVAS_TEXT_LINE_HEIGHT(
    int contextID)
{
    return NU_Internal_Text_Line_Height(contextID);
}
