#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> 
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <freetype/freetype.h>

#ifdef __cplusplus
extern "C" {
#endif


// Opaque structs
typedef struct NU_Stylesheet NU_Stylesheet;

// Visible structs
typedef enum NodeType
{
    NU_WINDOW, NU_BOX, NU_BUTTON, NU_INPUT, NU_CANVAS, NU_IMAGE, NU_TABLE, NU_THEAD, NU_ROW, NU_NAT,
} NodeType;

typedef struct Node
{
    char* textContent;
    float x, y, width, height;
    float contentWidth, contentHeight;
    uint16_t prefWidth, prefHeight;
    uint16_t minWidth, maxWidth, minHeight, maxHeight;
    int16_t left, right, top, bottom;
    uint8_t gap, padTop, padBottom, padLeft, padRight;
    uint8_t borderTop, borderBottom, borderLeft, borderRight;
    uint8_t borderRadiusTl, borderRadiusTr, borderRadiusBl, borderRadiusBr;
    uint8_t backgroundR, backgroundG, backgroundB;
    uint8_t borderR, borderG, borderB;
    uint8_t textR, textG, textB;
} Node;

typedef struct NU_Nodelist
{
    size_t size;
    Node** nodes;
} NU_Nodelist;

typedef struct {
    float r, g, b;
} NU_RGB;

enum NU_Event_Type
{
    NU_EVENT_ON_CLICK,
    NU_EVENT_ON_INPUT_CHANGED,
    NU_EVENT_ON_DRAG,
    NU_EVENT_ON_RELEASED,
    NU_EVENT_ON_RESIZE,
    NU_EVENT_ON_MOUSE_DOWN,
    NU_EVENT_ON_MOUSE_UP,
    NU_EVENT_ON_MOUSE_DOWN_OUTSIDE,
    NU_EVENT_ON_MOUSE_MOVED,
    NU_EVENT_ON_MOUSE_IN,
    NU_EVENT_ON_MOUSE_OUT,
    NU_EVENT_ON_MOUSE_WHEEL,
    NU_EVENT_ON_INPUT_FOCUS,
    NU_EVENT_ON_INPUT_DEFOCUS,
};

typedef struct NU_Event_Info_Mouse
{
    int mouse_btn;
    int mouse_x, mouse_y;
    float delta_x, delta_y;
    float wheel_delta;
} NU_Event_Info_Mouse;

typedef struct NU_Event_Info_Input
{
    char text[5];
} NU_Event_Info_Input;

typedef struct NU_Event
{
    Node* node;
    NU_Event_Info_Mouse mouse;
    NU_Event_Info_Input input;
} NU_Event;

typedef void (*NU_Callback)(NU_Event event, void* args);

struct NU_Callback_Info
{
    NU_Event event;
    void* args;
    NU_Callback callback;
};



// UI functions
__declspec(dllimport) int NU_Create_Gui(const char* xml_filepath, const char* css_filepath);
__declspec(dllimport) void NU_Quit(void);
__declspec(dllimport) int NU_Running(void);

// Stylesheet functions
__declspec(dllimport) uint32_t NU_Load_Stylesheet(const char* css_filepath);
__declspec(dllimport) int NU_Apply_Stylesheet(uint32_t stylesheet_handle);

// Window functions
__declspec(dllimport) void NU_Set_Window_Fullscreen(Node* node);
__declspec(dllimport) void NU_Set_Window_Windowed(Node* node);

// Cursor functions 
__declspec(dllimport) void NU_Set_Cursor_Default(void);
__declspec(dllimport) void NU_Set_Cursor_Pointer(void);
__declspec(dllimport) void NU_Set_Cursor_Text(void);
__declspec(dllimport) void NU_Set_Cursor_Wait(void);
__declspec(dllimport) void NU_Set_Cursor_Crosshair(void);
__declspec(dllimport) void NU_Set_Cursor_Move(void);
__declspec(dllimport) void NU_Set_Cursor_NsResize(void);
__declspec(dllimport) void NU_Set_Cursor_EwResize(void);
__declspec(dllimport) void NU_Set_Cursor_NwseResize(void);
__declspec(dllimport) void NU_Set_Cursor_NeswResize(void);

// DOM functions
__declspec(dllimport) inline Node* NU_PARENT(Node* node);
__declspec(dllimport) inline Node* NU_CHILD(Node* node, uint32_t childIndex);
__declspec(dllimport) inline int NU_CHILD_COUNT(Node* node);
__declspec(dllimport) inline Node* NU_CREATE_NODE(Node* parent, NodeType type);
__declspec(dllimport) inline void NU_DELETE_NODE(Node* node);
__declspec(dllimport) inline void NU_SHIFT_NODE_IN_PARENT(Node* node, int index);
__declspec(dllimport) inline const char* NU_INPUT_TEXT_CONTENT(Node* node);
__declspec(dllimport) void NU_FOCUS_ON_INPUT(Node* node);
__declspec(dllimport) void NU_SET_INPUT_TEXT_CONTENT(Node* node, const char* text);
__declspec(dllimport) Node* NU_HOVERED_NODE();
__declspec(dllimport) inline void NU_SHOW(Node* node);
__declspec(dllimport) inline void NU_HIDE(Node* node);
__declspec(dllimport) int NU_IS_SHOWN(Node* node);
__declspec(dllimport) Node* NU_Get_Node_By_Id(const char* id);
__declspec(dllimport) NU_Nodelist NU_Get_Nodes_By_Class(char* const class);
__declspec(dllimport) NU_Nodelist NU_Get_Descendents_With_Class(Node* node, char* const class);
__declspec(dllimport) NU_Nodelist NU_Get_Nodes_By_Tag(NodeType type);
__declspec(dllimport) NU_Nodelist NU_Get_Children(Node* node);
__declspec(dllimport) Node* NU_Get_First_Descendent_With_Class(Node* node, const char* class);
__declspec(dllimport) int NU_Descends_From(Node* node, Node* ancestor);
__declspec(dllimport) void NU_Nodelist_Free(NU_Nodelist* nodelist);
__declspec(dllimport) void NU_Set_Class(Node* node, const char* class);

// Event functions
__declspec(dllimport) void NU_Register_Event(
  Node* node, 
  void* args,
  NU_Callback callback, 
  enum NU_Event_Type event
); 

// Canvas functions
__declspec(dllimport) int NU_Get_Canvas_Ctx(Node* canvasNode);

__declspec(dllimport) void NU_Clear_Canvas(int contextID);

__declspec(dllimport) void NU_Render();

__declspec(dllimport) NU_RGB NU_RGB_From_Hex(const char* hex);

__declspec(dllimport) void NU_Border_Rect(
    int contextID,
    float x, float y, float w, float h,
    float thickness,
    NU_RGB border_col,
    NU_RGB fill_col
);

__declspec(dllimport) void NU_Line(
    int contextID,
    float x1, float y1, float x2, float y2,
    float thickness,
    NU_RGB col
);

__declspec(dllimport) void NU_Dashed_Line(
    int contextID,
    float x1, float y1, float x2, float y2,
    float thickness,
    uint8_t* dash_pattern,
    uint32_t dash_pattern_len,
    NU_RGB col
);

__declspec(dllimport) void NU_Set_Canvas_Font(int contextID, const char* font_name);

__declspec(dllimport) void NU_Text(
    int contextID,
    float x, float y, float wrapWidth,
    NU_RGB col, const char* string
);

__declspec(dllimport) float NU_Text_Height(
    int contextID,
    float wrapWidth,
    const char* string
);

__declspec(dllimport) float NU_Text_Width(
    int contextID,
    const char* string
);

__declspec(dllimport) float NU_Text_Line_Height(int contextID);

#ifdef __cplusplus
}
#endif