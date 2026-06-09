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
    NU_WINDOW, NU_BOX, NU_BUTTON,
    NU_INPUT, NU_CANVAS, NU_IMAGE, 
    NU_TABLE, NU_THEAD, NU_ROW, NU_NAT,
    NU_FRAME, NU_IMPORT
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
    NU_EVENT_ON_SCROLL,
    NU_EVENT_ON_KEY_DOWN,
    NU_EVENT_ON_KEY_UP
};

enum NU_Keycode
{
    NU_KEY_UNKNOWN = 0,
    
    // Printable keys (ASCII range - these are correct)
    NU_KEY_SPACE = 32,
    NU_KEY_APOSTROPHE = 39,
    NU_KEY_COMMA = 44,
    NU_KEY_MINUS = 45,
    NU_KEY_PERIOD = 46,
    NU_KEY_SLASH = 47,
    NU_KEY_0 = 48,
    NU_KEY_1 = 49,
    NU_KEY_2 = 50,
    NU_KEY_3 = 51,
    NU_KEY_4 = 52,
    NU_KEY_5 = 53,
    NU_KEY_6 = 54,
    NU_KEY_7 = 55,
    NU_KEY_8 = 56,
    NU_KEY_9 = 57,
    NU_KEY_SEMICOLON = 59,
    NU_KEY_EQUALS = 61,
    NU_KEY_A = 97,
    NU_KEY_B = 98,
    NU_KEY_C = 99,
    NU_KEY_D = 100,
    NU_KEY_E = 101,
    NU_KEY_F = 102,
    NU_KEY_G = 103,
    NU_KEY_H = 104,
    NU_KEY_I = 105,
    NU_KEY_J = 106,
    NU_KEY_K = 107,
    NU_KEY_L = 108,
    NU_KEY_M = 109,
    NU_KEY_N = 110,
    NU_KEY_O = 111,
    NU_KEY_P = 112,
    NU_KEY_Q = 113,
    NU_KEY_R = 114,
    NU_KEY_S = 115,
    NU_KEY_T = 116,
    NU_KEY_U = 117,
    NU_KEY_V = 118,
    NU_KEY_W = 119,
    NU_KEY_X = 120,
    NU_KEY_Y = 121,
    NU_KEY_Z = 122,
    NU_KEY_LEFT_BRACKET = 91,
    NU_KEY_BACKSLASH = 92,
    NU_KEY_RIGHT_BRACKET = 93,
    NU_KEY_GRAVE = 96,
    
    // Control keys (corrected from SDL header)
    NU_KEY_ESCAPE = 27,
    NU_KEY_ENTER = 13,
    NU_KEY_TAB = 9,
    NU_KEY_BACKSPACE = 8,
    NU_KEY_DELETE = 127,
    NU_KEY_CLEAR = 0x4000009cu,  // 1073741980 - from SDL header
    
    // Navigation keys (CORRECTED - these were wrong!)
    NU_KEY_UP = 0x40000052u,        // 1073741906
    NU_KEY_DOWN = 0x40000051u,      // 1073741905
    NU_KEY_LEFT = 0x40000050u,      // 1073741904
    NU_KEY_RIGHT = 0x4000004fu,     // 1073741903
    NU_KEY_HOME = 0x4000004au,      // 1073741898
    NU_KEY_END = 0x4000004du,       // 1073741901
    NU_KEY_PAGE_UP = 0x4000004bu,   // 1073741899
    NU_KEY_PAGE_DOWN = 0x4000004eu, // 1073741902
    NU_KEY_INSERT = 0x40000049u,    // 1073741897
    
