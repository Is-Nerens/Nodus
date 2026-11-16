#pragma once
#include "nu_keycode_enums.h"

#define NU_EVENT_FLAG_ON_CLICK                  (1U << 0)
#define NU_EVENT_FLAG_ON_INPUT_CHANGED          (1U << 1)
#define NU_EVENT_FLAG_ON_SCROLL                 (1U << 2)
#define NU_EVENT_FLAG_ON_RESIZE                 (1U << 3)
#define NU_EVENT_FLAG_ON_MOUSE_DOWN             (1U << 4)
#define NU_EVENT_FLAG_ON_MOUSE_UP               (1U << 5)
#define NU_EVENT_FLAG_ON_MOUSE_DOWN_OUTSIDE     (1U << 6)
#define NU_EVENT_FLAG_ON_MOUSE_MOVED            (1U << 7)
#define NU_EVENT_FLAG_ON_MOUSE_IN               (1U << 8)
#define NU_EVENT_FLAG_ON_MOUSE_OUT              (1U << 9)
#define NU_EVENT_FLAG_ON_MOUSE_WHEEL            (1U << 10)
#define NU_EVENT_FLAG_ON_INPUT_FOCUS            (1U << 11)
#define NU_EVENT_FLAG_ON_INPUT_DEFOCUS          (1U << 12)
#define NU_EVENT_FLAG_ON_KEY_DOWN               (1U << 13)
#define NU_EVENT_FLAG_ON_KEY_UP                 (1U << 14)
#define NU_EVENT_FLAG_ON_TYPE                   (1U << 15)
#define NU_EVENT_FLAG_ON_GAMEPAD_CONNECT        (1U << 16)
#define NU_EVENT_FLAG_ON_GAMEPAD_DISCONNECT     (1U << 17)

typedef enum NuEventType
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
    NU_EVENT_ON_TYPE,
    NU_EVENT_ON_GAMEPAD_CONNECT,
    NU_EVENT_ON_GAMEPAD_DISCONNECT
} NuEventType;

typedef struct NU_Event_Info_Mouse
{
    int mouseBtn;
    int mouseX, mouseY;
    float deltaX, deltaY;
    float wheelDelta;
} NU_Event_Info_Mouse;

typedef struct NU_Event_Info_Gamepad
{
    uint8 gamepadID;
} NU_Event_Info_Gamepad;

typedef struct NU_Event_Info_Input
{
    char text[5];
} NU_Event_Info_Input;

typedef struct NU_Event_Info_Keypress
{
    NuKeycode keycode;
    bool repeat;
} NU_Event_Info_Keypress;

typedef struct NU_Event_Info_Type
{
    char text[5];
} NU_Event_Info_Type;

typedef struct NU_Event
{
    NuNode* node;
    NU_Event_Info_Mouse mouse;
    NU_Event_Info_Gamepad gamepad;
    NU_Event_Info_Input input;
    NU_Event_Info_Keypress keypress;
    NU_Event_Info_Type type;
    NuEventType eventType;
} NU_Event;

typedef void (*NU_EventCallback)(NU_Event event, void* args);

typedef struct NU_EventCallbackInfo
{
    NU_Event event;
    void* args;
    NU_EventCallback callback;
    int widgetID;
} NU_EventCallbackInfo;

typedef struct EventSystem {
    Hashmap on_click_events;
    Hashmap on_input_changed_events;
    Hashmap on_scroll_events;
    Hashmap on_released_events;
    Hashmap on_resize_events;
    Hashmap node_resize_tracking;
    Hashmap on_mouse_down_events;
    Hashmap on_mouse_up_events;
    Hashmap on_mouse_down_outside_events;
    Hashmap on_mouse_move_events;
    Hashmap on_mouse_in_events;
    Hashmap on_mouse_out_events;
    Hashmap on_mouse_wheel_events;
    Hashmap on_input_focus_events;
    Hashmap on_input_defocus_events;
    Hashmap on_key_down_events;
    Hashmap on_key_up_events;
    Hashmap on_type_events;
    Hashmap on_gamepad_connect_events;
    Hashmap on_gamepad_disconnect_events;
} EventSystem;

// ----------------------------
// --- Function Definitions ---
// ----------------------------
void EventSystem_Init();

void EventSystem_Free();

void NU_Internal_Register_Event(
    NuNode* node,
    void* args,
    NU_EventCallback callback,
    enum NuEventType event_type
);

void NU_Unregister_All_Events(Node* node);

void TriggerOnMouseInEvent(Node* node, float mouseX, float mouseY);

void TriggerOnMouseOutEvent(Node* node, float mouseX, float mouseY);

void TriggerOnMouseDownEvent(Node* node, float mouseX, float mouseY, int mouseBtn);

void TriggerOnClickEvent(Node* node, float mouseX, float mouseY, int mouseBtn);

void TriggerOnScrollEvent(Node* node);

void TriggerOnInputFocusEvent(Node* node);

void TriggerOnInputDefocusEvent(Node* node);

void TriggerOnInputChangedEvent(Node* node, const char* text);

void CheckForResizeEvents();

void TriggerAllMouseupEvents(float mouseX, float mouseY, int mouseBtn);

void TriggerAllMouseMoveEvents(float mouseX, float mouseY, float mouseDeltaX, float mouseDeltaY);

void TriggerAllMouseWheelEvents(float wheelDelta);

void TriggerAllMouseDownOutsideEvents(float mouseX, float mouseY, int mouseBtn);

void TriggerAllOnKeyDownEvents(SDL_Keycode keycode, bool repeat);

void TriggerAllOnKeyUpEvents(SDL_Keycode keycode, bool repeat);

void TriggerAllOnTypeEvents(const char* text);

void NU_Unregister_All_Iterated_Events(Node* node);

void TriggerAllGamepadConnectEvents(uint8 gamepadID);

void TriggerAllGamepadDisconnectEvents(uint8 gamepadID);
