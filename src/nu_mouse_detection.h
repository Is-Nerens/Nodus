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
        left_wall = max(clip->left, node->node.x);
        right_wall = min(clip->right, node->node.x + node->node.width);
        top_wall = max(clip->top, node->node.y);
        bottom_wall = min(clip->bottom, node->node.y + node->node.height);
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
        float dx = mouseX - tl_a.x;
        float dy = mouseY - tl_a.y;
        float dist2 = dx * dx + dy * dy;
        if (dist2 > borderRadiusTl * borderRadiusTl) return false;
    }

    // ensure mouse is not in top right rounded deadzone
    if (mouseX > tr_a.x && mouseY < tr_a.y) {
        float dx = mouseX - tr_a.x;
        float dy = mouseY - tr_a.y;
        float dist2 = dx * dx + dy * dy;
        if (dist2 > borderRadiusTr * borderRadiusTr) return false;
    }

    // ensure mouse is not in bottom left rounded deadzone
    if (mouseX < bl_a.x && mouseY > bl_a.y) {
        float dx = mouseX - bl_a.x;
        float dy = mouseY - bl_a.y;
        float dist2 = dx * dx + dy * dy;
        if (dist2 > borderRadiusBl * borderRadiusBl) return false;
    }

    // ensure mouse is not in bottom right rounded deadzone
    if (mouseX > br_a.x && mouseY > br_a.y) {
        float dx = mouseX - br_a.x;
        float dy = mouseY - br_a.y;
        float dist2 = dx * dx + dy * dy;
        if (dist2 > borderRadiusBr * borderRadiusBr) return false;
    }

    return true;
}

static bool NU_Mouse_Over_Node_V_Scrollbar(
    NodeP* node, NU_Stylesheet_Scrollbar_Style* scrollbarStyle, 
    float mouseX, float mouseY, float* grabOffsetOut
) 
{
    Node* n = &node->node;

    // --------------------------------------
    // --- Compute constrained dimensions ---
    // --------------------------------------
    float thumbWidth = (float)scrollbarStyle->width - (float)scrollbarStyle->trackPadLeft - (float)scrollbarStyle->trackPadRight;

    // Constrain thumb width by thumb border
    if (thumbWidth < scrollbarStyle->thumbBorderLeft + scrollbarStyle->thumbBorderRight) {
        thumbWidth = scrollbarStyle->thumbBorderLeft + scrollbarStyle->thumbBorderRight;
    }

    // Ensure absolute minimum thumb width of 2px
    if (thumbWidth < 2) thumbWidth = 2;

    // Compute thumb-constrained track width
    float trackWidth = thumbWidth + (float)scrollbarStyle->trackPadLeft + (float)scrollbarStyle->trackPadRight;

    // Compute track height
    float trackHeight = n->height - n->borderTop - n->borderBottom;
    float usableTrackHeight = trackHeight - scrollbarStyle->trackPadTop - scrollbarStyle->trackPadBottom;

    // Compute track pos
    float trackX = n->x + n->width - n->borderRight - trackWidth;
    float trackY = n->y + n->borderTop;

    // Compute scroll transform values
    float scrollContentHeight = n->contentHeight;
    float scrollViewHeight = usableTrackHeight - n->padTop - n->padBottom; 
    float scrollScaleFactor = scrollViewHeight / scrollContentHeight;

    // Compute thumb pos and constrained height
    float thumbHeight = fmaxf(scrollViewHeight / n->contentHeight * usableTrackHeight, scrollbarStyle->thumbMinSize);
    float scrollTravel = usableTrackHeight - thumbHeight;
    float thumbY = trackY + scrollbarStyle->trackPadTop + node->scrollV * scrollTravel;
    float thumbX = trackX + scrollbarStyle->trackPadLeft;

    // Output grab offset
    *grabOffsetOut = mouseY - thumbY;

    // Determine result
    bool withinX = mouseX >= thumbX && mouseX <= thumbX + thumbWidth;
    bool withinY = mouseY >= thumbY && mouseY <= thumbY + thumbHeight;
    return withinX && withinY;
}