    // Modifier keys (corrected)
    NU_KEY_LEFT_SHIFT = 0x400000e1u,   // 1073742049
    NU_KEY_LEFT_CTRL = 0x400000e0u,    // 1073742048
    NU_KEY_LEFT_ALT = 0x400000e2u,     // 1073742050
    NU_KEY_LEFT_GUI = 0x400000e3u,     // 1073742051
    NU_KEY_RIGHT_SHIFT = 0x400000e5u,  // 1073742053
    NU_KEY_RIGHT_CTRL = 0x400000e4u,   // 1073742052
    NU_KEY_RIGHT_ALT = 0x400000e6u,    // 1073742054
    NU_KEY_RIGHT_GUI = 0x400000e7u,    // 1073742055
    
    // Lock keys (corrected)
    NU_KEY_CAPS_LOCK = 0x40000039u,    // 1073741881
    NU_KEY_NUM_LOCK = 0x40000053u,     // 1073741907
    NU_KEY_SCROLL_LOCK = 0x40000047u,  // 1073741895
    
    // Keypad keys (corrected)
    NU_KEY_KP_0 = 0x40000062u,         // 1073741922
    NU_KEY_KP_1 = 0x40000059u,         // 1073741913
    NU_KEY_KP_2 = 0x4000005au,         // 1073741914
    NU_KEY_KP_3 = 0x4000005bu,         // 1073741915
    NU_KEY_KP_4 = 0x4000005cu,         // 1073741916
    NU_KEY_KP_5 = 0x4000005du,         // 1073741917
    NU_KEY_KP_6 = 0x4000005eu,         // 1073741918
    NU_KEY_KP_7 = 0x4000005fu,         // 1073741919
    NU_KEY_KP_8 = 0x40000060u,         // 1073741920
    NU_KEY_KP_9 = 0x40000061u,         // 1073741921
    NU_KEY_KP_PERIOD = 0x40000063u,    // 1073741923
    NU_KEY_KP_DIVIDE = 0x40000054u,    // 1073741908
    NU_KEY_KP_MULTIPLY = 0x40000055u,  // 1073741909
    NU_KEY_KP_MINUS = 0x40000056u,     // 1073741910
    NU_KEY_KP_PLUS = 0x40000057u,      // 1073741911
    NU_KEY_KP_ENTER = 0x40000058u,     // 1073741912
    NU_KEY_KP_EQUALS = 0x40000067u,    // 1073741927
    NU_KEY_KP_COMMA = 0x40000085u,     // 1073741957
    NU_KEY_KP_SPACE = 0x400000cdu,     // 1073742029
    
    // Function keys (corrected - these are scancode based)
    NU_KEY_F1 = 0x4000003au,   // 1073741882
    NU_KEY_F2 = 0x4000003bu,   // 1073741883
    NU_KEY_F3 = 0x4000003cu,   // 1073741884
    NU_KEY_F4 = 0x4000003du,   // 1073741885
    NU_KEY_F5 = 0x4000003eu,   // 1073741886
    NU_KEY_F6 = 0x4000003fu,   // 1073741887
    NU_KEY_F7 = 0x40000040u,   // 1073741888
    NU_KEY_F8 = 0x40000041u,   // 1073741889
    NU_KEY_F9 = 0x40000042u,   // 1073741890
    NU_KEY_F10 = 0x40000043u,  // 1073741891
    NU_KEY_F11 = 0x40000044u,  // 1073741892
    NU_KEY_F12 = 0x40000045u,  // 1073741893
    NU_KEY_F13 = 0x40000068u,  // 1073741928
    NU_KEY_F14 = 0x40000069u,  // 1073741929
    NU_KEY_F15 = 0x4000006au,  // 1073741930
    NU_KEY_F16 = 0x4000006bu,  // 1073741931
    NU_KEY_F17 = 0x4000006cu,  // 1073741932
    NU_KEY_F18 = 0x4000006du,  // 1073741933
    NU_KEY_F19 = 0x4000006eu,  // 1073741934
    NU_KEY_F20 = 0x4000006fu,  // 1073741935
    NU_KEY_F21 = 0x40000070u,  // 1073741936
    NU_KEY_F22 = 0x40000071u,  // 1073741937
    NU_KEY_F23 = 0x40000072u,  // 1073741938
    NU_KEY_F24 = 0x40000073u,  // 1073741939
    
