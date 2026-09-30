
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H

#include "datastructures/container.h"
#define NODUS_BUILD_DLL
#include "nu_gui.h"
#include <stdio.h>

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
_EXPORT int NU_Create_Gui(const char* xml_filepath, const char* css_filepath) {
    return NU_Internal_Create_Gui(xml_filepath, css_filepath);
}

_EXPORT void NU_Quit(void) {
    NU_Internal_Quit();
}

_EXPORT void NU_Run(void) {
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
        SDL_WaitEventTimeout(&event, 4);
    }
}

_EXPORT void NU_Render()
{
    SDL_Event e;
    SDL_zero(e);
    e.type = GUI.SDL_CUSTOM_RENDER_EVENT;
    SDL_PushEvent(&e);
}

_EXPORT int NU_AddTimer(u32 intervalMs, NU_Callback callback, void* args)
{
    NU_Timer timer;
    timer.intervalMs = intervalMs;
    timer.lastFiredTime = 0;
    timer.callback = callback;
    timer.args = args;
    timer.willBeRemoved = false;
    return Container_Add(&GUI.timers, &timer);
}

_EXPORT void NU_RemoveTimer(int timer)
{
    NU_Timer* _timer = Container_Get(&GUI.timers, timer);
    if (_timer) {
        _timer->willBeRemoved = true;
    }
}

// -----------------------
// --- Error functions ---
// -----------------------
_EXPORT void NU_ClearErrors() {
    ErrorSystem_Clear(&GUI.errorSystem);
}

_EXPORT const char* NU_GetNextError() {
    return ErrorSystem_GetNextError(&GUI.errorSystem);
}

// ------------------------
// --- Window fucntions ---
// ------------------------
_EXPORT void NU_Set_Window_Fullscreen(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    SDL_Window* window = GetSDL_Window(&GUI.winManager, nodeP->windowID);
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
    GUI.awaiting_redraw = true;
    NU_Render();
}

_EXPORT void NU_Set_Window_Windowed(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    SDL_Window* window = GetSDL_Window(&GUI.winManager, nodeP->windowID);
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
    GUI.awaiting_redraw = true;
    NU_Render();
}

// ------------------------------
// --- Screen Query Functions ---
// ------------------------------
_EXPORT void NU_Get_Screen_Dimensions(NuNode* node, int* screenWidthOut, int* screenHeightOut)
{
    Node* nodeP = NODEP_OF(node);
    NU_Window* win = GetNodeWindow(&GUI.winManager, nodeP);
    *screenWidthOut = win->screenWidth;
    *screenHeightOut = win->screenHeight;
}


// ------------------------
// --- Cursor functions ---
// ------------------------
_EXPORT void NU_Set_Cursor_Default(void)
{
    SDL_SetCursor(GUI.cursorDefault);
}

_EXPORT void NU_Set_Cursor_Pointer(void)
{
    SDL_SetCursor(GUI.cursorPointer);
}

_EXPORT void NU_Set_Cursor_Text(void)
{
    SDL_SetCursor(GUI.cursorText);
}

_EXPORT void NU_Set_Cursor_Wait(void)
{
    SDL_SetCursor(GUI.cursorWait);
}

_EXPORT void NU_Set_Cursor_Crosshair(void)
{
    SDL_SetCursor(GUI.cursorCrosshair);
}

_EXPORT void NU_Set_Cursor_Move(void)
{
    SDL_SetCursor(GUI.cursorMove);
}

_EXPORT void NU_Set_Cursor_NsResize(void)
{
    SDL_SetCursor(GUI.cursorNsResize);
}

_EXPORT void NU_Set_Cursor_EwResize(void)
{
    SDL_SetCursor(GUI.cursorEwResize);
}

_EXPORT void NU_Set_Cursor_NwseResize(void)
{
    SDL_SetCursor(GUI.cursorNwseResize);
}

_EXPORT void NU_Set_Cursor_NeswResize(void)
{
    SDL_SetCursor(GUI.cursorNeswResize);
}

// ---------------------
// --- DOM functions ---
// ---------------------
_EXPORT NuNode* NU_PARENT(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    if (nodeP->parent == NULL) return NULL;
    return &nodeP->parent->public.common;
}

