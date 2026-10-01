#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Import macro
#if defined(_WIN64)
    #define _IMPORT __declspec(dllimport)
#elif defined(__APPLE__) || defined(__linux__)
    #define _IMPORT __attribute__((visibility("default")))
#else
    #define _IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif


// Opaque structs
typedef struct NU_Stylesheet NU_Stylesheet;

/* Members common to all node types */
#define NU_NODE_FIELDS \
    uint16_t preferredWidth, preferredHeight; \
    uint16_t minWidth, maxWidth; \
    uint16_t minHeight, maxHeight; \
    int16_t left, right, top, bottom; \
    uint8_t gap; \
    uint8_t padTop, padBottom, padLeft, padRight; \
    uint8_t borderTop, borderBottom, borderLeft, borderRight; \
    uint8_t borderRadiusTl, borderRadiusTr; \
    uint8_t borderRadiusBl, borderRadiusBr; \
    uint8_t backgroundR, backgroundG, backgroundB; \
    uint8_t borderR, borderG, borderB; \
    uint8_t textR, textG, textB;

/* Node type opaque definitions */
typedef struct NuNode NuNode;
typedef struct NuWindow NuWindow;
typedef struct NuBox NuBox;
typedef struct NuFrame NuFrame;
typedef struct NuButton NuButton;
typedef struct NuInput NuInput;
typedef struct NuCanvas NuCanvas;
typedef struct NuImage NuImage;
typedef struct NuTable NuTable;
typedef struct NuThead NuThead;
typedef struct NuRow NuRow;

/* Node type definitions */
typedef struct NuNodeAccess {
    NU_NODE_FIELDS;
} NuNodeAccess;

typedef struct NuWindowAccess {
    NU_NODE_FIELDS;
    char* textContent;
} NuWindowAccess;

typedef struct NuBoxAccess {
    NU_NODE_FIELDS;
    char* textContent;
} NuBoxAccess;

typedef struct NuButtonAccess {
    NU_NODE_FIELDS;
    char* textContent;
} NuButtonAccess;

typedef struct NuInputAccess {
    NU_NODE_FIELDS;
} NuInputAccess;

typedef struct NuCanvasAccess {
    NU_NODE_FIELDS;
} NuCanvasAccess;

typedef struct NuImageAccess {
    NU_NODE_FIELDS;
} NuImageAccess;

typedef struct NuTableAccess {
    NU_NODE_FIELDS;
} NuTableAccess;

typedef struct NuTheadAccess {
    NU_NODE_FIELDS;
} NuTheadAccess;

typedef struct NuRowAccess {
    NU_NODE_FIELDS;
} NuRowAccess;

typedef struct NuFrameAccess {
    NU_NODE_FIELDS;
} NuFrameAccess;

typedef struct NU_Nodelist
{
    size_t size;
    NuNode** nodes;
} NU_Nodelist;

typedef struct {
    float r, g, b;
} NU_RGB;

enum NU_EventType
{
    NU_EVENT_ON_CLICK,
    NU_EVENT_ON_INPUT_CHANGED,
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
    NU_EVENT_ON_KEY_UP,
    NU_EVENT_ON_TYPE
};

