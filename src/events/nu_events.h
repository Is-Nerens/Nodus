#pragma once
#include <events/nu_event_defs.h>
#include <events/nu_sdl_event_handler.h>

void EventSystem_Init()
{
    Hashmap_Init(&GUI.eventSystem.on_click_events,              sizeof(Node*), sizeof(struct NU_EventCallbackInfo), 128);
    Hashmap_Init(&GUI.eventSystem.on_input_changed_events,      sizeof(Node*), sizeof(struct NU_EventCallbackInfo), 10);
    Hashmap_Init(&GUI.eventSystem.on_scroll_events,             sizeof(Node*), sizeof(struct NU_EventCallbackInfo), 10);
    Hashmap_Init(&GUI.eventSystem.on_resize_events,             sizeof(Node*), sizeof(struct NU_EventCallbackInfo), 10);
    Hashmap_Init(&GUI.eventSystem.node_resize_tracking,         sizeof(Node*), sizeof(NU_NodeDimensions)      , 10);
    Hashmap_Init(&GUI.eventSystem.on_mouse_down_events,         sizeof(Node*), sizeof(struct NU_EventCallbackInfo), 10);
    Hashmap_Init(&GUI.eventSystem.on_mouse_up_events,           sizeof(Node*), sizeof(struct NU_EventCallbackInfo), 10);
    Hashmap_Init(&GUI.eventSystem.on_mouse_down_outside_events, sizeof(Node*), sizeof(struct NU_EventCallbackInfo), 10);
    Hashmap_Init(&GUI.eventSystem.on_mouse_move_events,         sizeof(Node*), sizeof(struct NU_EventCallbackInfo), 10);
    Hashmap_Init(&GUI.eventSystem.on_mouse_in_events,           sizeof(Node*), sizeof(struct NU_EventCallbackInfo), 10);
    Hashmap_Init(&GUI.eventSystem.on_mouse_out_events,          sizeof(Node*), sizeof(struct NU_EventCallbackInfo), 10);
    Hashmap_Init(&GUI.eventSystem.on_mouse_wheel_events,        sizeof(Node*), sizeof(struct NU_EventCallbackInfo), 10);
    Hashmap_Init(&GUI.eventSystem.on_input_focus_events,        sizeof(Node*), sizeof(struct NU_EventCallbackInfo), 10);
    Hashmap_Init(&GUI.eventSystem.on_input_defocus_events,      sizeof(Node*), sizeof(struct NU_EventCallbackInfo), 10);
    Hashmap_Init(&GUI.eventSystem.on_key_down_events,           sizeof(Node*), sizeof(struct NU_EventCallbackInfo), 10);
    Hashmap_Init(&GUI.eventSystem.on_key_up_events,             sizeof(Node*), sizeof(struct NU_EventCallbackInfo), 10);
    Hashmap_Init(&GUI.eventSystem.on_type_events,               sizeof(Node*), sizeof(struct NU_EventCallbackInfo), 10);
}

void EventSystem_Free()
{
    Hashmap_Free(&GUI.eventSystem.on_click_events);
    Hashmap_Free(&GUI.eventSystem.on_input_changed_events);
    Hashmap_Free(&GUI.eventSystem.on_scroll_events);
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
    Hashmap_Free(&GUI.eventSystem.on_type_events);
}



