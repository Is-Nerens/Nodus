#pragma once

static bool NU_MouseIsOverNode(NodeP* node, float mouseX, float mouseY)
{
    // get clipping info
    float left_wall = node->node.x;
    float right_wall = node->node.x + node->node.width;
    float top_wall = node->node.y;
    float bottom_wall = node->node.y + node->node.height; 
    if (node->clippingRootHandle != UINT32_MAX) {
        NU_ClipBounds* clip = HashmapGet(&__NGUI.winManager.clipMap, &node->clippingRootHandle);
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
    float scroll_thumb_top_wall = node->node.y + node->node.borderTop + (node->node.scrollV * (track_height - thumb_height));
    bool within_x_bound = mouse_x >= scroll_thumb_left_wall && mouse_x <= scroll_thumb_left_wall + 8.0f;
    bool within_y_bound = mouse_y >= scroll_thumb_top_wall && mouse_y <= scroll_thumb_top_wall + thumb_height;
    return within_x_bound && within_y_bound;
}

void NU_Mouse_Hover()
{   
    // remove potential pseudo style from current hovered node
    if (__NGUI.hovered_node != UINT32_MAX && __NGUI.hovered_node != __NGUI.mouse_down_node) {
        NU_Apply_Stylesheet_To_Node(NODE_P(__NGUI.hovered_node), __NGUI.stylesheet);
    }
    uint32_t prev_hovered_node = __NGUI.hovered_node;
    __NGUI.hovered_node = UINT32_MAX;
    __NGUI.scroll_hovered_node = UINT32_MAX;
    if (__NGUI.winManager.hoveredWindow == NULL) return;

    // get local mouse coords
    float mouseX, mouseY; GetLocalMouseCoords(&__NGUI.winManager, &mouseX, &mouseY);

    // create a traversal stack
    struct Vector stack;
    Vector_Reserve(&stack, sizeof(NodeP*), 32);

    // add absolute root nodes to stack
    for (uint32_t i=0; i<__NGUI.winManager.absoluteRootNodes.size; i++) {
        NodeP* absolute_node = *(NodeP**)Vector_Get(&__NGUI.winManager.absoluteRootNodes, i);
        if (absolute_node->node.window == __NGUI.winManager.hoveredWindow && NU_MouseIsOverNode(absolute_node, mouseX, mouseY)) {
            Vector_Push(&stack, &absolute_node);
        }
    }

    // add window root nodes to stack
    for (uint32_t i=0; i<__NGUI.winManager.windowNodes.size; i++) {
        uint32_t handle = *(uint32_t*)Vector_Get(&__NGUI.winManager.windowNodes, i);
        NodeP* node = NODE_P(handle);
        if (node->node.window == __NGUI.winManager.hoveredWindow){
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
        __NGUI.hovered_node = current_node->handle;
        stack.size -= 1;

        // skip children
        if (current_node->type == BUTTON) continue;

        // iterate over children
        Layer* childlayer = &__NGUI.tree.layers[current_node->layer+1];
        for (uint32_t i=current_node->firstChildIndex; i<current_node->firstChildIndex + current_node->childCount; i++)
        {
            NodeP* child = LayerGet(childlayer, i);
            if (child->state == 2 || 
                child->node.layoutFlags & POSITION_ABSOLUTE || 
                child->type == WINDOW ||
                !NU_MouseIsOverNode(child, mouseX, mouseY)) continue; // Skip

            // check for scroll hover
            if (child->node.layoutFlags & OVERFLOW_V_PROPERTY) {
                bool overflow_v = child->node.contentHeight > child->node.height - child->node.borderTop - child->node.borderBottom;
                if (overflow_v) {
                    __NGUI.scroll_hovered_node = child->handle;
                }
            }
            Vector_Push(&stack, &child);
        }
    }

    // apply pseudo style to hovered node
    if (__NGUI.hovered_node != UINT32_MAX && __NGUI.hovered_node != __NGUI.mouse_down_node) {
        NU_Apply_Pseudo_Style_To_Node(NODE_P(__NGUI.hovered_node), __NGUI.stylesheet, PSEUDO_HOVER);
    } 
    Vector_Free(&stack);

    // hovered node changed -> must redraw later
    if (prev_hovered_node != __NGUI.hovered_node) {
        __NGUI.awaiting_redraw = true;
    }
}