void NU_Mouse_Hover()
{   
    GUI.prev_hovered_node = GUI.hovered_node;
    GUI.hovered_node = NULL;
    GUI.scroll_hovered_node = NULL;
    if (GUI.winManager.hoveredWindowID == -1) return;

    // Get local mouse coords
    float mouseX, mouseY; GetLocalMouseCoords(&GUI.winManager, &mouseX, &mouseY);

    // Create a traversal stack
    struct Array stack;
    ArrayInit(&stack, sizeof(NodeP*), 32);

    // Add absolute root nodes to stack
    for (u32 i=0; i<GUI.winManager.absoluteRootNodes.size; i++) {
        NodeP* absolute_node = *(NodeP**)ArrayGet(&GUI.winManager.absoluteRootNodes, i);
        if (absolute_node->windowID == GUI.winManager.hoveredWindowID && NU_MouseIsOverNode(absolute_node, mouseX, mouseY)) {
            ArrayPush(&stack, &absolute_node);
        }
    }

    // Add window root nodes to stack
    for (u32 i=0; i<GUI.winManager.windowNodes.size; i++) {
        NodeP* node = *(NodeP**)ArrayGet(&GUI.winManager.windowNodes, i);
        if (node->windowID == GUI.winManager.hoveredWindowID){
            ArrayPush(&stack, &node);
            break;
        }
    }

    // traverse the tree
    bool break_loop = false;
    while (stack.size > 0 && !break_loop) 
    {
        // Pop the stack
        NodeP* current_node = *(NodeP**)ArrayGet(&stack, stack.size - 1);
        if (!(current_node->layoutFlags & IGNORE_MOUSE)) {
            GUI.hovered_node = current_node;
        }
        stack.size -= 1;

        // Skip children
        if (current_node->type == NU_BUTTON) continue;

        // Iterate over children
        NodeP* child = current_node->firstChild;
        while(child != NULL) {

            if (NodeStateHidden(child) || 
                child->layoutFlags & POSITION_ABSOLUTE || 
                child->type == NU_WINDOW ||
                !NU_MouseIsOverNode(child, mouseX, mouseY))
            {
                child = child->nextSibling; continue;
            }

            // Check for scroll hover
            if (child->layoutFlags & OVERFLOW_V_PROPERTY) {
                bool overflow_v = child->node.contentHeight > child->node.height - child->node.borderTop - child->node.borderBottom;
                if (overflow_v) GUI.scroll_hovered_node = child;
            }
            ArrayPush(&stack, &child);

            // Move to the next child
            child = child->nextSibling;
        }
    }
    ArrayFree(&stack);

    // On mouse in event triggered
    if (GUI.hovered_node != NULL &&
        GUI.prev_hovered_node != GUI.hovered_node && 
        GUI.hovered_node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_IN)
    {
        TriggerOnMouseInEvent(GUI.hovered_node, mouseX, mouseY);
    }

    // On mouse out event triggered
    if (GUI.prev_hovered_node != NULL && 
        GUI.prev_hovered_node != GUI.hovered_node && 
        GUI.prev_hovered_node->eventFlags & NU_EVENT_FLAG_ON_MOUSE_OUT)
    {
        TriggerOnMouseOutEvent(GUI.prev_hovered_node, mouseX, mouseY);
    }

    // Apply hover pseudo style
    if (GUI.hovered_node != GUI.prev_hovered_node) {
        if (GUI.prev_hovered_node != NULL && GUI.prev_hovered_node != GUI.mouse_down_node && GUI.prev_hovered_node != GUI.focused_node) NU_Apply_Stylesheet_To_Node(GUI.prev_hovered_node, &GUI.stylesheet);
        if (GUI.hovered_node != GUI.mouse_down_node && GUI.hovered_node != GUI.focused_node) NU_Apply_Pseudo_Style_To_Node(GUI.hovered_node, &GUI.stylesheet, PSEUDO_HOVER);
        GUI.awaiting_redraw = true;
    }
}