void NU_Internal_Bind_Event(NuNode* node, void* args, NU_EventCallback callback, enum NU_EventType event_type, int widgetID)
{
    struct NU_Event event = {0}; event.node = node;
    struct NU_EventCallbackInfo cb_info = { event, args, callback, widgetID };

    Node* nodeP = NODEP_OF(node);

    switch (event_type) {
        case NU_EVENT_ON_CLICK:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_CLICK;
            Hashmap_Set(&GUI.eventSystem.on_click_events, &nodeP, &cb_info);
            break;
        case NU_EVENT_ON_INPUT_CHANGED:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_INPUT_CHANGED;
            Hashmap_Set(&GUI.eventSystem.on_input_changed_events, &node, &cb_info);
            break;
        case NU_EVENT_ON_SCROLL:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_SCROLL;
            Hashmap_Set(&GUI.eventSystem.on_scroll_events, &nodeP, &cb_info);
            break;
        case NU_EVENT_ON_RESIZE:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_RESIZE;
            Hashmap_Set(&GUI.eventSystem.on_resize_events, &nodeP, &cb_info);
            NU_NodeDimensions initial_dimensions = { -1.0f, -1.0f };
            Hashmap_Set(&GUI.eventSystem.node_resize_tracking, &node, &initial_dimensions);
            break;
        case NU_EVENT_ON_MOUSE_DOWN:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_DOWN;
            Hashmap_Set(&GUI.eventSystem.on_mouse_down_events, &nodeP, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_UP:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_UP;
            Hashmap_Set(&GUI.eventSystem.on_mouse_up_events, &nodeP, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_DOWN_OUTSIDE:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_DOWN_OUTSIDE;
            Hashmap_Set(&GUI.eventSystem.on_mouse_down_outside_events, &nodeP, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_MOVED:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_MOVED;
            Hashmap_Set(&GUI.eventSystem.on_mouse_move_events, &nodeP, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_IN:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_IN;
            Hashmap_Set(&GUI.eventSystem.on_mouse_in_events, &nodeP, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_OUT:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_OUT;
            Hashmap_Set(&GUI.eventSystem.on_mouse_out_events, &nodeP, &cb_info);
            break;
        case NU_EVENT_ON_MOUSE_WHEEL:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_MOUSE_WHEEL;
            Hashmap_Set(&GUI.eventSystem.on_mouse_wheel_events, &nodeP, &cb_info);
            break;
        case NU_EVENT_ON_INPUT_FOCUS:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_INPUT_FOCUS;
            Hashmap_Set(&GUI.eventSystem.on_input_focus_events, &nodeP, &cb_info);
            break;
        case NU_EVENT_ON_INPUT_DEFOCUS:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_INPUT_DEFOCUS;
            Hashmap_Set(&GUI.eventSystem.on_input_defocus_events, &nodeP, &cb_info);
            break;
        case NU_EVENT_ON_KEY_DOWN:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_KEY_DOWN;
            Hashmap_Set(&GUI.eventSystem.on_key_down_events, &nodeP, &cb_info);
            break;
        case NU_EVENT_ON_KEY_UP:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_KEY_UP;
            Hashmap_Set(&GUI.eventSystem.on_key_up_events, &nodeP, &cb_info);
            break;
        case NU_EVENT_ON_TYPE:
            nodeP->eventFlags |= NU_EVENT_FLAG_ON_TYPE;
            Hashmap_Set(&GUI.eventSystem.on_type_events, &nodeP, &cb_info);
            break;
    }
}

void NU_Unregister_All_Non_Iterated_Events(Node* node)
{
    if (node->eventFlags & NU_EVENT_FLAG_ON_CLICK) {
        Hashmap_Delete(&GUI.eventSystem.on_click_events, &node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_INPUT_CHANGED) {
        Hashmap_Delete(&GUI.eventSystem.on_input_changed_events, &node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_SCROLL) {
        Hashmap_Delete(&GUI.eventSystem.on_scroll_events, &node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_DOWN) {
        Hashmap_Delete(&GUI.eventSystem.on_mouse_down_events, &node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_DOWN_OUTSIDE) {
        Hashmap_Delete(&GUI.eventSystem.on_mouse_down_outside_events, &node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_MOVED) {
        Hashmap_Delete(&GUI.eventSystem.on_mouse_move_events, &node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_IN) {
        Hashmap_Delete(&GUI.eventSystem.on_mouse_in_events, &node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_OUT) {
        Hashmap_Delete(&GUI.eventSystem.on_mouse_out_events, &node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_INPUT_FOCUS) {
        Hashmap_Delete(&GUI.eventSystem.on_input_focus_events, &node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_INPUT_DEFOCUS) {
        Hashmap_Delete(&GUI.eventSystem.on_input_defocus_events, &node);
    }
}

void NU_Unregister_All_Iterated_Events(Node* node)
{
    if (node->eventFlags & NU_EVENT_FLAG_ON_RESIZE) {
        Hashmap_Delete(&GUI.eventSystem.on_resize_events, &node);
        Hashmap_Delete(&GUI.eventSystem.node_resize_tracking, &node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_UP) {
        Hashmap_Delete(&GUI.eventSystem.on_mouse_up_events, &node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_MOVED) {
        Hashmap_Delete(&GUI.eventSystem.on_mouse_move_events, &node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_WHEEL) {
        Hashmap_Delete(&GUI.eventSystem.on_mouse_wheel_events, &node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_KEY_DOWN) {
        Hashmap_Delete(&GUI.eventSystem.on_input_defocus_events, &node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_KEY_UP) {
        Hashmap_Delete(&GUI.eventSystem.on_input_defocus_events, &node);
    }
    if (node->eventFlags & NU_EVENT_FLAG_ON_TYPE) {
        Hashmap_Delete(&GUI.eventSystem.on_type_events, &node);
    }
}

void NU_DissociateNode(Node* node)
{
    NU_Unregister_All_Non_Iterated_Events(node);

    switch(node->type) {
        case NU_TYPE_WINDOW:
            DisplayManager_DestroyWindow(&GUI.displayManager, node);
            break;
        case NU_TYPE_CANVAS:
            NU_DeleteCanvasContext(node->special.canvas.ctxHandle);
            break;
        case NU_TYPE_INPUT: {
            InputText* inputText = Container_Get(&GUI.textInputs, node->special.input.textInputHandle);
            InputText_Free(inputText);
            Container_Remove(&GUI.textInputs, node->special.input.textInputHandle);
            break;
        }
        default:
            break;
    }
    if (node->id != NULL) {
        Stringmap_Delete(&GUI.id_node_map, node->id);
    }
    if (node == GUI.hovered_node) {
        GUI.hovered_node = NULL;
    }
    if (node == GUI.mouse_down_node) {
        GUI.mouse_down_node = NULL;
    }
    if (node == GUI.scroll_hovered_node) {
        GUI.scroll_hovered_node = NULL;
    }
    if (node == GUI.scroll_mouse_down_node) {
        GUI.scroll_mouse_down_node = NULL;
    }
    if (node == GUI.focused_node) {
        GUI.focused_node = NULL;
    }

    if (node->widgetID != -1) {
        Container_Remove(&GUI.widgets, node->widgetID);
    }
}

void TriggerOnMouseOutEvent(Node* node, float mouseX, float mouseY)
{
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_OUT) {
        void* found_cb = Hashmap_Get(&GUI.eventSystem.on_mouse_out_events, &node);
        if (found_cb != NULL) {
            struct NU_EventCallbackInfo* cb_info = (struct NU_EventCallbackInfo*)found_cb;
            cb_info->event.mouse.mouseBtn = -1;
            cb_info->event.mouse.mouseX = mouseX;
            cb_info->event.mouse.mouseY = mouseY;
            cb_info->event.mouse.deltaX = 0.0f;
            cb_info->event.mouse.deltaY = 0.0f;
            cb_info->event.mouse.wheelDelta = 0.0f;
            if (cb_info->callback) cb_info->callback(cb_info->event, cb_info->args);

            // Run widget update function (if widget is bound)
            if (cb_info->widgetID >= 0) {
                NU_Widget* widget = Container_Get(&GUI.widgets, cb_info->widgetID);
                widget->updateCB((NuNode*)(widget->root), widget->updateCB_Args, &cb_info->event);
            }
        }
    }
}

void TriggerOnMouseInEvent(Node* node, float mouseX, float mouseY)
{
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_IN) {
        void* found_cb = Hashmap_Get(&GUI.eventSystem.on_mouse_in_events, &node);
        if (found_cb != NULL) {
            struct NU_EventCallbackInfo* cb_info = (struct NU_EventCallbackInfo*)found_cb;
            cb_info->event.mouse.mouseBtn = -1;
            cb_info->event.mouse.mouseX = mouseX;
            cb_info->event.mouse.mouseY = mouseY;
            cb_info->event.mouse.deltaX = 0.0f;
            cb_info->event.mouse.deltaY = 0.0f;
            cb_info->event.mouse.wheelDelta = 0.0f;
            if (cb_info->callback) cb_info->callback(cb_info->event, cb_info->args);

            // Run widget update function (if widget is bound)
            if (cb_info->widgetID >= 0) {
                NU_Widget* widget = Container_Get(&GUI.widgets, cb_info->widgetID);
                widget->updateCB((NuNode*)(widget->root), widget->updateCB_Args, &cb_info->event);
            }
        }
    }
}

void TriggerOnMouseDownEvent(Node* node, float mouseX, float mouseY, int mouseBtn)
{
    if (node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_DOWN)
    {
        void* found_cb = Hashmap_Get(&GUI.eventSystem.on_mouse_down_events, &node);
        if (found_cb != NULL)
        {
            struct NU_EventCallbackInfo* cb_info = (struct NU_EventCallbackInfo*)found_cb;
            cb_info->event.mouse.mouseBtn = mouseBtn;
            cb_info->event.mouse.mouseX = mouseX;
            cb_info->event.mouse.mouseY = mouseY;
            if (cb_info->callback) cb_info->callback(cb_info->event, cb_info->args);

            // Run widget update function (if widget is bound)
            if (cb_info->widgetID >= 0) {
                NU_Widget* widget = Container_Get(&GUI.widgets, cb_info->widgetID);
                widget->updateCB((NuNode*)(widget->root), widget->updateCB_Args, &cb_info->event);
            }
        }
    }
}

void TriggerOnClickEvent(Node* node, float mouseX, float mouseY, int mouseBtn)
{
    if (node->eventFlags & NU_EVENT_FLAG_ON_CLICK) {
        void* found_cb = Hashmap_Get(&GUI.eventSystem.on_click_events, &node);
        if (found_cb != NULL) {
            struct NU_EventCallbackInfo* cb_info = (struct NU_EventCallbackInfo*)found_cb;
            cb_info->event.mouse.mouseBtn = mouseBtn;
            cb_info->event.mouse.mouseX = mouseX;
            cb_info->event.mouse.mouseY = mouseY;
            if (cb_info->callback) cb_info->callback(cb_info->event, cb_info->args);

            // Run widget update function (if widget is bound)
            if (cb_info->widgetID >= 0) {
                NU_Widget* widget = Container_Get(&GUI.widgets, cb_info->widgetID);
                widget->updateCB((NuNode*)(widget->root), widget->updateCB_Args, &cb_info->event);
            }
        }
    }
}

void TriggerOnScrollEvent(Node* node)
{
    if (node->eventFlags & NU_EVENT_FLAG_ON_SCROLL) {
        void* found_cb = Hashmap_Get(&GUI.eventSystem.on_scroll_events, &node);
        if (found_cb != NULL) {
            struct NU_EventCallbackInfo* cb_info = (struct NU_EventCallbackInfo*)found_cb;
            if (cb_info->callback) cb_info->callback(cb_info->event, cb_info->args);

            // Run widget update function (if widget is bound)
            if (cb_info->widgetID >= 0) {
                NU_Widget* widget = Container_Get(&GUI.widgets, cb_info->widgetID);
                widget->updateCB((NuNode*)(widget->root), widget->updateCB_Args, &cb_info->event);
            }
        }
    }
}

void TriggerOnInputFocusEvent(Node* node)
{
    if (node->eventFlags & NU_EVENT_FLAG_ON_INPUT_FOCUS) {
        void* found_cb = Hashmap_Get(&GUI.eventSystem.on_input_focus_events, &node);
        if (found_cb != NULL) {
            struct NU_EventCallbackInfo* cb_info = (struct NU_EventCallbackInfo*)found_cb;
            if (cb_info->callback) cb_info->callback(cb_info->event, cb_info->args);

            // Run widget update function (if widget is bound)
            if (cb_info->widgetID >= 0) {
                NU_Widget* widget = Container_Get(&GUI.widgets, cb_info->widgetID);
                widget->updateCB((NuNode*)(widget->root), widget->updateCB_Args, &cb_info->event);
            }
        }
    }
}

void TriggerOnInputDefocusEvent(Node* node)
{
    if (node->eventFlags & NU_EVENT_FLAG_ON_INPUT_DEFOCUS) {
        void* found_cb = Hashmap_Get(&GUI.eventSystem.on_input_defocus_events, &node);
        if (found_cb != NULL) {
            struct NU_EventCallbackInfo* cb_info = (struct NU_EventCallbackInfo*)found_cb;
            if (cb_info->callback) cb_info->callback(cb_info->event, cb_info->args);

            // Run widget update function (if widget is bound)
            if (cb_info->widgetID >= 0) {
                NU_Widget* widget = Container_Get(&GUI.widgets, cb_info->widgetID);
                widget->updateCB((NuNode*)(widget->root), widget->updateCB_Args, &cb_info->event);
            }
        }
    }
}

void TriggerOnInputChangedEvent(Node* node, const char* text)
{
    if (node->eventFlags & NU_EVENT_FLAG_ON_INPUT_CHANGED) {
        void* found_cb = Hashmap_Get(&GUI.eventSystem.on_input_changed_events, &node);
        if (found_cb != NULL) {
            struct NU_EventCallbackInfo* cb_info = (struct NU_EventCallbackInfo*)found_cb;
            strcpy(cb_info->event.input.text, text);
            if (cb_info->callback) cb_info->callback(cb_info->event, cb_info->args);

            // Run widget update function (if widget is bound)
            if (cb_info->widgetID >= 0) {
                NU_Widget* widget = Container_Get(&GUI.widgets, cb_info->widgetID);
                widget->updateCB((NuNode*)(widget->root), widget->updateCB_Args, &cb_info->event);
            }
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
            struct NU_EventCallbackInfo* cb_info = (struct NU_EventCallbackInfo*)val;

            // Skip if node was recently deleted
            if (NodeStateDeleted(node)) continue;

            // Get current dimensions
            void* dims_get = Hashmap_Get(resizeTrackingHmap, &node);
            NU_NodeDimensions* dims = (NU_NodeDimensions*)dims_get;

            // Resize detected? -> update dims and call callback
            if (dims->width != -1.0f && (node->public.generic.width != dims->width || node->public.generic.height != dims->height)) {
                dims->width = node->public.generic.width;
                dims->height = node->public.generic.height;
                if (cb_info->callback) cb_info->callback(cb_info->event, cb_info->args);

                // Run widget update function (if widget is bound)
                if (cb_info->widgetID >= 0) {
                    NU_Widget* widget = Container_Get(&GUI.widgets, cb_info->widgetID);
                    widget->updateCB((NuNode*)(widget->root), widget->updateCB_Args, &cb_info->event);
                }
                continue;
            }

            // Init dims
            if (dims->width == -1.0f) {
                dims->width = node->public.generic.width;
                dims->height = node->public.generic.height;
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
            struct NU_EventCallbackInfo* cb_info = (struct NU_EventCallbackInfo*)val;

            // Skip if node was recently deleted
            if (NodeStateDeleted(node)) continue;

            // Set calback event values and trigger
            cb_info->event.mouse.mouseBtn = mouseBtn;
            cb_info->event.mouse.mouseX = mouseX;
            cb_info->event.mouse.mouseY = mouseY;
            cb_info->event.mouse.deltaX = 0.0f;
            cb_info->event.mouse.deltaY = 0.0f;
            cb_info->event.mouse.wheelDelta = 0.0f;
            if (cb_info->callback) cb_info->callback(cb_info->event, cb_info->args);

            // Run widget update function (if widget is bound)
            if (cb_info->widgetID >= 0) {
                NU_Widget* widget = Container_Get(&GUI.widgets, cb_info->widgetID);
                widget->updateCB((NuNode*)(widget->root), widget->updateCB_Args, &cb_info->event);
            }
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
            struct NU_EventCallbackInfo* cb_info = (struct NU_EventCallbackInfo*)val;

            // Skip if node was recently deleted
            if (NodeStateDeleted(node)) continue;

            // Set callback event values and trigger
            cb_info->event.mouse.mouseBtn = -1;
            cb_info->event.mouse.mouseX = mouseX;
            cb_info->event.mouse.mouseY = mouseY;
            cb_info->event.mouse.deltaX = mouseDeltaX;
            cb_info->event.mouse.deltaY = mouseDeltaY;
            cb_info->event.mouse.wheelDelta = 0.0f;
            if (cb_info->callback) cb_info->callback(cb_info->event, cb_info->args);

            // Run widget update function (if widget is bound)
            if (cb_info->widgetID >= 0) {
                NU_Widget* widget = Container_Get(&GUI.widgets, cb_info->widgetID);
                widget->updateCB((NuNode*)(widget->root), widget->updateCB_Args, &cb_info->event);
            }
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
            struct NU_EventCallbackInfo* cb_info = (struct NU_EventCallbackInfo*)val;

            // Skip if node was recently deleted
            if (NodeStateDeleted(node)) continue;

            // Set calback event values and trigger
            cb_info->event.mouse.wheelDelta = wheelDelta;
            if (cb_info->callback) cb_info->callback(cb_info->event, cb_info->args);

            // Run widget update function (if widget is bound)
            if (cb_info->widgetID >= 0) {
                NU_Widget* widget = Container_Get(&GUI.widgets, cb_info->widgetID);
                widget->updateCB((NuNode*)(widget->root), widget->updateCB_Args, &cb_info->event);
            }
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
            struct NU_EventCallbackInfo* cb_info = (struct NU_EventCallbackInfo*)val;

            // Skip if node was recently deleted OR node is hovered
            if (NodeStateDeleted(node) || node == GUI.hovered_node) continue;

            // Set calback event values and trigger
            cb_info->event.mouse.mouseBtn = mouseBtn;
            cb_info->event.mouse.mouseX = mouseX;
            cb_info->event.mouse.mouseY = mouseY;
            cb_info->event.mouse.deltaX = 0.0f;
            cb_info->event.mouse.deltaY = 0.0f;
            cb_info->event.mouse.wheelDelta = 0.0f;
            if (cb_info->callback) cb_info->callback(cb_info->event, cb_info->args);

            // Run widget update function (if widget is bound)
            if (cb_info->widgetID >= 0) {
                NU_Widget* widget = Container_Get(&GUI.widgets, cb_info->widgetID);
                widget->updateCB((NuNode*)(widget->root), widget->updateCB_Args, &cb_info->event);
            }
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
            struct NU_EventCallbackInfo* cb_info = (struct NU_EventCallbackInfo*)val;

            // Skip if node was recently deleted
            if (NodeStateDeleted(node)) continue;

            // Set calback event values and trigger
            cb_info->event.keypress.keycode = keycode;
            cb_info->event.keypress.repeat = repeat;
            if (cb_info->callback) cb_info->callback(cb_info->event, cb_info->args);

            // Run widget update function (if widget is bound)
            if (cb_info->widgetID >= 0) {
                NU_Widget* widget = Container_Get(&GUI.widgets, cb_info->widgetID);
                widget->updateCB((NuNode*)(widget->root), widget->updateCB_Args, &cb_info->event);
            }
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
            struct NU_EventCallbackInfo* cb_info = (struct NU_EventCallbackInfo*)val;

            // Skip if node was recently deleted
            if (NodeStateDeleted(node)) continue;

            // Set calback event values and trigger
            cb_info->event.keypress.keycode = keycode;
            cb_info->event.keypress.repeat = repeat;
            if (cb_info->callback) cb_info->callback(cb_info->event, cb_info->args);

            // Run widget update function (if widget is bound)
            if (cb_info->widgetID >= 0) {
                NU_Widget* widget = Container_Get(&GUI.widgets, cb_info->widgetID);
                widget->updateCB((NuNode*)(widget->root), widget->updateCB_Args, &cb_info->event);
            }
        }
    }
}

void TriggerAllOnTypeEvents(const char* text)
{
    if (GUI.eventSystem.on_type_events.itemCount > 0)
    {
        Hashmap* hmap = &GUI.eventSystem.on_type_events;
        HashmapIterator it = Hashmap_CreateIterator(hmap);
        void* key; void* val;

        while(Hashmap_IteratorNext(&it, &key, &val))
        {
            // Cast key, value to correct types
            Node* node = *(Node**)key;
            struct NU_EventCallbackInfo* cb_info = (struct NU_EventCallbackInfo*)val;

            // Skip if node was recently deleted
            if (NodeStateDeleted(node)) continue;

            // Set calback event values and trigger
            strcpy(cb_info->event.type.text, text);
            if (cb_info->callback) cb_info->callback(cb_info->event, cb_info->args);

            // Run widget update function (if widget is bound)
            if (cb_info->widgetID >= 0) {
                NU_Widget* widget = Container_Get(&GUI.widgets, cb_info->widgetID);
                widget->updateCB((NuNode*)(widget->root), widget->updateCB_Args, &cb_info->event);
            }
        }
    }
}
