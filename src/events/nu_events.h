#pragma once
#include <events/nu_event_defs.h>
#include <events/nu_sdl_event_handler.h>

void EventSystem_Init()
{
    Hashmap_Init(&GUI.eventSystem.on_click_events,              sizeof(Node*), sizeof(struct NU_Callback_Info), 128);
    Hashmap_Init(&GUI.eventSystem.on_input_changed_events,      sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    Hashmap_Init(&GUI.eventSystem.on_scroll_events,             sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    Hashmap_Init(&GUI.eventSystem.on_released_events,           sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    Hashmap_Init(&GUI.eventSystem.on_resize_events,             sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    Hashmap_Init(&GUI.eventSystem.node_resize_tracking,         sizeof(Node*), sizeof(NU_NodeDimensions)      , 10);
    Hashmap_Init(&GUI.eventSystem.on_mouse_down_events,         sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    Hashmap_Init(&GUI.eventSystem.on_mouse_up_events,           sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    Hashmap_Init(&GUI.eventSystem.on_mouse_down_outside_events, sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    Hashmap_Init(&GUI.eventSystem.on_mouse_move_events,         sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    Hashmap_Init(&GUI.eventSystem.on_mouse_in_events,           sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    Hashmap_Init(&GUI.eventSystem.on_mouse_out_events,          sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    Hashmap_Init(&GUI.eventSystem.on_mouse_wheel_events,        sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    Hashmap_Init(&GUI.eventSystem.on_input_focus_events,        sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    Hashmap_Init(&GUI.eventSystem.on_input_defocus_events,      sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    Hashmap_Init(&GUI.eventSystem.on_key_down_events,           sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
    Hashmap_Init(&GUI.eventSystem.on_key_up_events,             sizeof(Node*), sizeof(struct NU_Callback_Info), 10);
}

void EventSystem_Free()
{
    Hashmap_Free(&GUI.eventSystem.on_click_events);
    Hashmap_Free(&GUI.eventSystem.on_input_changed_events);
    Hashmap_Free(&GUI.eventSystem.on_scroll_events);
    Hashmap_Free(&GUI.eventSystem.on_released_events);
    Hashmap_Free(&GUI.eventSystem.on_resize_events);
    Hashmap_Free(&GUI.eventSystem.node_resize_tracking);
    Hashmap_Free(&GUI.eventSystem.on_mouse_down_events);
    Hashmap_Free(&GUI.eventSystem.on_mouse_up_events);
    Hashmap_Free(&GUI.eventSystem.on_mouse_down_outside_events);
    Hashmap_Free(&GUI.eventSystem.on_mouse_move_events);
    Hashmap_Free(&GUI.eventSystem.on_mouse_in_events);
    Hashmap_Free(&GUI.eventSystem.on_mouse_out_events);
    Hashmap_Free(&GUI.eventSystem.on_mouse_wheel_events);
    Hashmap_Free(&GUI.eventSystem.on_input_focus_events);
    Hashmap_Free(&GUI.eventSystem.on_input_defocus_events);
    Hashmap_Free(&GUI.eventSystem.on_key_down_events);
    Hashmap_Free(&GUI.eventSystem.on_key_up_events);
}



void NU_Internal_Register_Event(Node* node, void* args, NU_Callback callback, enum NU_Event_Type event_type)
{
    struct NU_Event event = {0}; event.node = node;
    struct NU_Callback_Info cb_info = { event, args, callback };

    NodeP* nodeP = NODEP_OF(node);
    
    switch (event_type) {
        case NU_EVENT_ON_CLICK:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_CLICK;
            Hashmap_Set(&GUI.eventSystem.on_click_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_INPUT_CHANGED:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_INPUT_CHANGED;
            Hashmap_Set(&GUI.eventSystem.on_input_changed_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_SCROLL:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_SCROLL;
            Hashmap_Set(&GUI.eventSystem.on_scroll_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_RELEASED:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_RELEASED;
            Hashmap_Set(&GUI.eventSystem.on_released_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_RESIZE:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_RESIZE;
            Hashmap_Set(&GUI.eventSystem.on_resize_events, &node, &cb_info);
            NU_NodeDimensions initial_dimensions = { -1.0f, -1.0f };
            Hashmap_Set(&GUI.eventSystem.node_resize_tracking, &node, &initial_dimensions);
            break;
        case NU_EVENT_ON_MOUSE_DOWN:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_DOWN;
            Hashmap_Set(&GUI.eventSystem.on_mouse_down_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_UP:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_UP;
            Hashmap_Set(&GUI.eventSystem.on_mouse_up_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_DOWN_OUTSIDE:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_DOWN_OUTSIDE;
            Hashmap_Set(&GUI.eventSystem.on_mouse_down_outside_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_MOVED:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_MOVED;
            Hashmap_Set(&GUI.eventSystem.on_mouse_move_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_IN:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_IN;
            Hashmap_Set(&GUI.eventSystem.on_mouse_in_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_OUT:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_OUT;
            Hashmap_Set(&GUI.eventSystem.on_mouse_out_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_WHEEL:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_WHEEL;
            Hashmap_Set(&GUI.eventSystem.on_mouse_wheel_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_INPUT_FOCUS:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_INPUT_FOCUS;
            Hashmap_Set(&GUI.eventSystem.on_input_focus_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_INPUT_DEFOCUS:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_INPUT_DEFOCUS;
            Hashmap_Set(&GUI.eventSystem.on_input_defocus_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_KEY_DOWN:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_KEY_DOWN;
            Hashmap_Set(&GUI.eventSystem.on_key_down_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_KEY_UP:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_KEY_UP;
            Hashmap_Set(&GUI.eventSystem.on_key_up_events, &node, &cb_info);
            break;
    }
}

void NU_Unregister_All_Non_Iterated_Events(NodeP* node)
{
    if (node->eventFlags & NU_EVENT_FLAG_ON_CLICK) {
        Hashmap_Delete(&GUI.eventSystem.on_click_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_INPUT_CHANGED) {
        Hashmap_Delete(&GUI.eventSystem.on_input_changed_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_SCROLL) {
        Hashmap_Delete(&GUI.eventSystem.on_scroll_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_RELEASED) {
        Hashmap_Delete(&GUI.eventSystem.on_released_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_DOWN) {
        Hashmap_Delete(&GUI.eventSystem.on_mouse_down_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_DOWN_OUTSIDE) {
        Hashmap_Delete(&GUI.eventSystem.on_mouse_down_outside_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_MOVED) {
        Hashmap_Delete(&GUI.eventSystem.on_mouse_move_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_IN) {
        Hashmap_Delete(&GUI.eventSystem.on_mouse_in_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_OUT) {
        Hashmap_Delete(&GUI.eventSystem.on_mouse_out_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_INPUT_FOCUS) {
        Hashmap_Delete(&GUI.eventSystem.on_input_focus_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_INPUT_DEFOCUS) {
        Hashmap_Delete(&GUI.eventSystem.on_input_defocus_events, &node->node);
    }
}

void NU_Unregister_All_Iterated_Events(NodeP* node)
{
    if (node->eventFlags & NU_EVENT_FLAG_ON_RESIZE) {
        Hashmap_Delete(&GUI.eventSystem.on_resize_events, &node->node);
        Hashmap_Delete(&GUI.eventSystem.node_resize_tracking, &node->node); 
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_UP) {
        Hashmap_Delete(&GUI.eventSystem.on_mouse_up_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_MOVED) {
        Hashmap_Delete(&GUI.eventSystem.on_mouse_move_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_WHEEL) {
        Hashmap_Delete(&GUI.eventSystem.on_mouse_wheel_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_KEY_DOWN) {
        Hashmap_Delete(&GUI.eventSystem.on_input_defocus_events, &node->node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_KEY_UP) {
        Hashmap_Delete(&GUI.eventSystem.on_input_defocus_events, &node->node);
    }
}

void TriggerOnMouseOutEvent(NodeP* nodeP, float mouseX, float mouseY)
{
    if (nodeP->eventFlags & NU_EVENT_FLAG_ON_MOUSE_OUT) {
        Node* node = &nodeP->node;
        void* found_cb = Hashmap_Get(&GUI.eventSystem.on_mouse_out_events, &node);
        if (found_cb != NULL) {
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
            cb_info->event.mouse.mouseBtn = -1;
            cb_info->event.mouse.mouseX = mouseX;
            cb_info->event.mouse.mouseY = mouseY;
            cb_info->event.mouse.deltaX = 0.0f;
            cb_info->event.mouse.deltaY = 0.0f;
            cb_info->event.mouse.wheelDelta = 0.0f;
            cb_info->callback(cb_info->event, cb_info->args);
        }
    }
}

void TriggerOnMouseInEvent(NodeP* nodeP, float mouseX, float mouseY)
{
    if (nodeP->eventFlags & NU_EVENT_FLAG_ON_MOUSE_IN) {
        Node* node = &nodeP->node;
        void* found_cb = Hashmap_Get(&GUI.eventSystem.on_mouse_in_events, &node);
        if (found_cb != NULL) {
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
            cb_info->event.mouse.mouseBtn = -1;
            cb_info->event.mouse.mouseX = mouseX;
            cb_info->event.mouse.mouseY = mouseY;
            cb_info->event.mouse.deltaX = 0.0f;
            cb_info->event.mouse.deltaY = 0.0f;
            cb_info->event.mouse.wheelDelta = 0.0f;
            cb_info->callback(cb_info->event, cb_info->args);
        }
    }
}

void TriggerOnMouseDownEvent(NodeP* nodeP, float mouseX, float mouseY, int mouseBtn)
{
    if (nodeP->eventFlags & NU_EVENT_FLAG_ON_MOUSE_DOWN)
    {
        Node* node = &nodeP->node;
        void* found_cb = Hashmap_Get(&GUI.eventSystem.on_mouse_down_events, &node);
        if (found_cb != NULL) 
        {                   
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
            cb_info->event.mouse.mouseBtn = mouseBtn;
            cb_info->event.mouse.mouseX = mouseX;
            cb_info->event.mouse.mouseY = mouseY;
            cb_info->callback(cb_info->event, cb_info->args);
        }
    }
}

void TriggerOnClickEvent(NodeP* nodeP, float mouseX, float mouseY, int mouseBtn)
{
    if (nodeP->eventFlags & NU_EVENT_FLAG_ON_CLICK) {
        Node* node = &nodeP->node;
        void* found_cb = Hashmap_Get(&GUI.eventSystem.on_click_events, &node);
        if (found_cb != NULL) {
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
            cb_info->event.mouse.mouseBtn = mouseBtn;
            cb_info->event.mouse.mouseX = mouseX;
            cb_info->event.mouse.mouseY = mouseY;
            cb_info->callback(cb_info->event, cb_info->args);
        }
    }
}

void TriggerOnScrollEvent(NodeP* nodeP)
{
    if (nodeP->eventFlags & NU_EVENT_FLAG_ON_SCROLL) {
        Node* node = &nodeP->node;
        void* found_cb = Hashmap_Get(&GUI.eventSystem.on_scroll_events, &node);
        if (found_cb != NULL) {
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
            cb_info->callback(cb_info->event, cb_info->args);
        }
    }
}

void TriggerOnInputFocusEvent(NodeP* nodeP)
{
    if (nodeP->eventFlags & NU_EVENT_FLAG_ON_INPUT_FOCUS) {
        Node* node = &nodeP->node;
        void* found_cb = Hashmap_Get(&GUI.eventSystem.on_input_focus_events, &node);
        if (found_cb != NULL) {
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
            cb_info->callback(cb_info->event, cb_info->args);
        }
    }
}

void TriggerOnInputDefocusEvent(NodeP* nodeP)
{
    if (nodeP->eventFlags & NU_EVENT_FLAG_ON_INPUT_DEFOCUS) {
        Node* node = &nodeP->node;
        void* found_cb = Hashmap_Get(&GUI.eventSystem.on_input_defocus_events, &node);
        if (found_cb != NULL) {
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
            cb_info->callback(cb_info->event, cb_info->args);
        }
    }
}

void TriggerOnInputChangedEvent(NodeP* nodeP, const char* text)
{
    if (nodeP->eventFlags & NU_EVENT_FLAG_ON_INPUT_CHANGED) {
        Node* node = &nodeP->node;
        void* found_cb = Hashmap_Get(&GUI.eventSystem.on_input_changed_events, &node);
        if (found_cb != NULL) {
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)found_cb;
            strcpy(cb_info->event.input.text, "");
            cb_info->callback(cb_info->event, cb_info->args);
        }
    }
}


// --------------------------------------------
// --- Iterate over the respective hashmaps ---
// --------------------------------------------
void CheckForResizeEvents()
{
    if (GUI.eventSystem.on_resize_events.itemCount > 0)
    {
        Hashmap* resizeHmap = &GUI.eventSystem.on_resize_events;
        Hashmap* resizeTrackingHmap = &GUI.eventSystem.node_resize_tracking;
        HashmapIterator it = Hashmap_CreateIterator(resizeHmap);
        void* key; void* val;

        while(Hashmap_IteratorNext(&it, &key, &val))
        {
            // Cast key, val to correct types
            Node* node = *(Node**)key; 
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;

            // Skip if node was recently deleted
            NodeP* nodeP = NODEP_OF(node);
            if (NodeStateDeleted(nodeP)) continue;

            // Get current dimensions
            void* dims_get = Hashmap_Get(resizeTrackingHmap, &node);
            NU_NodeDimensions* dims = (NU_NodeDimensions*)dims_get;

            // Resize detected? -> update dims and call callback
            if (dims->width != -1.0f && (node->width != dims->width || node->height != dims->height)) {
                dims->width = node->width;
                dims->height = node->height;
                cb_info->callback(cb_info->event, cb_info->args);
                continue;
            }

            // Init dims
            if (dims->width == -1.0f) {
                dims->width = node->width;
                dims->height = node->height;
            }
        }
    }
}

void TriggerAllMouseupEvents(float mouseX, float mouseY, int mouseBtn)
{
    if (GUI.eventSystem.on_mouse_up_events.itemCount > 0)
    {
        Hashmap* hmap = &GUI.eventSystem.on_mouse_up_events;
        HashmapIterator it = Hashmap_CreateIterator(hmap);
        void* key; void* val;

        while(Hashmap_IteratorNext(&it, &key, &val))
        {
            // Cast key, value to correct types
            Node* node = *(Node**)key; 
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;

            // Skip if node was recently deleted
            NodeP* nodeP = NODEP_OF(node);
            if (NodeStateDeleted(nodeP)) continue;

            // Set calback event values and trigger
            cb_info->event.mouse.mouseBtn = mouseBtn;
            cb_info->event.mouse.mouseX = mouseX;
            cb_info->event.mouse.mouseY = mouseY;
            cb_info->event.mouse.deltaX = 0.0f;
            cb_info->event.mouse.deltaY = 0.0f;
            cb_info->event.mouse.wheelDelta = 0.0f;
            cb_info->callback(cb_info->event, cb_info->args);
        }
    }
}

void TriggerAllMouseMoveEvents(float mouseX, float mouseY, float mouseDeltaX, float mouseDeltaY)
{
    if (GUI.eventSystem.on_mouse_move_events.itemCount > 0)
    {
        Hashmap* hmap = &GUI.eventSystem.on_mouse_move_events;
        HashmapIterator it = Hashmap_CreateIterator(hmap);
        void* key; void* val;

        while(Hashmap_IteratorNext(&it, &key, &val))
        {
            // Cast key, value to correct types
            Node* node = *(Node**)key; 
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;

            // Skip if node was recently deleted
            NodeP* nodeP = NODEP_OF(node);
            if (NodeStateDeleted(nodeP)) continue;

            // Set callback event values and trigger
            cb_info->event.mouse.mouseBtn = -1;
            cb_info->event.mouse.mouseX = mouseX;
            cb_info->event.mouse.mouseY = mouseY;
            cb_info->event.mouse.deltaX = mouseDeltaX;
            cb_info->event.mouse.deltaY = mouseDeltaY;
            cb_info->event.mouse.wheelDelta = 0.0f;
            cb_info->callback(cb_info->event, cb_info->args);
        }
    }
}

void TriggerAllMouseWheelEvents(float wheelDelta)
{
    if (GUI.eventSystem.on_mouse_wheel_events.itemCount > 0)
    {
        Hashmap* hmap = &GUI.eventSystem.on_mouse_wheel_events;
        HashmapIterator it = Hashmap_CreateIterator(hmap);
        void* key; void* val;

        while(Hashmap_IteratorNext(&it, &key, &val))
        {
            // Cast key, value to correct types
            Node* node = *(Node**)key;
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;

            // Skip if node was recently deleted
            NodeP* nodeP = NODEP_OF(node);
            if (NodeStateDeleted(nodeP)) continue;

            // Set calback event values and trigger
            cb_info->event.mouse.wheelDelta = wheelDelta;
            cb_info->callback(cb_info->event, cb_info->args);
        }
    }
}

void TriggerAllMouseDownOutsideEvents(float mouseX, float mouseY, int mouseBtn)
{
    if (GUI.eventSystem.on_mouse_down_outside_events.itemCount > 0)
    {
        Hashmap* hmap = &GUI.eventSystem.on_mouse_down_outside_events;
        HashmapIterator it = Hashmap_CreateIterator(hmap);
        void* key; void* val;

        while(Hashmap_IteratorNext(&it, &key, &val))
        {
            // Cast key, value to correct types
            Node* node = *(Node**)key;
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;

            // Skip if node was recently deleted OR node is hovered
            NodeP* nodeP = NODEP_OF(node);
            if (NodeStateDeleted(nodeP) || node == &GUI.hovered_node->node) continue;

            // Set calback event values and trigger
            cb_info->event.mouse.mouseBtn = mouseBtn;
            cb_info->event.mouse.mouseX = mouseX;
            cb_info->event.mouse.mouseY = mouseY;
            cb_info->event.mouse.deltaX = 0.0f;
            cb_info->event.mouse.deltaY = 0.0f;
            cb_info->event.mouse.wheelDelta = 0.0f;
            cb_info->callback(cb_info->event, cb_info->args);
        }
    }
}

void TriggerAllOnKeyDownEvents(SDL_Keycode keycode, bool repeat)
{
    if (GUI.eventSystem.on_key_down_events.itemCount > 0)
    {
        Hashmap* hmap = &GUI.eventSystem.on_key_down_events;
        HashmapIterator it = Hashmap_CreateIterator(hmap);
        void* key; void* val;

        while(Hashmap_IteratorNext(&it, &key, &val))
        {
            // Cast key, value to correct types
            Node* node = *(Node**)key;
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;

            // Skip if node was recently deleted
            NodeP* nodeP = NODEP_OF(node);
            if (NodeStateDeleted(nodeP)) continue;

            // Set calback event values and trigger
            cb_info->event.keypress.keycode = keycode;
            cb_info->event.keypress.repeat = repeat;
            cb_info->callback(cb_info->event, cb_info->args);
        }
    }
}

void TriggerAllOnKeyUpEvents(SDL_Keycode keycode, bool repeat)
{
    if (GUI.eventSystem.on_key_up_events.itemCount > 0)
    {
        Hashmap* hmap = &GUI.eventSystem.on_key_up_events;
        HashmapIterator it = Hashmap_CreateIterator(hmap);
        void* key; void* val;

        while(Hashmap_IteratorNext(&it, &key, &val))
        {
            // Cast key, value to correct types
            Node* node = *(Node**)key;
            struct NU_Callback_Info* cb_info = (struct NU_Callback_Info*)val;

            // Skip if node was recently deleted
            NodeP* nodeP = NODEP_OF(node);
            if (NodeStateDeleted(nodeP)) continue;

            // Set calback event values and trigger
            cb_info->event.keypress.keycode = keycode;
            cb_info->event.keypress.repeat = repeat;
            cb_info->callback(cb_info->event, cb_info->args);
        }
    }
}



