#pragma once

static bool NU_MouseIsOverNode(NodeP* node, float mouseX, float mouseY)
{
    // get clipping info
    float left_wall = node->node.x;
    float right_wall = node->node.x + node->node.width;
    float top_wall = node->node.y;
    float bottom_wall = node->node.y + node->node.height; 
    if (node->clippedAncestor != NULL) {
        NU_ClipBounds* clip = HashmapGet(&GUI.winManager.clipMap, &node->clippedAncestor);
        left_wall = max(clip->clip_left, node->node.x);
        right_wall = min(clip->clip_right, node->node.x + node->node.width);
        top_wall = max(clip->clip_top, node->node.y);
        bottom_wall = min(clip->clip_bottom, node->node.y + node->node.height);
    }

    // check if mouse is within clipped bounding box
    bool within_x_bound = mouseX >= left_wall && mouseX <= right_wall;
    bool within_y_bound = mouseY >= top_wall && mouseY <= bottom_wall;
    if (!(within_x_bound && within_y_bound)) return false; // Not in bounding rect

    // constrain border radii
    float borderRadiusBl = node->node.borderRadiusBl;
    float borderRadiusBr = node->node.borderRadiusBr;
    float borderRadiusTl = node->node.borderRadiusTl;
    float borderRadiusTr = node->node.borderRadiusTr;
    float left_radii_sum   = borderRadiusTl + borderRadiusBl;
    float right_radii_sum  = borderRadiusTr + borderRadiusBr;
    float top_radii_sum    = borderRadiusTl + borderRadiusTr;
    float bottom_radii_sum = borderRadiusBl + borderRadiusBr;
    if (left_radii_sum   > node->node.height)  { float scale = node->node.height / left_radii_sum;   borderRadiusTl *= scale; borderRadiusBl *= scale; }
    if (right_radii_sum  > node->node.height)  { float scale = node->node.height / right_radii_sum;  borderRadiusTr *= scale; borderRadiusBr *= scale; }
    if (top_radii_sum    > node->node.width )  { float scale = node->node.width  / top_radii_sum;    borderRadiusTl *= scale; borderRadiusTr *= scale; }
    if (bottom_radii_sum > node->node.width )  { float scale = node->node.width  / bottom_radii_sum; borderRadiusBl *= scale; borderRadiusBr *= scale; }

    // rounded border anchors
    vec2 tl_a = { floorf(node->node.x + borderRadiusTl),                    floorf(node->node.y + borderRadiusTl) };
    vec2 tr_a = { floorf(node->node.x + node->node.width - borderRadiusTr), floorf(node->node.y + borderRadiusTr) };
    vec2 bl_a = { floorf(node->node.x + borderRadiusBl),                    floorf(node->node.y + node->node.height - borderRadiusBl) };
    vec2 br_a = { floorf(node->node.x + node->node.width - borderRadiusBr), floorf(node->node.y + node->node.height - borderRadiusBr) };

    // ensure mouse is not in top left rounded deadzone
    if (mouseX < tl_a.x && mouseY < tl_a.y) {
        float dist = sqrtf((mouseX - tl_a.x) * (mouseX - tl_a.x) + (mouseY - tl_a.y) * (mouseY - tl_a.y)); 
        if (dist > borderRadiusTl) return false;
    }

    // ensure mouse is not in top right rounded deadzone
    if (mouseX > tr_a.x && mouseY < tr_a.y) {
        float dist = sqrtf((mouseX - tr_a.x) * (mouseX - tr_a.x) + (mouseY - tr_a.y) * (mouseY - tr_a.y)); 
        if (dist > borderRadiusTr) return false;
    }

    // ensure mouse is not in bottom left rounded deadzone
    if (mouseX < bl_a.x && mouseY > bl_a.y) {
        float dist = sqrtf((mouseX - bl_a.x) * (mouseX - bl_a.x) + (mouseY - bl_a.y) * (mouseY - bl_a.y)); 
        if (dist > borderRadiusBl) return false;
    }

    // ensure mouse is not in bottom right rounded deadzone
    if (mouseX > br_a.x && mouseY > br_a.y) {
        float dist = sqrtf((mouseX - br_a.x) * (mouseX - br_a.x) + (mouseY - br_a.y) * (mouseY - br_a.y)); 
        if (dist > borderRadiusBr) return false;
    }

    return true;
}

static bool NU_Mouse_Over_Node_V_Scrollbar(NodeP* node, float mouse_x, float mouse_y) {
    float track_height = node->node.height - node->node.borderTop - node->node.borderBottom;
    float thumb_height = (track_height / node->node.contentHeight) * track_height;
    float scroll_thumb_left_wall = node->node.x + node->node.width - node->node.borderRight - 8.0f;
    float scroll_thumb_top_wall = node->node.y + node->node.borderTop + (node->scrollV * (track_height - thumb_height));
    bool within_x_bound = mouse_x >= scroll_thumb_left_wall && mouse_x <= scroll_thumb_left_wall + 8.0f;
    bool within_y_bound = mouse_y >= scroll_thumb_top_wall && mouse_y <= scroll_thumb_top_wall + thumb_height;
    return within_x_bound && within_y_bound;
}

