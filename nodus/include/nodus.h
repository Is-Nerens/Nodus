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
typedef struct NU_Nodelist NU_Nodelist;
typedef struct NU_Stylesheet NU_Stylesheet;

// Visible structs
typedef enum NodeType
{
    NU_WINDOW, NU_BOX, NU_BUTTON, NU_INPUT, NU_CANVAS, NU_IMAGE, NU_TABLE, NU_THEAD, NU_ROW, NU_NAT,
} NodeType;

typedef struct Node
{
    SDL_Window* window;
    char* class;
    char* id;
    char* textContent;
    
    uint64_t inlineStyleFlags; // --- Tracks which styles were applied in xml ---
    uint16_t eventFlags; // --- Event Information
    uint8_t positionAbsolute; // --- Tree information ---

    // --- Styling ---
    float x, y, width, height, preferred_width, preferred_height;
    float minWidth, maxWidth, minHeight, maxHeight;
    float gap, contentWidth, contentHeight, scrollX, scrollV;
    float left, right, top, bottom;
    uint8_t padTop, padBottom, padLeft, padRight;
    uint8_t borderTop, borderBottom, borderLeft, borderRight;
    uint8_t borderRadiusTl, borderRadiusTr, borderRadiusBl, borderRadiusBr;
    uint8_t backgroundR, backgroundG, backgroundB;
    uint8_t borderR, borderG, borderB;
    uint8_t textR, textG, textB;
    uint8_t fontId;
    uint8_t layoutFlags;
    char horizontalAlignment;
    char verticalAlignment;
    char horizontalTextAlignment;
    char verticalTextAlignment;
    bool hideBackground;
} Node;

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
    NU_EVENT_ON_MOUSE_MOVED,
    NU_EVENT_ON_MOUSE_IN,
    NU_EVENT_ON_MOUSE_OUT,
};

typedef struct NU_Event_Info_Mouse
{
    int mouse_btn;
    int mouse_x, mouse_y;
    float delta_x, delta_y;
} NU_Event_Info_Mouse;

typedef struct NU_Event_Info_Input
{
    char text[5];
} NU_Event_Info_Input;

typedef struct NU_Event
{
    uint32_t nodeHandle;
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
__declspec(dllimport) int NU_Create_Gui(char* xml_filepath, char* css_filepath);
__declspec(dllimport) void NU_Quit(void);
__declspec(dllimport) int NU_Running(void);
__declspec(dllimport) void NU_Unblock(void);

// Stylesheet functions
__declspec(dllimport) uint32_t NU_Load_Stylesheet(char* css_filepath);
__declspec(dllimport) int NU_Apply_Stylesheet(uint32_t stylesheet_handle);

// DOM functions
__declspec(dllimport) inline Node* NU_NODE(uint32_t nodeHandle);
__declspec(dllimport) inline uint32_t NU_PARENT(uint32_t nodeHandle);
__declspec(dllimport) inline uint32_t NU_CHILD(uint32_t nodeHandle, uint32_t childIndex);
__declspec(dllimport) inline uint32_t* NU_CHILD_COUNT(uint32_t nodeHandle);
__declspec(dllimport) inline uint32_t NU_DEPTH(uint32_t nodeHandle);
__declspec(dllimport) inline uint32_t NU_CREATE_NODE(uint32_t parentHandle, NodeType type);
__declspec(dllimport) inline const char* NU_INPUT_TEXT_CONTENT(uint32_t nodeHandle);
__declspec(dllimport) inline void NU_SHOW(uint32_t nodeHandle);
__declspec(dllimport) inline void NU_HIDE(uint32_t nodeHandle);
__declspec(dllimport) inline void NU_DELETE_NODE(uint32_t nodeHandle);
__declspec(dllimport) uint32_t NU_Get_Node_By_Id(char* id);
__declspec(dllimport) NU_Nodelist NU_Get_Nodes_By_Class(char* class_name);
__declspec(dllimport) NU_Nodelist NU_Get_Nodes_By_Tag(NodeType type);
__declspec(dllimport) void NU_Set_Class(uint32_t nodeHandle, char* class_name);

// Event functions
__declspec(dllimport) void NU_Register_Event(
  uint32_t node_handle, 
  void* args,
  NU_Callback callback, 
  enum NU_Event_Type event
); 

// Canvas functions
__declspec(dllimport) void NU_Clear_Canvas(uint32_t canvas_handle);

__declspec(dllimport) void NU_Render();

__declspec(dllimport) void NU_Border_Rect(
    uint32_t canvas_handle,
    float x, float y, float w, float h,
    float thickness,
    NU_RGB* border_col,
    NU_RGB* fill_col
);

__declspec(dllimport) void NU_Line(
    uint32_t canvas_handle,
    float x1, float y1, float x2, float y2,
    float thickness,
    NU_RGB* col
);

__declspec(dllimport) void NU_Dashed_Line(
    uint32_t canvas_handle,
    float x1, float y1, float x2, float y2,
    float thickness,
    uint8_t* dash_pattern,
    uint32_t dash_pattern_len,
    NU_RGB* col
);

#ifdef __cplusplus
}
#endif