_EXPORT NuNode* NU_CHILD(NuNode* node, u32 childIndex) {
    Node* nodeP = NODEP_OF(node);
    if (nodeP == NULL || childIndex >= nodeP->childCount) return NULL;
    Node* child = nodeP->firstChild;
    u32 i = 0;
    while(child != NULL) {
        if (i == childIndex) return &child->public.common;
        i++;
        child = child->nextSibling;
    }
    return NULL;
}

_EXPORT u8 NU_TYPE(NuNode* node) {
    return NODEP_OF(node)->type;
}

_EXPORT int NU_CHILD_COUNT(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    return (int)nodeP->childCount;
}

_EXPORT NuNode* NU_NEXT_SIBLING(NuNode* node)
{
    Node* nodeP = NODEP_OF(node);
    return &nodeP->nextSibling->public.common;
}

_EXPORT NuNode* NU_PREVIOUS_SIBLING(NuNode* node)
{
    Node* nodeP = NODEP_OF(node);
    return &nodeP->prevSibling->public.common;
}

_EXPORT NuWindow* NU_WINDOW(NuNode* parent) {
    if (parent == NULL) return NULL;
    Node* parentP = NODEP_OF(parent);
    Node* node = TreeCreateNode(&GUI.tree, parentP, NU_TYPE_WINDOW);
    CreateSubwindow(&GUI.winManager, node);
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, node);
    return &node->public.window;
}

_EXPORT NuBox* NU_BOX(NuNode* parent) {
    if (parent == NULL) return NULL;
    Node* parentP = NODEP_OF(parent);
    Node* node = TreeCreateNode(&GUI.tree, parentP, NU_TYPE_BOX);
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, node);
    return &node->public.box;
}

_EXPORT NuButton* NU_BUTTON(NuNode* parent) {
    if (parent == NULL) return NULL;
    Node* parentP = NODEP_OF(parent);
    Node* node = TreeCreateNode(&GUI.tree, parentP, NU_TYPE_BUTTON);
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, node);
    return &node->public.button;
}

_EXPORT NuInput* NU_INPUT(NuNode* parent) {
    if (parent == NULL) return NULL;
    Node* parentP = NODEP_OF(parent);
    Node* node = TreeCreateNode(&GUI.tree, parentP, NU_TYPE_INPUT);
    InputText inputText;
    InputText_Init(&inputText);
    node->special.input.textInputHandle = Container_Add(&GUI.textInputs, &inputText);
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, node);
    return &node->public.input;
}

_EXPORT NuCanvas* NU_CANVAS(NuNode* parent) {
    if (parent == NULL) return NULL;
    Node* parentP = NODEP_OF(parent);
    Node* node = TreeCreateNode(&GUI.tree, parentP, NU_TYPE_CANVAS);
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, node);
    return &node->public.canvas;
}

_EXPORT NuImage* NU_IMAGE(NuNode* parent) {
    if (parent == NULL) return NULL;
    Node* parentP = NODEP_OF(parent);
    Node* node = TreeCreateNode(&GUI.tree, parentP, NU_TYPE_IMAGE);
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, node);
    return &node->public.image;
}

_EXPORT NuTable* NU_TABLE(NuNode* parent) {
    if (parent == NULL) return NULL;
    Node* parentP = NODEP_OF(parent);
    Node* node = TreeCreateNode(&GUI.tree, parentP, NU_TYPE_TABLE);
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, node);
    return &node->public.table;
}

_EXPORT NuThead* NU_THEAD(NuNode* parent) {
    if (parent == NULL) return NULL;
    Node* parentP = NODEP_OF(parent);
    Node* node = TreeCreateNode(&GUI.tree, parentP, NU_TYPE_THEAD);
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, node);
    return &node->public.thead;
}

_EXPORT NuRow* NU_ROW(NuNode* parent) {
    if (parent == NULL) return NULL;
    Node* parentP = NODEP_OF(parent);
    Node* node = TreeCreateNode(&GUI.tree, parentP, NU_TYPE_ROW);
    Stylesheet_ApplyStyleToNode(&GUI.stylesheet, node);
    return &node->public.row;
}

_EXPORT void NU_DELETE_NODE(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    return TreeDeleteNode(&GUI.tree, nodeP, NU_DissociateNode);
}

_EXPORT void NU_SHIFT_NODE_IN_PARENT(NuNode* node, int index) {
    Node* nodeP = NODEP_OF(node);
    TreeShiftNodeInParent(&GUI.tree, nodeP, index);
}