inline void TriggerOnMouseOutEvent(Node* node, float mouseX, float mouseY)
{
    // on mouse out event triggered
    void* found_cb = HashmapGet(&GUI.on_mouse_out_events, &node);
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

inline void TriggerOnMouseInEvent(Node* node, float mouseX, float mouseY)
{
    // on mouse out event triggered'
    void* found_cb = HashmapGet(&GUI.on_mouse_in_events, &node);
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

void NU_Mouse_Hover()
{   
    // remove potential pseudo style from current hovered node
    if (GUI.hovered_node != NULL && GUI.hovered_node != GUI.mouse_down_node) {
        NU_Apply_Stylesheet_To_Node(GUI.hovered_node, GUI.stylesheet);
    }
    GUI.prev_hovered_node = GUI.hovered_node;
    GUI.hovered_node = NULL;
    GUI.scroll_hovered_node = NULL;
    if (GUI.winManager.hoveredWindowID == -1) return;

    // get local mouse coords
    float mouseX, mouseY; GetLocalMouseCoords(&GUI.winManager, &mouseX, &mouseY);

    // create a traversal stack
    struct Vector stack;
    Vector_Reserve(&stack, sizeof(NodeP*), 32);

    // add absolute root nodes to stack
    for (u32 i=0; i<GUI.winManager.absoluteRootNodes.size; i++) {
        NodeP* absolute_node = *(NodeP**)Vector_Get(&GUI.winManager.absoluteRootNodes, i);
        if (absolute_node->windowID == GUI.winManager.hoveredWindowID && NU_MouseIsOverNode(absolute_node, mouseX, mouseY)) {
            Vector_Push(&stack, &absolute_node);
        }
    }

    // add window root nodes to stack
    for (u32 i=0; i<GUI.winManager.windowNodes.size; i++) {
        NodeP* node = *(NodeP**)Vector_Get(&GUI.winManager.windowNodes, i);
        if (node->windowID == GUI.winManager.hoveredWindowID){
            Vector_Push(&stack, &node);
            break;
        }
    }

    // traverse the tree
    bool break_loop = false;
    while (stack.size > 0 && !break_loop) 
    {
        // pop the stack
        NodeP* current_node = *(NodeP**)Vector_Get(&stack, stack.size - 1);
        if (!(current_node->layoutFlags & IGNORE_MOUSE)) {
            GUI.hovered_node = current_node;
        }
        stack.size -= 1;

        // skip children
        if (current_node->type == NU_BUTTON) continue;

        // iterate over children
        NodeP* child = current_node->firstChild;
        while(child != NULL) {

            if (child->state == 2 || 
                child->layoutFlags & POSITION_ABSOLUTE || 
                child->type == NU_WINDOW ||
                !NU_MouseIsOverNode(child, mouseX, mouseY))
            {
                child = child->nextSibling; continue;
            }

            // check for scroll hover
            if (child->layoutFlags & OVERFLOW_V_PROPERTY) {
                bool overflow_v = child->node.contentHeight > child->node.height - child->node.borderTop - child->node.borderBottom;
                if (overflow_v) GUI.scroll_hovered_node = child;
            }
            Vector_Push(&stack, &child);

            // move to the next child
            child = child->nextSibling;
        }
    }

    // apply pseudo style to hovered node
    if (GUI.hovered_node != NULL && GUI.hovered_node != GUI.mouse_down_node) {
        NU_Apply_Pseudo_Style_To_Node(GUI.hovered_node, GUI.stylesheet, PSEUDO_HOVER);
        GUI.awaiting_redraw = true;
    } 
    Vector_Free(&stack);

    // hovered node changed -> must redraw later
    if (GUI.prev_hovered_node != GUI.hovered_node) {
        GUI.awaiting_redraw = true;
    }


    // on mouse in event triggered
    if (GUI.hovered_node != NULL &&
        GUI.prev_hovered_node != GUI.hovered_node && 
        GUI.hovered_node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_IN)
    {
        TriggerOnMouseInEvent(&GUI.hovered_node->node, mouseX, mouseY);
    }

    // on mouse out event triggered
    if (GUI.prev_hovered_node != NULL && 
        GUI.prev_hovered_node != GUI.hovered_node && 
        GUI.prev_hovered_node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_OUT)
    {
        TriggerOnMouseOutEvent(&GUI.prev_hovered_node->node, mouseX, mouseY);
    }

    GUI.recalculate_mouse_hover = false;
}