enum NU_Keycode
{
    NU_KEY_UNKNOWN = 0,
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
    NU_KEY_ESCAPE = 27,
    NU_KEY_ENTER = 13,
    NU_KEY_TAB = 9,
    NU_KEY_BACKSPACE = 8,
    NU_KEY_DELETE = 127,
    NU_KEY_CLEAR = 0x4000009cu,
    NU_KEY_UP = 0x40000052u,        // 1073741906
    NU_KEY_DOWN = 0x40000051u,      // 1073741905
    NU_KEY_LEFT = 0x40000050u,      // 1073741904
    NU_KEY_RIGHT = 0x4000004fu,     // 1073741903
    NU_KEY_HOME = 0x4000004au,      // 1073741898
    NU_KEY_END = 0x4000004du,       // 1073741901
    NU_KEY_PAGE_UP = 0x4000004bu,   // 1073741899
    NU_KEY_PAGE_DOWN = 0x4000004eu, // 1073741902
    NU_KEY_INSERT = 0x40000049u,    // 1073741897
    NU_KEY_LEFT_SHIFT = 0x400000e1u,   // 1073742049
    NU_KEY_LEFT_CTRL = 0x400000e0u,    // 1073742048
    NU_KEY_LEFT_ALT = 0x400000e2u,     // 1073742050
    NU_KEY_LEFT_GUI = 0x400000e3u,     // 1073742051
    NU_KEY_RIGHT_SHIFT = 0x400000e5u,  // 1073742053
    NU_KEY_RIGHT_CTRL = 0x400000e4u,   // 1073742052
    NU_KEY_RIGHT_ALT = 0x400000e6u,    // 1073742054
    NU_KEY_RIGHT_GUI = 0x400000e7u,    // 1073742055
    NU_KEY_CAPS_LOCK = 0x40000039u,    // 1073741881
    NU_KEY_NUM_LOCK = 0x40000053u,     // 1073741907
    NU_KEY_SCROLL_LOCK = 0x40000047u,  // 1073741895
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
    NU_KEY_PRINT_SCREEN = 0x40000046u,  // 1073741894
    NU_KEY_PAUSE = 0x40000048u,        // 1073741896
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
    NU_KEY_AUDIO_NEXT = 0x4000010bu,   // 1073742091
    NU_KEY_AUDIO_PREV = 0x4000010cu,   // 1073742092
    NU_KEY_AUDIO_STOP = 0x4000010du,   // 1073742093
    NU_KEY_AUDIO_PLAY = 0x40000106u,   // 1073742086
    NU_KEY_AUDIO_MUTE = 0x4000007fu,   // 1073741951
    NU_KEY_AUDIO_VOLUME_UP = 0x40000080u,   // 1073741952
    NU_KEY_AUDIO_VOLUME_DOWN = 0x40000081u, // 1073741953
    NU_KEY_AC_SEARCH = 0x40000118u,    // 1073742104
    NU_KEY_AC_HOME = 0x40000119u,      // 1073742105
    NU_KEY_AC_BACK = 0x4000011au,      // 1073742106
    NU_KEY_AC_FORWARD = 0x4000011bu,   // 1073742107
    NU_KEY_AC_STOP = 0x4000011cu,      // 1073742108
    NU_KEY_AC_REFRESH = 0x4000011du,   // 1073742109
    NU_KEY_AC_BOOKMARKS = 0x4000011eu, // 1073742110
};

#define NU_TYPE_WINDOW 0
#define NU_TYPE_BOX    1
#define NU_TYPE_BUTTON 2
#define NU_TYPE_INPUT  3
#define NU_TYPE_CANVAS 4
#define NU_TYPE_IMAGE  5
#define NU_TYPE_TABLE  6
#define NU_TYPE_THEAD  7
#define NU_TYPE_ROW    8
#define NU_TYPE_FRAME  9

typedef struct NU_EventInfoMouse
{
    int mouseBtn;
    int mouseX, mouseY;
    float deltaX, deltaY;
    float wheelDelta;
} NU_EventInfoMouse;

typedef struct NU_EventInfoInput
{
    char text[5];
} NU_EventInfoInput;

typedef struct NU_EventInfoKeypress
{
    enum NU_Keycode keycode;
    bool repeat;
} NU_EventInfoKeypress;

typedef struct NU_EventInfoType
{
    char text[5];
} NU_EventInfoType;

typedef struct NU_Event
{
    NuNode* node;
    NU_EventInfoMouse mouse;
    NU_EventInfoInput input;
    NU_EventInfoKeypress keypress;
    NU_EventInfoType type;
    enum NU_EventType eventType;
} NU_Event;

typedef void (*NU_EventCallback)(NU_Event event, void* args);
typedef void (*NU_Callback)(void* args);
typedef void (*NU_WidgetCreateCallback)(NuNode* root, int widgetID, void* args);
typedef void (*NU_WidgetUpdateCallback)(NuNode* root, void* args, const NU_Event* event);

struct NU_EventCallbackInfo
{
    NU_Event event;
    void* args;
    NU_EventCallback callback;
};



// UI functions
_IMPORT int NU_Create_Gui(const char* xml_filepath, const char* css_filepath);
_IMPORT void NU_Quit(void);
_IMPORT void NU_Run(void);
_IMPORT int NU_AddTimer(uint32_t intervalMs, NU_Callback callback, void* args);
_IMPORT void NU_RemoveTimer(int timer);

// Error functions
_IMPORT void NU_ClearErrors(void);
_IMPORT const char* NU_GetNextError(void);

// Window functions
_IMPORT void NU_Set_Window_Fullscreen(NuNode* node);
_IMPORT void NU_Set_Window_Windowed(NuNode* node);

// Screen query functions
_IMPORT void NU_Get_Screen_Dimensions(NuNode* node, int* screenWidthOut, int* screenHeightOut);