    // Print screen and pause (corrected)
    NU_KEY_PRINT_SCREEN = 0x40000046u,  // 1073741894
    NU_KEY_PAUSE = 0x40000048u,        // 1073741896
    
    // Miscellaneous (corrected)
    NU_KEY_APPLICATION = 0x40000065u,   // 1073741925
    NU_KEY_POWER = 0x40000066u,        // 1073741926
    NU_KEY_EXECUTE = 0x40000074u,      // 1073741940
    NU_KEY_HELP = 0x40000075u,         // 1073741941
    NU_KEY_MENU = 0x40000076u,         // 1073741942
    NU_KEY_SELECT = 0x40000077u,       // 1073741943
    NU_KEY_STOP = 0x40000078u,         // 1073741944
    NU_KEY_AGAIN = 0x40000079u,        // 1073741945
    NU_KEY_UNDO = 0x4000007au,         // 1073741946
    NU_KEY_CUT = 0x4000007bu,          // 1073741947
    NU_KEY_COPY = 0x4000007cu,         // 1073741948
    NU_KEY_PASTE = 0x4000007du,        // 1073741949
    NU_KEY_FIND = 0x4000007eu,         // 1073741950
    NU_KEY_MUTE = 0x4000007fu,         // 1073741951
    NU_KEY_VOLUME_UP = 0x40000080u,    // 1073741952
    NU_KEY_VOLUME_DOWN = 0x40000081u,  // 1073741953
    
    // Audio control (corrected)
    NU_KEY_AUDIO_NEXT = 0x4000010bu,   // 1073742091
    NU_KEY_AUDIO_PREV = 0x4000010cu,   // 1073742092
    NU_KEY_AUDIO_STOP = 0x4000010du,   // 1073742093
    NU_KEY_AUDIO_PLAY = 0x40000106u,   // 1073742086
    NU_KEY_AUDIO_MUTE = 0x4000007fu,   // 1073741951
    NU_KEY_AUDIO_VOLUME_UP = 0x40000080u,   // 1073741952
    NU_KEY_AUDIO_VOLUME_DOWN = 0x40000081u, // 1073741953
    
    // Browser keys (corrected)
    NU_KEY_AC_SEARCH = 0x40000118u,    // 1073742104
    NU_KEY_AC_HOME = 0x40000119u,      // 1073742105
    NU_KEY_AC_BACK = 0x4000011au,      // 1073742106
    NU_KEY_AC_FORWARD = 0x4000011bu,   // 1073742107
    NU_KEY_AC_STOP = 0x4000011cu,      // 1073742108
    NU_KEY_AC_REFRESH = 0x4000011du,   // 1073742109
    NU_KEY_AC_BOOKMARKS = 0x4000011eu, // 1073742110
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

typedef struct NU_Event_Info_Keypress
{
    enum NU_Keycode keycode;
    int repeat;
} NU_Event_Info_Keypress;

typedef struct NU_Event
{
    Node* node;
    NU_Event_Info_Mouse mouse;
    NU_Event_Info_Input input;
    NU_Event_Info_Keypress keypress;
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

// Error functions
__declspec(dllimport) inline void NU_ClearErrors(void);
__declspec(dllimport) const char* NU_GetNextError(void);

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
__declspec(dllimport) inline void NU_REPARENT_NODE(Node* node, Node* newParent);
__declspec(dllimport) float NU_NODE_SCROLL(Node* node);
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

__declspec(dllimport) void NU_Triangle(
    int contextID,
    float x1, float y1, 
    float x2, float y2, 
    float x3, float y3,
    float thickness,
    NU_RGB border_col,
    NU_RGB fill_col
);

__declspec(dllimport) void NU_Vline(
    int contextID,
    float x, float y, float height,
    float thickness,
    NU_RGB col
);

__declspec(dllimport) void NU_Hline(
    int contextID,
    float x, float y, float width,
    float thickness,
    NU_RGB col
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