_EXPORT void NU_REPARENT_NODE(NuNode* node, Node* newParent) {
    Node* nodeP = NODEP_OF(node);
    Node* newParentP = NODEP_OF(newParent);
    TreeReparentNode(&GUI.tree, nodeP, newParentP);
}

_EXPORT float NU_NODE_SCROLL(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    return nodeP->scrollV;
}

_EXPORT const char* NU_INPUT_TEXT_CONTENT(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    if (nodeP->type != NU_TYPE_INPUT) return NULL;
    InputText* inputText = Container_Get(&GUI.textInputs, nodeP->special.input.textInputHandle);
    return inputText->buffer;
}

_EXPORT void NU_FOCUS_ON_INPUT(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    if (nodeP->type != NU_TYPE_INPUT) return;

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
        NU_Font* font = Stylesheet_Get_Font(&GUI.stylesheet, GUI.focused_node->fontId);
        InputText* inputText = Container_Get(&GUI.textInputs, GUI.focused_node->special.input.textInputHandle);
        InputText_MousePlaceCursor(inputText, GUI.focused_node, font, 1000000.0f);
        SDL_StartTextInput(GetSDL_Window(&GUI.winManager, GUI.focused_node->windowID));

        // Defer offsets calculation
        inputText->updateOffsetsPostLayout = true;

        // Trigger focus event
        TriggerOnInputFocusEvent(nodeP);
        NU_Render();
    }
}

_EXPORT void NU_SET_INPUT_TEXT_CONTENT(NuNode* node, const char* text) {
    Node* nodeP = NODEP_OF(node);
    if (nodeP->type != NU_TYPE_INPUT) return;
    NU_Font* font = Stylesheet_Get_Font(&GUI.stylesheet, nodeP->fontId);
    InputText* inputText = Container_Get(&GUI.textInputs, nodeP->special.input.textInputHandle);
    InputText_SetText(inputText, nodeP, font, text);
    TriggerOnInputChangedEvent(nodeP, "");
    GUI.awaiting_redraw = true;
}

_EXPORT NuNode* NU_HOVERED_NODE() {
    if (!GUI.hovered_node) return NULL;
    return &GUI.hovered_node->public.common;
}

_EXPORT void NU_HIDE(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    nodeP->layoutFlags |= HIDDEN;
}

_EXPORT void NU_SHOW(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    nodeP->layoutFlags &= ~HIDDEN;
}

_EXPORT int NU_IS_SHOWN(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    return !(nodeP->layoutFlags & HIDDEN);
}

_EXPORT void NU_GROW_VERTICAL(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    nodeP->layoutFlags |= GROW_VERTICAL;
}

_EXPORT void NU_GROW_HORIZONTAL(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    nodeP->layoutFlags |= GROW_HORIZONTAL;
}

_EXPORT void NU_GROW_BOTH(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    nodeP->layoutFlags |= GROW_VERTICAL;
    nodeP->layoutFlags |= GROW_HORIZONTAL;
}

_EXPORT void NU_GROW_NONE(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    nodeP->layoutFlags &= ~GROW_VERTICAL;
    nodeP->layoutFlags &= ~GROW_HORIZONTAL;
}

_EXPORT void NU_GROW_IGNORE_STYLESHEET(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    nodeP->overrideStyleFlags |= PROPERTY_FLAG_GROW;
}

_EXPORT void NU_GROW_ENABLE_STYLESHEET(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    nodeP->overrideStyleFlags &= ~PROPERTY_FLAG_GROW;
}

_EXPORT void NU_BACKGROUND_VISIBILITY_SHOWN(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    nodeP->layoutFlags &= ~HIDE_BACKGROUND;
}

_EXPORT void NU_BACKGROUND_VISIBILITY_HIDDEN(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    nodeP->layoutFlags |= HIDE_BACKGROUND;
}

_EXPORT void NU_BACKGROUND_VISIBILITY_IGNORE_STYLESHEET(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    nodeP->overrideStyleFlags |= PROPERTY_FLAG_HIDE_BACKGROUND;
}

_EXPORT void NU_BACKGROUND_VISIBILITY_ENABLE_STYLESHEET(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    nodeP->overrideStyleFlags &= ~PROPERTY_FLAG_HIDE_BACKGROUND;
}