// Cursor functions
_IMPORT void NU_Set_Cursor_Default(void);
_IMPORT void NU_Set_Cursor_Pointer(void);
_IMPORT void NU_Set_Cursor_Text(void);
_IMPORT void NU_Set_Cursor_Wait(void);
_IMPORT void NU_Set_Cursor_Crosshair(void);
_IMPORT void NU_Set_Cursor_Move(void);
_IMPORT void NU_Set_Cursor_NsResize(void);
_IMPORT void NU_Set_Cursor_EwResize(void);
_IMPORT void NU_Set_Cursor_NwseResize(void);
_IMPORT void NU_Set_Cursor_NeswResize(void);

// Node traversal
_IMPORT NuNode* NU_PARENT(NuNode* node);
_IMPORT NuNode* NU_CHILD(NuNode* node, uint32_t childIndex);
_IMPORT NuNode* NU_NEXT_SIBLING(NuNode* node);
_IMPORT NuNode* NU_PREVIOUS_SIBLING(NuNode* node);

// Node queries
_IMPORT uint8_t NU_TYPE(NuNode* node);
_IMPORT int NU_CHILD_COUNT(NuNode* node);
_IMPORT int NU_IS_SHOWN(NuNode* node);

// Node creation functions
_IMPORT NuWindow* NU_WINDOW(NuNode* parent);
_IMPORT NuBox* NU_BOX(NuNode* parent);
_IMPORT NuFrame* NU_FRAME(NuNode* parent);
_IMPORT NuButton* NU_BUTTON(NuNode* parent);
_IMPORT NuInput* NU_INPUT(NuNode* parent);
_IMPORT NuCanvas* NU_CANVAS(NuNode* parent);
_IMPORT NuImage* NU_IMAGE(NuNode* parent);
_IMPORT NuTable* NU_TABLE(NuNode* parent);
_IMPORT NuThead* NU_THEAD(NuNode* parent);
_IMPORT NuRow* NU_ROW(NuNode* parent);

// Node deletion
_IMPORT void NU_DELETE_NODE(NuNode* node);

// Node member variable read access functions
_IMPORT const NuWindowAccess* NU_WINDOW_GET(NuWindow* window);
_IMPORT const NuBoxAccess*    NU_BOX_GET(NuBox* box);
_IMPORT const NuFrameAccess*  NU_FRAME_GET(NuFrame* frame);
_IMPORT const NuButtonAccess* NU_BUTTON_GET(NuButton* button);
_IMPORT const NuInputAccess*  NU_INPUT_GET(NuInput* input);
_IMPORT const NuCanvasAccess* NU_CANVAS_GET(NuCanvas* canvas);
_IMPORT const NuImageAccess*  NU_IMAGE_GET(NuImage* image);
_IMPORT const NuTableAccess*  NU_TABLE_GET(NuTable* table);
_IMPORT const NuTheadAccess*  NU_THEAD_GET(NuThead* thead);
_IMPORT const NuRowAccess*    NU_ROW_GET(NuRow* row);

// Generic node set functions
_IMPORT void NU_SET_PREFERRED_WIDTH(NuNode* node, uint16_t preferredWidth);
_IMPORT void NU_SET_PREFERRED_HEIGHT(NuNode* node, uint16_t preferredHeight);
_IMPORT void NU_SET_MIN_WIDTH(NuNode* node, uint16_t minWidth);
_IMPORT void NU_SET_MAX_WIDTH(NuNode* node, uint16_t maxWidth);
_IMPORT void NU_SET_MIN_HEIGHT(NuNode* node, uint16_t minHeight);
_IMPORT void NU_SET_MAX_HEIGHT(NuNode* node, uint16_t maxHeight);
_IMPORT void NU_SET_LEFT(NuNode* node, uint16_t left);
_IMPORT void NU_SET_RIGHT(NuNode* node, uint16_t right);
_IMPORT void NU_SET_TOP(NuNode* node, uint16_t top);
_IMPORT void NU_SET_BOTTOM(NuNode* node, uint16_t bottom);
_IMPORT void NU_SET_GAP(NuNode* node, uint8_t gap);
_IMPORT void NU_SET_PAD_TOP(NuNode* node, uint8_t padTop);
_IMPORT void NU_SET_PAD_BOTTOM(NuNode* node, uint8_t padBottom);
_IMPORT void NU_SET_PAD_LEFT(NuNode* node, uint8_t padLeft);
_IMPORT void NU_SET_PAD_RIGHT(NuNode* node, uint8_t padRight);
_IMPORT void NU_SET_BORDER_TOP(NuNode* node, uint8_t borderTop);
_IMPORT void NU_SET_BORDER_BOTTOM(NuNode* node, uint8_t borderBottom);
_IMPORT void NU_SET_BORDER_LEFT(NuNode* node, uint8_t borderLeft);
_IMPORT void NU_SET_BORDER_RIGHT(NuNode* node, uint8_t borderRight);
_IMPORT void NU_SET_BORDER_RADIUS_TL(NuNode* node, uint8_t borderRadiusTl);
_IMPORT void NU_SET_BORDER_RADIUS_TR(NuNode* node, uint8_t borderRadiusTr);
_IMPORT void NU_SET_BORDER_RADIUS_BL(NuNode* node, uint8_t borderRadiusBl);
_IMPORT void NU_SET_BORDER_RADIUS_BR(NuNode* node, uint8_t borderRadiusBr);
_IMPORT void NU_SET_BACKGROUND_RGB(NuNode* node, NU_RGB rgb);
_IMPORT void NU_SET_BORDER_RGB(NuNode* node, NU_RGB rgb);
_IMPORT void NU_SET_TEXT_RGB(NuNode* node, NU_RGB rgb);
_IMPORT void NU_SET_BACKGROUND_HEX(NuNode* node, const char* hexcode);
_IMPORT void NU_SET_BORDER_HEX(NuNode* node, const char* hexcode);
_IMPORT void NU_SET_TEXT_HEX(NuNode* node, const char* hexcode);

