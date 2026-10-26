#pragma once

typedef void (*NU_WidgetCreateCallback)(NuNode* root, int widgetID, void* args);
typedef void (*NU_WidgetUpdateCallback)(NuNode* root, void* args, NU_Event*);

typedef struct NU_Widget {
    Node* root;
    NU_WidgetUpdateCallback updateCB;
    void* updateCB_Args;
} NU_Widget;
