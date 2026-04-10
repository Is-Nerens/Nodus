#pragma once
#define NU_EVENT_FLAG_ON_CLICK              0x01     // 0b0000000000000001
#define NU_EVENT_FLAG_ON_INPUT_CHANGED      0x02     // 0b0000000000000010
#define NU_EVENT_FLAG_ON_DRAG               0x04     // 0b0000000000000100
#define NU_EVENT_FLAG_ON_RELEASED           0x08     // 0b0000000000001000
#define NU_EVENT_FLAG_ON_RESIZE             0x10     // 0b0000000000010000
#define NU_EVENT_FLAG_ON_MOUSE_DOWN         0x20     // 0b0000000000100000
#define NU_EVENT_FLAG_ON_MOUSE_UP           0x40     // 0b0000000001000000
#define NU_EVENT_FLAG_ON_MOUSE_DOWN_OUTSIDE 0x80     // 0b0000000010000000
#define NU_EVENT_FLAG_ON_MOUSE_MOVED        0x100    // 0b0000000100000000
#define NU_EVENT_FLAG_ON_MOUSE_IN           0x200    // 0b0000001000000000
#define NU_EVENT_FLAG_ON_MOUSE_OUT          0x400    // 0b0000010000000000
#define NU_EVENT_FLAG_ON_MOUSE_WHEEL        0x800    // 0b0000100000000000
#define NU_EVENT_FLAG_ON_INPUT_FOCUS        0x1000   // 0b0001000000000000
#define NU_EVENT_FLAG_ON_INPUT_DEFOCUS      0x2000   // 0b0010000000000000
 
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
    NU_EVENT_ON_INPUT_DEFOCUS
};

typedef struct NU_Event_Info_Mouse
{
    int mouseBtn;
    int mouseX, mouseY;
    float deltaX, deltaY;
    float wheelDelta;
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