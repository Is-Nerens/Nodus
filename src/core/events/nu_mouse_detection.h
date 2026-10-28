#pragma once

static bool NU_MouseIsOverNode(Node* node, float mouseX, float mouseY)
{
    // get clipping info
    float left_wall = node->public.generic.x;
    float right_wall = node->public.generic.x + node->public.generic.width;
    float top_wall = node->public.generic.y;
    float bottom_wall = node->public.generic.y + node->public.generic.height;
    if (node->clippedAncestor) {
        NU_ClipBounds* clip = Hashmap_Get(&GUI.displayManager.clipMap, &node->clippedAncestor);
        left_wall = max(clip->left, node->public.generic.x);
        right_wall = min(clip->right, node->public.generic.x + node->public.generic.width);
        top_wall = max(clip->top, node->public.generic.y);
        bottom_wall = min(clip->bottom, node->public.generic.y + node->public.generic.height);
    }

    // check if mouse is within clipped bounding box
    bool within_x_bound = mouseX >= left_wall && mouseX <= right_wall;
    bool within_y_bound = mouseY >= top_wall && mouseY <= bottom_wall;
    if (!(within_x_bound && within_y_bound)) return false; // Not in bounding rect

    // constrain border radii
    float borderRadiusBl = node->public.generic.borderRadiusBl;
    float borderRadiusBr = node->public.generic.borderRadiusBr;
    float borderRadiusTl = node->public.generic.borderRadiusTl;
    float borderRadiusTr = node->public.generic.borderRadiusTr;
    float left_radii_sum   = borderRadiusTl + borderRadiusBl;
    float right_radii_sum  = borderRadiusTr + borderRadiusBr;
    float top_radii_sum    = borderRadiusTl + borderRadiusTr;
    float bottom_radii_sum = borderRadiusBl + borderRadiusBr;
    if (left_radii_sum   > node->public.generic.height)  { float scale = node->public.generic.height / left_radii_sum;   borderRadiusTl *= scale; borderRadiusBl *= scale; }
    if (right_radii_sum  > node->public.generic.height)  { float scale = node->public.generic.height / right_radii_sum;  borderRadiusTr *= scale; borderRadiusBr *= scale; }
    if (top_radii_sum    > node->public.generic.width )  { float scale = node->public.generic.width  / top_radii_sum;    borderRadiusTl *= scale; borderRadiusTr *= scale; }
    if (bottom_radii_sum > node->public.generic.width )  { float scale = node->public.generic.width  / bottom_radii_sum; borderRadiusBl *= scale; borderRadiusBr *= scale; }

    // rounded border anchors
    vec2 tl_a = { floorf(node->public.generic.x + borderRadiusTl),                    floorf(node->public.generic.y + borderRadiusTl) };
    vec2 tr_a = { floorf(node->public.generic.x + node->public.generic.width - borderRadiusTr), floorf(node->public.generic.y + borderRadiusTr) };
    vec2 bl_a = { floorf(node->public.generic.x + borderRadiusBl),                    floorf(node->public.generic.y + node->public.generic.height - borderRadiusBl) };
    vec2 br_a = { floorf(node->public.generic.x + node->public.generic.width - borderRadiusBr), floorf(node->public.generic.y + node->public.generic.height - borderRadiusBr) };

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
    Node* node, Stylesheet_Scrollbar_Style* scrollbarStyle,
    float mouseX, float mouseY, float* grabOffsetOut
)
{
    NuNodeGenericAccess* n = &node->public.generic;

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
    float trackHeight = node->public.generic.height - n->borderTop - n->borderBottom;
    float usableTrackHeight = trackHeight - scrollbarStyle->trackPadTop - scrollbarStyle->trackPadBottom;

    // Compute track pos
    float trackX = node->public.generic.x + node->public.generic.width - n->borderRight - trackWidth;
    float trackY = node->public.generic.y + n->borderTop;

    // Compute scroll transform values
    float scrollContentHeight = node->public.generic.contentHeight;
    float scrollViewHeight = usableTrackHeight - n->padTop - n->padBottom;
    float scrollScaleFactor = scrollViewHeight / scrollContentHeight;

    // Compute thumb pos and constrained height
    float thumbHeight = fmaxf(scrollViewHeight / node->public.generic.contentHeight * usableTrackHeight, scrollbarStyle->thumbMinSize);
    float scrollTravel = usableTrackHeight - thumbHeight;
    float thumbY = trackY + scrollbarStyle->trackPadTop + node->public.generic.scrollV * scrollTravel;
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
    if (!GUI.displayManager.hoveredWindowNode) return;

    // Get local mouse coords
    float mouseX, mouseY; GetLocalMouseCoords(&GUI.displayManager, &mouseX, &mouseY);

    // Create a traversal stack
    struct Array stack;
    Array_Init(&stack, sizeof(Node*), 32);

    // Add absolute root nodes to stack
    for (u32 i=0; i<GUI.displayManager.absoluteRootNodes.size; i++) {
        Node* absolute_node = *(Node**)Array_Get(&GUI.displayManager.absoluteRootNodes, i);
        if (absolute_node->windowID == GUI.displayManager.hoveredWindowNode->windowID && NU_MouseIsOverNode(absolute_node, mouseX, mouseY)) {
            Array_Push(&stack, &absolute_node);
        }
    }

    // Add hovered window node to stack
    Array_Push(&stack, &GUI.displayManager.hoveredWindowNode);

    // traverse the tree
    bool break_loop = false;
    while (stack.size > 0 && !break_loop)
    {
        // Pop the stack
        Node* current_node = *(Node**)Array_Get(&stack, stack.size - 1);
        if (!(current_node->layoutFlags & IGNORE_MOUSE)) {
            GUI.hovered_node = current_node;
        }
        stack.size -= 1;

        // Skip children
        if (current_node->type == NU_TYPE_BUTTON) continue;

        // Iterate over children
        Node* child = current_node->firstChild;
        while(child) {

            if (NodeStateHidden(child) ||
                child->layoutFlags & POSITION_ABSOLUTE ||
                child->type == NU_TYPE_WINDOW ||
                !NU_MouseIsOverNode(child, mouseX, mouseY))
            {
                child = child->nextSibling; continue;
            }

            // Check for scroll hover
            if ((child->layoutFlags & OVERFLOW_VERTICAL_SCROLL)) {
                if (NODE_CONTENTS_OVERFLOW_V(child)) GUI.scroll_hovered_node = child;
            }
            Array_Push(&stack, &child);

            // Move to the next child
            child = child->nextSibling;
        }
    }
    Array_Free(&stack);

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
        if (GUI.prev_hovered_node != NULL && GUI.prev_hovered_node != GUI.mouse_down_node && GUI.prev_hovered_node != GUI.focused_node) Stylesheet_ApplyStyleToNode(&GUI.stylesheet, GUI.prev_hovered_node);
        if (GUI.hovered_node != GUI.mouse_down_node && GUI.hovered_node != GUI.focused_node) Stylesheet_ApplyPseudoStyleToNode(&GUI.stylesheet, GUI.hovered_node, PSEUDO_HOVER);
        GUI.awaiting_redraw = true;
    }
}