// Node type specific set functions
_IMPORT void NU_WINDOW_SET_TEXT_CONTENT(NuWindow* window, char* string);
_IMPORT void NU_BOX_SET_TEXT_CONTENT(NuBox* box, char* string);
_IMPORT void NU_BUTTON_SET_TEXT_CONTENT(NuButton* button, char* string);
_IMPORT void NU_INPUT_SET_TEXT_CONTENT(NuInput* input, const char* string);
_IMPORT void NU_WINDOW_SET_TITLE(NuNode* windowNode, const char* title);

// Special node type functions
_IMPORT void NU_INPUT_FOCUS(NuNode* node);
_IMPORT const char* NU_INPUT_GET_TEXT_CONTENT(NuInput* input);


// Generic node functions
_IMPORT NuNode* NU_HOVERED_NODE();
_IMPORT int NU_DOES_DESCEND(NuNode* node, NuNode* ancestor);
_IMPORT void NU_SHOW(NuNode* node);
_IMPORT void NU_HIDE(NuNode* node);
_IMPORT void NU_GROW_VERTICAL(NuNode* node);
_IMPORT void NU_GROW_HORIZONTAL(NuNode* node);
_IMPORT void NU_GROW_BOTH(NuNode* node);
_IMPORT void NU_GROW_NONE(NuNode* node);
_IMPORT void NU_GROW_IGNORE_STYLESHEET(NuNode* node);
_IMPORT void NU_GROW_ENABLE_STYLESHEET(NuNode* node);
_IMPORT void NU_BACKGROUND_VISIBILITY_SHOWN(NuNode* node);
_IMPORT void NU_BACKGROUND_VISIBILITY_HIDDEN(NuNode* node);
_IMPORT void NU_BACKGROUND_VISIBILITY_IGNORE_STYLESHEET(NuNode* node);
_IMPORT void NU_BACKGROUND_VISIBILITY_ENABLE_STYLESHEET(NuNode* node);
_IMPORT void NU_SET_CLASS(NuNode* node, const char* class);
_IMPORT void NU_REMOVE_CLASS(NuNode* node);
_IMPORT void NU_SHIFT_NODE_IN_PARENT(NuNode* node, int index);
_IMPORT void NU_REPARENT_NODE(NuNode* node, NuNode* newParent);
_IMPORT void NU_BIND_EVENT(
  NuNode* node,
  NU_EventCallback callback,
  void* args,
  enum NU_EventType event,
  int widgetID
);

// Node retrieval functions
_IMPORT NuNode* NU_NODE_WITH_ID(const char* id);
_IMPORT NU_Nodelist NU_NODES_WITH_CLASS(char* const class);
_IMPORT NU_Nodelist NU_NODES_WITH_TYPE(uint8_t type);
_IMPORT NU_Nodelist NU_DESCENDENTS_WITH_CLASS(NuNode* node, char* const class);
_IMPORT NU_Nodelist NU_DESCENDENTS_WITH_TYPE(NuNode* node, const uint8_t type);
_IMPORT NuNode* NU_FIRST_DESCENDENT_WITH_CLASS(NuNode* node, const char* class);
_IMPORT NuNode* NU_FIRST_DESCENDENT_WITH_TYPE(NuNode* node, const uint8_t type);
_IMPORT NU_Nodelist NU_CHILDREN(NuNode* node);
_IMPORT void NU_NODELIST_FREE(NU_Nodelist* nodelist);