_EXPORT void NU_SET_WINDOW_TITLE(NuNode* windowNode, const char* title) {
    Node* nodeP = NODEP_OF(windowNode);
    if (nodeP->type != NU_TYPE_WINDOW) return;
    SDL_Window* window = GetSDL_Window(&GUI.winManager, nodeP->windowID);
    SDL_SetWindowTitle(window, title);
}

_EXPORT NuNode* NU_NODE_WITH_ID(const char* id) {
    void* found = Stringmap_Get(&GUI.id_node_map, id);
    if (found == NULL) return NULL;
    Node* node = *(Node**)found;
    return &node->public.common;
}

_EXPORT NU_Nodelist NU_NODES_WITH_CLASS(const char* class) {

    NU_Nodelist_Internal result;
    NU_Nodelist_Init(&result, 8);
    DepthFirstSearch dfs = DepthFirstSearch_Create(GUI.tree.root);
    Node* node;
    while(DepthFirstSearch_Next(&dfs, &node)) {
        if (node->class != NULL && strcmp(class, node->class) == 0) {
            NU_Nodelist_Push(&result, &node->public.common);
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
            NU_Nodelist_Push(&result, &node->public.common);
        }
    }
    DepthFirstSearch_Free(&dfs);
    return result.nodelist;
}

_EXPORT NU_Nodelist NU_DESCENDENTS_WITH_CLASS(NuNode* node, const char* class) {
    Node* nodeP = NODEP_OF(node);
    NU_Nodelist_Internal result;
    NU_Nodelist_Init(&result, 8);
    DepthFirstSearch dfs = DepthFirstSearch_Create(nodeP);
    Node* currNode;
    while(DepthFirstSearch_Next(&dfs, &currNode)) {
        if (currNode->class != NULL && strcmp(class, currNode->class) == 0) {
            NU_Nodelist_Push(&result, &currNode->public.common);
        }
    }
    DepthFirstSearch_Free(&dfs);
    return result.nodelist;
}

_EXPORT NU_Nodelist NU_DESCENDENTS_WITH_TYPE(NuNode* node, const u8 type) {
    Node* nodeP = NODEP_OF(node);
    NU_Nodelist_Internal result;
    NU_Nodelist_Init(&result, 8);
    DepthFirstSearch dfs = DepthFirstSearch_Create(nodeP);
    Node* currNode;
    while(DepthFirstSearch_Next(&dfs, &currNode)) {
        if (currNode->type == type) {
            NU_Nodelist_Push(&result, &currNode->public.common);
        }
    }
    DepthFirstSearch_Free(&dfs);
    return result.nodelist;
}

_EXPORT NU_Nodelist NU_CHILDREN(NuNode* node) {
    Node* nodeP = NODEP_OF(node);
    NU_Nodelist_Internal result;
    NU_Nodelist_Init(&result, nodeP->childCount);
    Node* child = nodeP->firstChild;
    while(child != NULL) {
        NU_Nodelist_Push(&result, &child->public.common);
        child = child->nextSibling; // move to the next child
    }
    return result.nodelist;
}

_EXPORT NuNode* NU_FIRST_DESCENDENT_WITH_CLASS(NuNode* node, const char* class) {
    Node* nodeP = NODEP_OF(node);
    NuNode* result = NULL;
    BreadthFirstSearch_Reset(&GUI.bfs, nodeP);
    Node* bfsNode;
    while(BreadthFirstSearch_Next(&GUI.bfs, &bfsNode)) {
        if (strcmp(bfsNode->class, class) == 0) {
            result = &bfsNode->public.common;
            break;
        }
    }
    return result;
}

_EXPORT NuNode* NU_FIRST_DESCENDENT_WITH_TYPE(NuNode* node, const u8 type) {
    Node* nodeP = NODEP_OF(node);
    NuNode* result = NULL;
    BreadthFirstSearch_Reset(&GUI.bfs, nodeP);
    Node* bfsNode;
    while(BreadthFirstSearch_Next(&GUI.bfs, &bfsNode)) {
        if (bfsNode->type == type) {
            result = &bfsNode->public.common;
            break;
        }
    }
    return result;
}