// Widget functions
_IMPORT int NU_WIDGET(NuNode* widgetRoot,
    NU_WidgetCreateCallback createCB,
    NU_WidgetUpdateCallback updateCB,
    void* createCB_Args,
    void* updateCB_Args
);
_IMPORT void NU_WIDGET_UPDATE(int widgetID);
_IMPORT void NU_WIDGET_DELETE(int widgetID, int deleteRoot);

// Canvas functions
_IMPORT int NU_Get_Canvas_Ctx(NuNode* canvasNode);
_IMPORT void NU_Clear_Canvas(int contextID);
_IMPORT void NU_Render();
_IMPORT NU_RGB NU_RGB_From_Hex(const char* hex);
_IMPORT void NU_Border_Rect(
    int contextID,
    float x, float y, float w, float h,
    float thickness,
    NU_RGB border_col,
    NU_RGB fill_col
);
_IMPORT void NU_Triangle(
    int contextID,
    float x1, float y1,
    float x2, float y2,
    float x3, float y3,
    float thickness,
    NU_RGB border_col,
    NU_RGB fill_col
);
_IMPORT void NU_Vline(
    int contextID,
    float x, float y, float height,
    float thickness,
    NU_RGB col
);
_IMPORT void NU_Hline(
    int contextID,
    float x, float y, float width,
    float thickness,
    NU_RGB col
);
_IMPORT void NU_Line(
    int contextID,
    float x1, float y1, float x2, float y2,
    float thickness,
    NU_RGB col
);
_IMPORT void NU_Dashed_Line(
    int contextID,
    float x1, float y1, float x2, float y2,
    float thickness,
    uint8_t* dash_pattern,
    uint32_t dash_pattern_len,
    NU_RGB col
);
_IMPORT void NU_Set_Canvas_Font(int contextID, const char* font_name);
_IMPORT void NU_Text(
    int contextID,
    float x, float y, float wrapWidth,
    NU_RGB col, const char* string
);
_IMPORT float NU_Text_Height(
    int contextID,
    float wrapWidth,
    const char* string
);
_IMPORT float NU_Text_Width(
    int contextID,
    const char* string
);
_IMPORT void NU_LText(
    int contextID,
    float x, float y, float wrapWidth,
    NU_RGB col, const char* string,
    size_t stringLen
);
_IMPORT float NU_LText_Height(
    int contextID,
    float wrapWidth,
    const char* string,
    size_t stringLen
);
_IMPORT float NU_LText_Width(
    int contextID,
    const char* string,
    size_t stringLen
);
_IMPORT float NU_Text_Line_Height(int contextID);

#ifdef __cplusplus
}
#endif






// void build(void* data)
// {
//     NuWindow* parent;
//     NuBtn* btn = NU_Button((NuNode*)(parent), "btn-class", eventCB, event_flags);
//     NuText* btnText = NU_Text((NuNode*)(btn), "label-class", "text", eventCB, event_flags);
// }

// void update(void* data)
// {
//     Node* widgetRoot = ...;
//     NuBtn* btn = NU_CHILD(widgetRoot, 0);
//     NuText* text = NU_CHILD((Node*)(btn), 0);
//     //Or NuText* = data->text; (if you want to store a text reference)
// }

// int NU_Widget(void* build, void* update);
// int NU_WidgetUpdate(int widgetID);

// // Therefore: If you provide NULL for the update callback, the library will
// // automatically delete all the widget nodes and recreate them by calling "build"



// typedef struct NuWidget {
//     int id;
//     NuNode *root;
//     void (*build)(void *data);
//     void (*update)(void *data);
//     void *data;
// } NuWidget;





// what i realise is that without partial repaints
// the energy consumprtion is just too high
// it seems like if i want to set something
// it is best to use a setter function
// If i want to read something I can totally just
// read from the pointer directly
//
// then there is no ambiguity or special cases
//
// NuButton* button NU_CREATE_BUTTON(parent)
// button->x = 100.0f; button->y = 100.0f;
// button->textContent = "once apon a time";
// button->borderRadiusTopLeft = 5;
// float width = button->width;
// NU_RENDER_BRANCH(button);
//
// OR
// NuButton* button NU_CREATE_BUTTON(parent)
// NU_SET_X((NuNode*)button, 100.0f);
// NU_SET_Y((NuNode*)button, 100.0f);
// NU_SET_TEXT_CONTENT((NuNode*)button, 100.0f);
// NU_SET_BORDER_RADIUS_TOP_LEFT((NuNode*)button, 100.0f);
// const float width = NU_BUTTON_GET(NuButton*)->width;
// const NuButtonGet* buttonGet = NU_BUTTON_GET(NuButton*);
// float height = buttonGet->height;