_EXPORT int NU_DOES_DESCEND(NuNode* node, NuNode* ancestor) {
    if (node == NULL || ancestor == NULL) return 0;
    Node* nodeP = NODEP_OF(node);
    Node* ancestorP = NODEP_OF(ancestor);
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
    Node* nodeP = NODEP_OF(node);
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
    Node* nodeP = NODEP_OF(node);
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
  enum NU_Event_Type event_type,
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
    if (NODEP_OF(widgetRoot)->widgetID != -1) return NODEP_OF(widgetRoot)->widgetID;

    NU_Widget widget;
    widget.root = NODEP_OF(widgetRoot);
    widget.updateCB = updateCB;
    widget.updateCB_Args = updateCB_Args;
    int widgetID = Container_Add(&GUI.widgets, &widget);
    createCB(widgetRoot, widgetID, createCB_Args);

    return widgetID;
}

_EXPORT void NU_WIDGET_UPDATE(int widgetID)
{
    NU_Widget* widget = Container_Get(&GUI.widgets, widgetID);
    widget->updateCB(&widget->root->public.common, widget->updateCB_Args);
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
_EXPORT int64_t NU_Get_Canvas_Ctx(NuNode* canvasNode)
{
    return NU_Internal_Get_Canvas_Context(canvasNode);
}

_EXPORT void NU_Clear_Canvas(int contextID)
{
    NU_Internal_Clear_Canvas(contextID);
}

_EXPORT NU_RGB NU_RGB_From_Hex(const char* hex)
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

_EXPORT void NU_Border_Rect(
    int contextID,
    float x, float y, float w, float h,
    float thickness,
    NU_RGB border_col,
    NU_RGB fill_col)
{
    NU_Internal_Border_Rect(contextID, x, y, w, h, thickness, border_col, fill_col);
}

_EXPORT void NU_Triangle(
    int contextID,
    float x1, float y1,
    float x2, float y2,
    float x3, float y3,
    float thickness,
    NU_RGB border_col,
    NU_RGB fill_col)
{
    NU_Internal_Triangle(contextID, x1, y1, x2, y2, x3, y3, thickness, border_col, fill_col);
}

_EXPORT void NU_Vline(
    int contextID,
    float x, float y, float height,
    float thickness,
    NU_RGB col)
{
    NU_Internal_Vline(contextID, x, y, height, thickness, col);
}

_EXPORT void NU_Hline(
    int contextID,
    float x, float y, float width,
    float thickness,
    NU_RGB col)
{
    NU_Internal_Hline(contextID, x, y, width, thickness, col);
}

_EXPORT void NU_Line(
    int contextID,
    float x1, float y1, float x2, float y2,
    float thickness,
    NU_RGB col)
{
    NU_Internal_Line(contextID, x1, y1, x2, y2, thickness, col);
}

_EXPORT void NU_Dashed_Line(
    int contextID,
    float x1, float y1, float x2, float y2,
    float thickness,
    u8* dash_pattern,
    u32 dash_pattern_len,
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

_EXPORT void NU_Set_Canvas_Font(
    int contextID,
    const char* font_name)
{
    NU_Internal_Set_Canvas_Font(contextID, font_name);
}

_EXPORT void NU_Text(
    int contextID,
    float x, float y, float wrapWidth,
    NU_RGB col, const char* string)
{
    NU_Internal_Text(contextID, x, y, wrapWidth, col, string);
}

_EXPORT float NU_Text_Height(
    int contextID,
    float wrapWidth,
    const char* string)
{
    return NU_Internal_Text_Height(contextID, wrapWidth, string);
}

_EXPORT float NU_Text_Width(
    int contextID,
    const char* string)
{
    return NU_Internal_Text_Width(contextID, string);
}

_EXPORT void NU_LText(
    int contextID,
    float x, float y, float wrapWidth,
    NU_RGB col, const char* string, size_t stringLen)
{
    NU_Internal_LText(contextID, x, y, wrapWidth, col, string, stringLen);
}

_EXPORT float NU_LText_Height(
    int contextID,
    float wrapWidth,
    const char* string,
    size_t stringLen)
{
    return NU_Internal_LText_Height(contextID, wrapWidth, string, stringLen);
}

_EXPORT float NU_LText_Width(
    int contextID,
    const char* string,
    size_t stringLen)
{
    return NU_Internal_LText_Width(contextID, string, stringLen);
}

_EXPORT float NU_Text_Line_Height(
    int contextID)
{
    return NU_Internal_Text_Line_Height(contextID);
}
