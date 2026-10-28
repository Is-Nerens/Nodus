#pragma once

static inline float NODE_BORDER_PAD_WIDTH(const Node* node)
{
    return node->public.generic.borderLeft + node->public.generic.borderRight
         + node->public.generic.padLeft + node->public.generic.padRight;
}

static inline float NODE_BORDER_PAD_HEIGHT(const Node* node)
{
    return node->public.generic.borderTop + node->public.generic.borderBottom
         + node->public.generic.padTop + node->public.generic.padBottom;
}

static inline float NODE_INNER_WIDTH(const Node* node)
{
    return node->public.generic.width - (node->public.generic.borderLeft + node->public.generic.borderRight
         + node->public.generic.padLeft + node->public.generic.padRight);
}

static inline float NODE_INNER_HEIGHT(const Node* node)
{
    return node->public.generic.height - (node->public.generic.borderTop + node->public.generic.borderBottom
         + node->public.generic.padTop + node->public.generic.padBottom);
}

static inline float SCROLLBAR_TRACK_WIDTH()
{
    float thumbWidth = (float)GUI.stylesheet.scrollbarStyle.width - (float)GUI.stylesheet.scrollbarStyle.trackPadLeft - (float)GUI.stylesheet.scrollbarStyle.trackPadRight;
    if (thumbWidth < GUI.stylesheet.scrollbarStyle.thumbBorderLeft + GUI.stylesheet.scrollbarStyle.thumbBorderRight) {
        thumbWidth = GUI.stylesheet.scrollbarStyle.thumbBorderLeft + GUI.stylesheet.scrollbarStyle.thumbBorderRight;
    }
    if (thumbWidth < 2) thumbWidth = 2;
    return thumbWidth + (float)GUI.stylesheet.scrollbarStyle.trackPadLeft + (float)GUI.stylesheet.scrollbarStyle.trackPadRight;
}

static inline bool NODE_HAS_TEXT_CONTENT(const Node* node)
{
    return node->public.generic.textContent != NULL &&
    (node->type == NU_TYPE_BOX || node->type == NU_TYPE_BUTTON);
}

static inline bool NODE_CONTENTS_OVERFLOW_V(const Node* node)
{
    return node->public.generic.contentHeight > NODE_INNER_WIDTH(node);
}

static void NU_ApplyMinMaxWidthConstraint(Node* node)
{
    // Enforce max >= width >= min
    node->public.generic.width = min(max(node->public.generic.width, node->public.generic.minWidth), node->public.generic.maxWidth);

    // Enforce width >= preferred
    node->public.generic.width = max(node->public.generic.width, node->public.generic.preferredWidth);
}

static void NU_ApplyMinMaxHeightConstraint(Node* node)
{
    // Enforce max >= height >= min
    node->public.generic.height = min(max(node->public.generic.height, node->public.generic.minHeight), node->public.generic.maxHeight);

    // Enforce height >= preferred
    node->public.generic.height = max(node->public.generic.height, node->public.generic.preferredHeight);
}

static void NU_Prepass(BreadthFirstSearch* bfs, Array* scrollAutoNodes)
{
    Node* node;
    while (BreadthFirstSearch_Next(bfs, &node)) {

        // Reset state
        node->stateFlags = 0;
        node->clippedAncestor = NULL;

        // Set node hidden (if explicitly hidden OR inherent hiddeness from parent unless node is a window)
        if (node->layoutFlags & HIDDEN || (node->type != NU_TYPE_WINDOW && node->parent && NodeStateHidden(node->parent))) {
            node->stateFlags |= STATE_FLAG_HIDDEN; // Don't affect layout or draw node
            continue;
        }

        // Set / inherit absolute positioning
        if (node->layoutFlags & POSITION_ABSOLUTE || (node->parent && NodeStatePosAbsolute(node->parent))) {
            node->stateFlags |= STATE_FLAG_POS_ABSOLUTE;
        }

        // Add node to list of scroll auto nodes
        if (scrollAutoNodes != NULL && (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL)) {
            Array_Push(scrollAutoNodes, &node);
        }

        // Reset position and content size
        node->public.generic.x = 0.0f;
        node->public.generic.y = 0.0f;
        node->public.generic.contentWidth = 0.0f;
        node->public.generic.contentHeight = 0.0f;

        // Compute border + pad width / height
        float borderPadWidth = NODE_BORDER_PAD_WIDTH(node);
        float borderPadHeight = NODE_BORDER_PAD_HEIGHT(node);

        // Compute content width / height for special node types
        if (node->type == NU_TYPE_IMAGE)
        {
            // Get image dimensions
            u16 w, h;
            ImageResourceManager_GetImageSize(&GUI.imageResourceManager, node->special.image.imageHandle, &w, &h);

            const bool wIntrinsic = (node->special.image.widthDimType  == IMAGE_DIM_INTRINSIC);
            const bool hIntrinsic = (node->special.image.heightDimType == IMAGE_DIM_INTRINSIC);
            const bool wAspect    = (node->special.image.widthDimType  == IMAGE_DIM_ASPECT);
            const bool hAspect    = (node->special.image.heightDimType == IMAGE_DIM_ASPECT);
            const float aspect = (h > 0) ? ((float)w / (float)h) : 1.0f;

            float contentW = 0.0f;
            float contentH = 0.0f;

            // Both aspect? -> render both intrisic
            if (wAspect && hAspect) {
                contentW = (float)w;
                contentH = (float)h;
            }
            else {
                // Intrinsic axes come straight from the image.
                if (wIntrinsic) contentW = (float)w;
                if (hIntrinsic) contentH = (float)h;

                // Derrive aspect width / height from other (non aspect) dimension
                if (wAspect) {
                    float innerHeight = hIntrinsic ? (float)h : (node->public.generic.preferredHeight - borderPadHeight);
                    contentW = innerHeight * aspect;
                }
                if (hAspect) {
                    float innerWidth = wIntrinsic ? (float)w : (node->public.generic.preferredWidth - borderPadWidth);
                    contentH = innerWidth / aspect;
                }
            }
            node->public.generic.contentWidth  = contentW;
            node->public.generic.contentHeight = contentH;
        }
        else if (node->type == NU_TYPE_WINDOW) {
            int winWidth, winHeight;
            SDL_Window* window = DisplayManager_GetNodeSDLWindow(&GUI.displayManager, node);
            SDL_GetWindowSizeInPixels(window, &winWidth, &winHeight);
            node->public.generic.contentWidth = winWidth;
            node->public.generic.contentHeight = winHeight;
        }
        else if (NODE_HAS_TEXT_CONTENT(node)) {
            Font* font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontIdx);

            // Calculate text width -> drives content width
            float text_width = NU_Calculate_Text_Unwrapped_Width(font, node->public.generic.textContent);
            node->public.generic.contentWidth = text_width;
        }

        // Enforce constraint -> min >= borderAndPad
        node->public.generic.minWidth = max(node->public.generic.minWidth, borderPadWidth);
        node->public.generic.minHeight = max(node->public.generic.minHeight, borderPadHeight);

        // Enforce constraint -> max >= min
        node->public.generic.maxWidth = max(node->public.generic.maxWidth, node->public.generic.minWidth);
        node->public.generic.maxHeight = max(node->public.generic.maxHeight, node->public.generic.minHeight);

        // Enforce constraints -> pref >= min && pref <= max
        float clampedPrefWidth  = min(max(node->public.generic.preferredWidth,  node->public.generic.minWidth),  node->public.generic.maxWidth);
        float clampedPrefHeight = min(max(node->public.generic.preferredHeight, node->public.generic.minHeight), node->public.generic.maxHeight);

        // Compute initial width / height
        float initialWidth = max(clampedPrefWidth, node->public.generic.contentWidth + borderPadWidth);
        float initialHeight = max(clampedPrefHeight, node->public.generic.contentHeight + borderPadHeight);
        node->public.generic.width = initialWidth;
        node->public.generic.height = initialHeight;
    }
}

static void NU_PropagateChildWidthsToParents(ReverseBreadthFirstSearch* rbfs)
{
    Node* node;
    while (ReverseBreadthFirstSearch_Next(rbfs, &node)) {
        if (NodeStateHidden(node)) continue;

        int is_layout_horizontal = !(node->layoutFlags & LAYOUT_VERTICAL);

        // Calculate content width from children's widths
        int visibleChildren = 0;
        Node* child = node->firstChild;
        while(child)
        {
            // Update content width to account for child width
            if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW && !(child->layoutFlags & POSITION_ABSOLUTE)) {

                // Horizontal layout -> content width = sum(child widths)
                if (is_layout_horizontal) node->public.generic.contentWidth += child->public.generic.width;

                // Vertical layout -> content width = width of widest child
                else  {
                    node->public.generic.contentWidth = MAX(node->public.generic.contentWidth, child->public.generic.width);
                }
                visibleChildren++;
            }

            child = child->nextSibling; // Move to next child
        }

        // Horizontal layout with children -> add child gap to content width
        if (is_layout_horizontal && visibleChildren > 0) node->public.generic.contentWidth += (visibleChildren - 1) * node->public.generic.gap;

        // Vertical layout with children and overflow-v scroll -> reserve scrollbar width
        if (!is_layout_horizontal && visibleChildren > 0 && (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL)) {
            node->public.generic.contentWidth += SCROLLBAR_TRACK_WIDTH();
        }

        // Recalculate width to account for content width
        if (node->type != NU_TYPE_WINDOW && node->public.generic.contentWidth > node->public.generic.width) {
            node->public.generic.width = node->public.generic.contentWidth + NODE_BORDER_PAD_WIDTH(node);
            NU_ApplyMinMaxWidthConstraint(node);
        }
    }
}

static void NU_PropagateChildHeightsToParents(ReverseBreadthFirstSearch* rbfs)
{
    Node* node;
    while (ReverseBreadthFirstSearch_Next(rbfs, &node)) {
        if (NodeStateHidden(node)) continue;

        int is_layout_horizontal = !(node->layoutFlags & LAYOUT_VERTICAL);

        // Calculate content height from children's heights
        int visibleChildren = 0;
        Node* child = node->firstChild;
        while(child) {

            if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW && !(child->layoutFlags & POSITION_ABSOLUTE)) {

                // Horizontal layout -> content hwight = height of tallest child
                if (is_layout_horizontal) node->public.generic.contentHeight = MAX(node->public.generic.contentHeight, child->public.generic.height);

                // Vertical layout -> content height = sum(child heights)
                else node->public.generic.contentHeight += child->public.generic.height;
                visibleChildren++;
            }

            // Move to next child
            child = child->nextSibling;
        }

        // Vertical layout with children -> add child gap to content height
        if (!is_layout_horizontal && visibleChildren > 0) node->public.generic.contentHeight += (visibleChildren - 1) * node->public.generic.gap;

        // Recalculate height to account for content height
        if (node->type != NU_TYPE_WINDOW) {
            if (!(node->layoutFlags & OVERFLOW_VERTICAL_SCROLL)) {
                node->public.generic.height = node->public.generic.contentHeight + NODE_BORDER_PAD_HEIGHT(node);
            }
            NU_ApplyMinMaxHeightConstraint(node);
        }
    }
}

static void NU_GrowShrinkChildWidths(Node* node, float scrollbarThickness)
{
    float remainingWidth = node->public.generic.width - NODE_BORDER_PAD_WIDTH(node);
    remainingWidth -= !!(node->layoutFlags & OVERFLOW_VERTICAL_SCROLL) * scrollbarThickness;

    // ---------------------------------------------------------------------------------------
    // --- Expand widths of absolute elements if left and right distances are both defined ---
    // ---------------------------------------------------------------------------------------
    Node* child = node->firstChild;
    while (child) {

        if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
            child->layoutFlags & POSITION_ABSOLUTE &&
            child->public.generic.left >= 0.0f && child->public.generic.right >= 0.0f)
        {
            float expandedWidth = remainingWidth - child->public.generic.left - child->public.generic.right;
            if (expandedWidth > child->public.generic.width) child->public.generic.width = expandedWidth;
            NU_ApplyMinMaxWidthConstraint(child);
        }

        // move to the next child
        child = child->nextSibling;
    }

    // ------------------------------------------------
    // If node lays out children vertically ---------
    // ------------------------------------------------
    if (node->layoutFlags & LAYOUT_VERTICAL)
    {
        child = node->firstChild;
        while (child) {

            if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
                !(child->layoutFlags & POSITION_ABSOLUTE) &&
                child->layoutFlags & GROW_HORIZONTAL &&
                remainingWidth > child->public.generic.width)
            {
                child->public.generic.width = remainingWidth;
                NU_ApplyMinMaxWidthConstraint(child);
                node->public.generic.contentWidth = max(node->public.generic.contentWidth, child->public.generic.width);
            }

            // move to the next child
            child = child->nextSibling;
        }
    }

    // ------------------------------------------------
    // If node lays out children horizontally -------
    // ------------------------------------------------
    else
    {
        // ----------------------------------------------------
        // --- Calculate growable count and remaining width ---
        // ----------------------------------------------------
        int growable = 0;
        int visible = 0;
        child = node->firstChild;
        while (child) {

            if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW && !(child->layoutFlags & POSITION_ABSOLUTE))
            {
                remainingWidth -= child->public.generic.width;
                if (child->layoutFlags & GROW_HORIZONTAL && child->type != NU_TYPE_WINDOW) growable++;
                visible++;
            }

            // move to the next child
            child = child->nextSibling;
        }
        remainingWidth -= (visible - 1) * node->public.generic.gap;
        if (growable == 0) return;

        // -------------------------
        // --- Grow child widths ---
        // -------------------------
        while (remainingWidth > 0.01f)
        {
            // --------------------------------------------------------------
            // --- Determine smallest, second smallest and growable count ---
            // --------------------------------------------------------------
            float smallest = 1e20f;
            float secondSmallest = 1e30f;
            growable = 0;

            child = node->firstChild;
            while (child) {

                if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
                    !(child->layoutFlags & POSITION_ABSOLUTE) &&
                    (child->layoutFlags & GROW_HORIZONTAL) &&
                    child->public.generic.width < child->public.generic.maxWidth)
                {
                    growable++;
                    if (child->public.generic.width < smallest) {
                        secondSmallest = smallest;
                        smallest = child->public.generic.width;
                    } else if (child->public.generic.width < secondSmallest) {
                        secondSmallest = child->public.generic.width;
                    }
                }

                // move to the next child
                child = child->nextSibling;
            }

            // ----------------------------
            // --- Compute width to add ---
            // ----------------------------
            float width_to_add = remainingWidth / (float)growable;
            if (secondSmallest > smallest) {
                width_to_add = min(width_to_add, secondSmallest - smallest);
            }

            // -----------------------------------------
            // --- Grow width of each eligible child ---
            // -----------------------------------------
            bool grew_any = false;
            child = node->firstChild;
            while (child) {

                // if child is growable
                if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
                    !(child->layoutFlags & POSITION_ABSOLUTE) &&
                    child->layoutFlags & GROW_HORIZONTAL &&
                    child->public.generic.width < child->public.generic.maxWidth &&
                    child->public.generic.width == smallest)
                {
                    float available = child->public.generic.maxWidth - child->public.generic.width;
                    float grow = min(width_to_add, available);
                    if (grow > 0.0f) {
                        node->public.generic.contentWidth += grow;
                        child->public.generic.width += grow;
                        remainingWidth -= grow;
                        grew_any = true;
                    }
                }

                // move to the next child
                child = child->nextSibling;
            }
            if (!grew_any) break;
        }

        // ----------------------------------------
        // --- Shrink overgrown children (text) ---
        // ----------------------------------------
        while (remainingWidth < -0.01f)
        {
            // --------------------------------------------------------------
            // --- determine smallest, second smallest and shrinkable count ---
            // --------------------------------------------------------------
            float largest = -1e20f;
            float secondLargest = -1e30f;
            int shrinkableCount = 0;

            child = node->firstChild;
            while (child) {
                if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
                    !(child->layoutFlags & POSITION_ABSOLUTE) &&
                    (child->layoutFlags & GROW_HORIZONTAL) &&
                    child->public.generic.width > child->public.generic.minWidth)
                {
                    shrinkableCount++;
                    if (child->public.generic.width > largest) {
                        secondLargest = largest;
                        largest = child->public.generic.width;
                    } else if (child->public.generic.width > secondLargest) {
                        secondLargest = child->public.generic.width;
                    }
                }

                // move to the next child
                child = child->nextSibling;
            }

            // ---------------------------------
            // --- Compute width to subtract ---
            // ---------------------------------
            float width_to_subtract = -remainingWidth / (float)shrinkableCount;
            if (secondLargest < largest && secondLargest >= 0) {
                width_to_subtract = min(width_to_subtract, largest - secondLargest);
            }

            // -------------------------------------------
            // --- Shrink width of each eligible child ---
            // -------------------------------------------
            bool shrunk_any = false;
            child = node->firstChild;
            while (child) {
                if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
                    !(child->layoutFlags & POSITION_ABSOLUTE) &&
                    (child->layoutFlags & GROW_HORIZONTAL) &&
                    child->public.generic.width > child->public.generic.minWidth &&
                    child->public.generic.width == largest)
                {
                    float available = child->public.generic.width - child->public.generic.minWidth;
                    float shrink = min(width_to_subtract, available);
                    if (shrink > 0.0f) {
                        node->public.generic.contentWidth -= shrink;
                        child->public.generic.width -= shrink;
                        remainingWidth += shrink;
                        shrunk_any = true;
                    }
                }

                // move to the next child
                child = child->nextSibling;
            }
            if (!shrunk_any) break;
        }
    }
}

static void NU_GrowShrinkChildHeights(Node* node, float scrollbarThickness)
{
    float remainingHeight = node->public.generic.height - NODE_BORDER_PAD_HEIGHT(node);
    remainingHeight -= !!(node->layoutFlags & OVERFLOW_HORIZONTAL_SCROLL) * scrollbarThickness;

    // ----------------------------------------------------------------------------------------
    // --- Expand heights of absolute elements if top and bottom distances are both defined ---
    // ----------------------------------------------------------------------------------------
    Node* child = node->firstChild;
    while(child) {

        if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
            child->layoutFlags & POSITION_ABSOLUTE &&
            child->public.generic.top >= 0.0f && child->public.generic.bottom >= 0.0f)
        {
            float expandedHeight = remainingHeight - child->public.generic.top - child->public.generic.bottom;
            if (expandedHeight > child->public.generic.height) child->public.generic.height = expandedHeight;
            NU_ApplyMinMaxHeightConstraint(child);
        }

        // move to the next child
        child = child->nextSibling;
    }

    if (!(node->layoutFlags & LAYOUT_VERTICAL))
    {
        child = node->firstChild;
        while(child) {

            if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
                !(child->layoutFlags & POSITION_ABSOLUTE) &&
                child->layoutFlags & GROW_VERTICAL &&
                remainingHeight > child->public.generic.height)
            {
                child->public.generic.height = remainingHeight;
                NU_ApplyMinMaxHeightConstraint(child);
                node->public.generic.contentHeight = max(node->public.generic.contentHeight, child->public.generic.height);
            }

            // move to the next child
            child = child->nextSibling;
        }
    }
    else
    {
        // -----------------------------------------------------
        // --- Calculate growable count and remaining height ---
        // -----------------------------------------------------
        int growable = 0;
        int visible = 0;
        child = node->firstChild;
        while(child) {

            if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW && !(child->layoutFlags & POSITION_ABSOLUTE))
            {
                remainingHeight -= child->public.generic.height;
                if (child->layoutFlags & GROW_VERTICAL && child->type != NU_TYPE_WINDOW) growable++;
                visible++;
            }

            // move to the next child
            child = child->nextSibling;
        }
        remainingHeight -= (visible - 1) * node->public.generic.gap;
        if (growable == 0) return;

        // --------------------------
        // --- Grow child heights ---
        // --------------------------
        while (remainingHeight > 0.01f)
        {
            // --------------------------------------------------------------
            // --- Determine smallest, second smallest and growable count ---
            // --------------------------------------------------------------
            float smallest = 1e20f;
            float secondSmallest = 1e30f;
            growable = 0;

            child = node->firstChild;
            while(child) {

                if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
                    !(child->layoutFlags & POSITION_ABSOLUTE) &&
                    (child->layoutFlags & GROW_VERTICAL) &&
                    child->public.generic.height < child->public.generic.maxHeight)
                {
                    growable++;
                    if (child->public.generic.height < smallest) {
                        secondSmallest = smallest;
                        smallest = child->public.generic.height;
                    } else if (child->public.generic.height < secondSmallest) {
                        secondSmallest = child->public.generic.height;
                    }
                }

                // move to the next child
                child = child->nextSibling;
            }

            // -----------------------------
            // --- Compute height to add ---
            // -----------------------------
            float height_to_add = remainingHeight / (float)growable;
            if (secondSmallest > smallest) {
                height_to_add = min(height_to_add, secondSmallest - smallest);
            }

            // ------------------------------------------
            // --- Grow height of each eligible child ---
            // ------------------------------------------
            bool grew_any = false;
            child = node->firstChild;
            while(child) {

                if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
                    !(child->layoutFlags & POSITION_ABSOLUTE) &&
                    (child->layoutFlags & GROW_VERTICAL) &&
                    child->public.generic.height < child->public.generic.maxHeight &&
                    child->public.generic.height == smallest)
                {
                    float available = child->public.generic.maxHeight - child->public.generic.height;
                    float grow = min(height_to_add, available);
                    if (grow > 0.0f) {
                        node->public.generic.contentHeight += grow;
                        child->public.generic.height += grow;
                        remainingHeight -= grow;
                        grew_any = true;
                    }
                }

                // move to the next child
                child = child->nextSibling;
            }
            if (!grew_any) break;
        }
    }
}

static void NU_GrowShrinkWidths(BreadthFirstSearch* bfs, float scrollbarThickness)
{
    Node* node;
    while (BreadthFirstSearch_Next(bfs, &node)) {
        if (NodeStateHidden(node) || node->type == NU_TYPE_ROW || node->type == NU_TYPE_THEAD || node->type == NU_TYPE_TABLE) continue;
        NU_GrowShrinkChildWidths(node, scrollbarThickness);
    }
}

static void NU_GrowShrinkHeights(BreadthFirstSearch* bfs, float scrollbarThickness)
{
    Node* node;
    while (BreadthFirstSearch_Next(bfs, &node)) {
        if (NodeStateHidden(node) || node->type == NU_TYPE_TABLE) continue;
        NU_GrowShrinkChildHeights(node, scrollbarThickness);
    }
}

static void NU_CalculateTableColumnWidths(BreadthFirstSearch* bfs, DepthFirstSearch* dfs, float scrollbarThickness)
{
    Node* node;
    while(BreadthFirstSearch_Next(bfs, &node))
    {
        if (NodeStateHidden(node) || node->type != NU_TYPE_TABLE || node->childCount == 0) continue;

        float columnWidths[4096]; int columnWidthsCount = 0;

        // ------------------------------------------------------------
        // --- Calculate the widest cell width in each table column ---
        // ------------------------------------------------------------
        Node* row = node->firstChild;
        while(row != NULL)
        {
            if (NodeStateHidden(row)) { row = row->nextSibling; continue; } // Ignore hidden rows

            int cellIndex = 0;
            Node* cell = row->firstChild;
            while(cell != NULL)
            {
                if (NodeStateHidden(cell)) {
                    cell = cell->nextSibling;
                    cellIndex++;
                    continue;
                }

                // Ignore columns beyond our fixed stack buffer
                if (cellIndex >= 4096) { cell = cell->nextSibling; cellIndex++; continue; }

                // Initialise widest cell width to 0 (in array)
                if (cellIndex == columnWidthsCount) {
                    columnWidths[cellIndex] = 0;
                    columnWidthsCount++;
                }

                // Cell widths are unshrunk at this point (table pass owns them),
                // so this is the true intrinsic width.
                if (cell->public.generic.width > columnWidths[cellIndex]) columnWidths[cellIndex] = cell->public.generic.width;

                // Move to the next cell
                cellIndex++;
                cell = cell->nextSibling;
            }

            // Move to the next row
            row = row->nextSibling;
        }

        // -----------------------------------------------
        // --- Apply widest column widths to all cells ---
        // -----------------------------------------------
        float tableInnerWidth = node->public.generic.width - node->public.generic.borderLeft - node->public.generic.borderRight - node->public.generic.padLeft - node->public.generic.padRight;
        tableInnerWidth -= !!(node->layoutFlags & OVERFLOW_VERTICAL_SCROLL) * scrollbarThickness;
        float remainingTableInnerWidth = tableInnerWidth;
        for (int k=0; k<columnWidthsCount; k++) { remainingTableInnerWidth -= columnWidths[k]; }
        float used_table_width = tableInnerWidth - remainingTableInnerWidth;

        // Interate over all the rows in the table
        row = node->firstChild;
        while(row != NULL) {

            // Ignore hidden rows
            if (NodeStateHidden(row)) { row = row->nextSibling; continue; }

            // Calculate the row border + pad + gap
            float rowBorderPadGapWidth = NODE_BORDER_PAD_WIDTH(row);
            // Accumulate gap depending on how many visible children belong to the row
            if (row->public.generic.gap != 0.0f) {
                int visibleCells = 0;
                Node* cell = row->firstChild;
                while(cell != NULL) {
                    if (!NodeStateHidden(cell)) visibleCells++;
                    cell = cell->nextSibling;
                }
                if (visibleCells > 1) rowBorderPadGapWidth += row->public.generic.gap * (visibleCells - 1);
            }

            // Space available for cell content in this row (subtracted once, not per-cell)
            float availableForCells = tableInnerWidth - rowBorderPadGapWidth;
            float leftoverForCells = availableForCells - used_table_width;

            // Grow the width of all cells (and accumulate row width as we go)
            float rowUsedWidth = 0;
            int cellIndex = 0;
            int visibleCells = 0;
            Node* cell = row->firstChild;
            while(cell != NULL)
            {
                if (NodeStateHidden(cell)) { cell = cell->nextSibling; cellIndex++; continue; } // Skip hidden cells

                // Ignore columns beyond our fixed stack buffer
                if (cellIndex >= 4096) { cell = cell->nextSibling; cellIndex++; continue; }

                // Compute and set cell width
                float proportion = (used_table_width > 0.0f) ? (columnWidths[cellIndex] / used_table_width) : 0.0f;
                cell->public.generic.width = columnWidths[cellIndex] + leftoverForCells * proportion;

                // Never let a cell go below its column minimum
                if (cell->public.generic.width < columnWidths[cellIndex]) cell->public.generic.width = columnWidths[cellIndex];

                // Accumulate final row width
                rowUsedWidth += cell->public.generic.width;

                // Re-grow cell subtree
                Node* dfsNode;
                DepthFirstSearch_Reset(dfs, cell);
                while(DepthFirstSearch_Next(dfs, &dfsNode)) {
                    NU_GrowShrinkChildWidths(dfsNode, scrollbarThickness);
                }

                // Move to the next cell
                visibleCells++;
                cellIndex++;
                cell = cell->nextSibling;
            }

            // Set final row width from its laid-out cells
            row->public.generic.width = rowBorderPadGapWidth + rowUsedWidth;

            // Move to the next row
            row = row->nextSibling;
        }

        // ------------------------------------------------------------------
        // --- Recompute table width from final row/cell widths (post-layout) ---
        // ------------------------------------------------------------------
        // Column distribution may have grown cells past the original table
        // width. Recompute so the parent positions siblings using final sizes.
        float finalTableInnerWidth = 0.0f;
        row = node->lastChild;
        while(row != NULL) {
            if (NodeStateHidden(row)) { row = row->prevSibling; continue; }
            finalTableInnerWidth = row->public.generic.width;
            break;
        }

        float finalTableWidth = finalTableInnerWidth + NODE_BORDER_PAD_WIDTH(node);
        finalTableWidth += !!(node->layoutFlags & OVERFLOW_VERTICAL_SCROLL) * scrollbarThickness;
        node->public.generic.width = finalTableWidth;
    }
}

static void NU_CalculateTextHeights(BreadthFirstSearch* bfs)
{
    Node* node;
    while (BreadthFirstSearch_Next(bfs, &node)) {

        // Skip irrelevant nodes
        if (NodeStateHidden(node) || // Hidden
            node->public.generic.textContent == NULL || // No text content
            (node->type != NU_TYPE_BOX && node->type != NU_TYPE_BUTTON && node->type != NU_TYPE_INPUT)) { // Not a valid node type
            continue;
        }

        if (node->type == NU_TYPE_INPUT) {
            Font* node_font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontIdx);
            float borderPadHeight = NODE_BORDER_PAD_HEIGHT(node);

            // Set input content height equal to line height
            node->public.generic.contentHeight = node_font->line_height;

            // Recalculate height based on updated content height
            node->public.generic.height = max(node->public.generic.contentHeight + borderPadHeight, node->public.generic.preferredHeight);
        }
        else {
            Font* node_font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontIdx);

            // Calculate text height -> drive content height
            float textHeight = NU_Calculate_FreeText_Height_From_Wrap_Width(node_font, node->public.generic.textContent, node->public.generic.contentWidth);
            node->public.generic.contentHeight = textHeight;

            // Recalculate height based on updated content height
            node->public.generic.height = max(node->public.generic.contentHeight + NODE_BORDER_PAD_HEIGHT(node), node->public.generic.preferredHeight);
        }
    }
}

static void NU_PositionChildrenHorizontally(Node* node, float scrollbarThickness)
{
    // layout dir -> top to bottom
    if (node->layoutFlags & LAYOUT_VERTICAL)
    {
        Node* child = node->firstChild;
        while(child) {

            if (NodeStateHidden(child) || child->type == NU_TYPE_WINDOW) {
                child = child->nextSibling; continue;
            }

            if (!(child->layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float remaning_width = (node->public.generic.width - node->public.generic.padLeft - node->public.generic.padRight - node->public.generic.borderLeft - node->public.generic.borderRight) - child->public.generic.width;
                float x_align_offset = remaning_width * 0.5f * (float)node->horizontalAlignment;
                child->public.generic.x = node->public.generic.x + node->public.generic.padLeft + node->public.generic.borderLeft + x_align_offset;
            }
            else { // position absolute
                child->public.generic.x = node->public.generic.x + node->public.generic.padLeft + node->public.generic.borderLeft;
                if (child->public.generic.left != INT16_MIN) {
                    child->public.generic.x = node->public.generic.x + child->public.generic.left + node->public.generic.padLeft + node->public.generic.borderLeft;
                }
                else if (child->public.generic.right != INT16_MIN) {
                    float inner_width = node->public.generic.width - node->public.generic.padLeft - node->public.generic.padRight - node->public.generic.borderLeft - node->public.generic.borderRight;
                    child->public.generic.x = node->public.generic.x + inner_width - child->public.generic.width - child->public.generic.right;
                }
            }

            // move to the next child
            child = child->nextSibling;
        }
    }
    // layout dir -> left to right
    else
    {
        // calculate remaining width (optimise this by caching this value inside node's content width variable)
        float remainingWidth = NODE_INNER_WIDTH(node);
        remainingWidth -= !!(node->layoutFlags & OVERFLOW_VERTICAL_SCROLL) * scrollbarThickness;

        int numChildrenAffectingWidth = 0;
        Node* child = node->firstChild;
        while(child) {
            if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW && !(child->layoutFlags & POSITION_ABSOLUTE)) {
                remainingWidth -= child->public.generic.width; numChildrenAffectingWidth++;
            }

            // move to the next child
            child = child->nextSibling;
        }
        remainingWidth -= node->public.generic.gap * (numChildrenAffectingWidth - 1);


        // position children horizontally
        float cursorX = 0.0f;
        child = node->firstChild;
        while(child) {
            if (NodeStateHidden(child) || child->type == NU_TYPE_WINDOW) {
                child = child->nextSibling; continue;
            }

            if (!(child->layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float x_align_offset = remainingWidth * 0.5f * (float)node->horizontalAlignment;
                child->public.generic.x += node->public.generic.x + node->public.generic.padLeft + node->public.generic.borderLeft + cursorX + x_align_offset;
                cursorX += child->public.generic.width + node->public.generic.gap;
            }
            else { // position absolute
                child->public.generic.x = node->public.generic.x + node->public.generic.padLeft + node->public.generic.borderLeft;
                if (child->public.generic.left != INT16_MIN) {
                    child->public.generic.x = node->public.generic.x + child->public.generic.left + node->public.generic.padLeft + node->public.generic.borderLeft;
                }
                else if (child->public.generic.right != INT16_MIN) {
                    float inner_width = node->public.generic.width - node->public.generic.padLeft - node->public.generic.padRight - node->public.generic.borderLeft - node->public.generic.borderRight;
                    child->public.generic.x = node->public.generic.x + inner_width - child->public.generic.width - child->public.generic.right;
                }
            }

            // move to the next child
            child = child->nextSibling;
        }
    }
}

static void NU_PositionChildrenVertically(Node* node, float scrollbarThickness)
{
    float y_scroll_offset = 0.0f;
    if ((node->layoutFlags & OVERFLOW_VERTICAL_SCROLL) && NODE_CONTENTS_OVERFLOW_V(node))
    {
        float track_h = node->public.generic.height - node->public.generic.borderTop - node->public.generic.borderBottom;
        float inner_height_w_pad = track_h - node->public.generic.padTop - node->public.generic.padBottom;
        float inner_proportion_of_content_height = inner_height_w_pad / node->public.generic.contentHeight;
        float thumb_h = inner_proportion_of_content_height * track_h;
        float content_scroll_range = node->public.generic.contentHeight - inner_height_w_pad;
        float thumb_scroll_range = track_h - thumb_h;
        float scroll_factor = content_scroll_range / max(thumb_scroll_range, 1.0f);
        y_scroll_offset += (-node->public.generic.scrollV * (track_h - thumb_h)) * scroll_factor;

        // undo effect of scroll offset for table header row
        if (node->firstChild->type == NU_TYPE_THEAD) {
            node->firstChild->public.generic.y -= y_scroll_offset;
        }
    }

    // layout dir -> left to right
    if (!(node->layoutFlags & LAYOUT_VERTICAL))
    {
        Node* child = node->firstChild;
        while(child) {

            if (NodeStateHidden(child) || child->type == NU_TYPE_WINDOW) {
                child = child->nextSibling; continue;
            }

            if (!(child->layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float remaining_height = (NODE_INNER_HEIGHT(node)) - child->public.generic.height;
                float y_align_offset = remaining_height * 0.5f * (float)node->verticalAlignment;
                child->public.generic.y += node->public.generic.y + node->public.generic.padTop + node->public.generic.borderTop + y_align_offset + y_scroll_offset;
            }
            else { // position absolute
                child->public.generic.y = node->public.generic.y + node->public.generic.padTop + node->public.generic.borderTop;
                if (child->public.generic.top > 0.0f) {
                    child->public.generic.y = node->public.generic.y + child->public.generic.top + node->public.generic.padTop + node->public.generic.borderTop;
                }
                else if (child->public.generic.bottom > 0.0f) {
                    float inner_height = node->public.generic.height - node->public.generic.padTop - node->public.generic.padBottom - node->public.generic.borderTop - node->public.generic.borderBottom;
                    child->public.generic.y = node->public.generic.y + inner_height - child->public.generic.height - child->public.generic.bottom;
                }
            }

            // move to the next child
            child = child->nextSibling;
        }

    }
    // layout dir -> top to bottom
    else
    {
        // calculate remaining height (optimise this by caching this value inside node's content height variable)
        float remainingHeight = NODE_INNER_HEIGHT(node);
        remainingHeight -= !!(node->layoutFlags & OVERFLOW_HORIZONTAL_SCROLL) * scrollbarThickness;
        int numChildrenAffectingHeight = 0;
        Node* child = node->firstChild;
        while(child) {
            if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW && !(child->layoutFlags & POSITION_ABSOLUTE)) {
                remainingHeight -= child->public.generic.height; numChildrenAffectingHeight++;
            }

            // move to the next child
            child = child->nextSibling;
        }
        remainingHeight -= node->public.generic.gap * (numChildrenAffectingHeight - 1);

        // position children vertically
        float cursorY = 0.0f;
        child = node->firstChild;
        while(child) {

            if (NodeStateHidden(child) || child->type == NU_TYPE_WINDOW) {
                child = child->nextSibling; continue;
            }

            if (!(child->layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float y_align_offset = remainingHeight * 0.5f * (float)node->verticalAlignment;
                child->public.generic.y += node->public.generic.y + node->public.generic.padTop + node->public.generic.borderTop + cursorY + y_align_offset + y_scroll_offset;
                cursorY += child->public.generic.height + node->public.generic.gap;
            }
            else { // position abosolute
                child->public.generic.y = node->public.generic.y + node->public.generic.padTop + node->public.generic.borderTop;
                if (child->public.generic.top >= 0.0f) {
                    child->public.generic.y = node->public.generic.y + child->public.generic.top + node->public.generic.padTop + node->public.generic.borderTop;
                }
                else if (child->public.generic.bottom >= 0.0f) {
                    float inner_height = node->public.generic.height - node->public.generic.padTop - node->public.generic.padBottom - node->public.generic.borderTop - node->public.generic.borderBottom;
                    child->public.generic.y = node->public.generic.y + inner_height - child->public.generic.height - child->public.generic.bottom;
                }
            }

            // move to the next child
            child = child->nextSibling;
        }
    }
}

static void NU_CalculatePositions(BreadthFirstSearch* bfs, float scrollbarThickness)
{
    Node* node;
    while (BreadthFirstSearch_Next(bfs, &node)) {
        if (NodeStateHidden(node)) continue;
        NU_PositionChildrenHorizontally(node, scrollbarThickness);
        NU_PositionChildrenVertically(node, scrollbarThickness);
    }
}

static void NU_Repass(BreadthFirstSearch* bfs)
{
    Node* node;
    while (BreadthFirstSearch_Next(bfs, &node)) {

        if (NodeStateHidden(node)) continue;

        // Reset content size
        node->public.generic.contentWidth = 0.0f;
        node->public.generic.contentHeight = 0.0f;

        Node* child = node->firstChild;
        while(child)
        {
            if (NodeStateHidden(child)) {
                child = child->nextSibling;
                continue;
            }

            // Compute border + pad width / height
            float borderPadWidth = child->public.generic.borderLeft + child->public.generic.borderRight + child->public.generic.padLeft + child->public.generic.padRight;
            float borderPadHeight = child->public.generic.borderTop + child->public.generic.borderBottom + child->public.generic.padTop + child->public.generic.padBottom;

            // Determine if node has text content
            bool hasTextContent = child->public.generic.textContent != NULL && (child->type == NU_TYPE_BOX || child->type == NU_TYPE_BUTTON);

            // Compute content width / height for special node types
            if (child->type == NU_TYPE_IMAGE)
            {
                // Get image dimensions
                u16 w, h;
                ImageResourceManager_GetImageSize(&GUI.imageResourceManager, child->special.image.imageHandle, &w, &h);

                const bool wIntrinsic = (child->special.image.widthDimType  == IMAGE_DIM_INTRINSIC);
                const bool hIntrinsic = (child->special.image.heightDimType == IMAGE_DIM_INTRINSIC);
                const bool wAspect    = (child->special.image.widthDimType  == IMAGE_DIM_ASPECT);
                const bool hAspect    = (child->special.image.heightDimType == IMAGE_DIM_ASPECT);
                const float aspect = (h > 0) ? ((float)w / (float)h) : 1.0f;

                float contentW = 0.0f;
                float contentH = 0.0f;

                // Both aspect? -> render both intrisic
                if (wAspect && hAspect) {
                    contentW = (float)w;
                    contentH = (float)h;
                }
                else {
                    // Intrinsic axes come straight from the image.
                    if (wIntrinsic) contentW = (float)w;
                    if (hIntrinsic) contentH = (float)h;

                    // Derrive aspect width / height from other (non aspect) dimension
                    if (wAspect) {
                        float innerHeight = hIntrinsic ? (float)h : (child->public.generic.preferredHeight - borderPadHeight);
                        contentW = innerHeight * aspect;
                    }
                    if (hAspect) {
                        float innerWidth = wIntrinsic ? (float)w : (child->public.generic.preferredWidth - borderPadWidth);
                        contentH = innerWidth / aspect;
                    }
                }
                child->public.generic.contentWidth  = contentW;
                child->public.generic.contentHeight = contentH;
            }
            else if (child->type == NU_TYPE_WINDOW) {
                int winWidth, winHeight;
                SDL_Window* window = DisplayManager_GetNodeSDLWindow(&GUI.displayManager, child);
                SDL_GetWindowSizeInPixels(window, &winWidth, &winHeight);
                child->public.generic.contentWidth = winWidth;
                child->public.generic.contentHeight = winHeight;
            }
            else if (hasTextContent) {
                Font* font = Stylesheet_Get_Font(&GUI.stylesheet, child->fontIdx);

                // Calculate text width -> drives content width
                float text_width = NU_Calculate_Text_Unwrapped_Width(font, child->public.generic.textContent);
                child->public.generic.contentWidth = text_width;
            }

            // Enforce constraints -> pref >= min && pref <= max
            float clampedPrefWidth  = min(max(child->public.generic.preferredWidth,  child->public.generic.minWidth),  child->public.generic.maxWidth);
            float clampedPrefHeight = min(max(child->public.generic.preferredHeight, child->public.generic.minHeight), child->public.generic.maxHeight);

            // Compute initial width / height
            float initialWidth = max(clampedPrefWidth, child->public.generic.contentWidth + borderPadWidth);
            float initialHeight = max(clampedPrefHeight, child->public.generic.contentHeight + borderPadHeight);
            child->public.generic.width = initialWidth;
            child->public.generic.height = initialHeight;

            // move to next sibling
            child = child->nextSibling;
        }
    }
}

void NU_Layout()
{
    // Reset traversal datastructures
    DepthFirstSearch* dfs = &GUI.dfs;
    BreadthFirstSearch* bfs = &GUI.bfs;
    ReverseBreadthFirstSearch* rbfs = &GUI.rbfs;
    DepthFirstSearch_Reset(dfs, GUI.tree.root);
    BreadthFirstSearch_Reset(bfs, GUI.tree.root);
    ReverseBreadthFirstSearch_Reset(rbfs, GUI.tree.root);
    Array_Clear(&GUI.layoutScrollAutoNodes);

    printf("\nserial layout time: ");
    timer_start();

    // Primary iteration
    NU_Prepass(bfs, &GUI.layoutScrollAutoNodes);
    NU_PropagateChildWidthsToParents(rbfs);
    NU_GrowShrinkWidths(bfs, 0.0f);
    NU_CalculateTableColumnWidths(bfs, dfs, 0.0f);
    NU_CalculateTextHeights(bfs);
    NU_PropagateChildHeightsToParents(rbfs);
    NU_GrowShrinkHeights(bfs, 0.0f);
    NU_CalculatePositions(bfs, 0.0f);

    // // Compute thumb-constrained track width
    float trackWidth = SCROLLBAR_TRACK_WIDTH();

    // SECOND PASS -> RECOMPUTE OVERFLOWED SCROLL BRANCHES
    for (u32 i=0; i<GUI.layoutScrollAutoNodes.size; i++)
    {
        Node* node = *(Node**)Array_Get(&GUI.layoutScrollAutoNodes, i);

        // Recompute branch with scrollbar track included
        if (NODE_CONTENTS_OVERFLOW_V(node)) {
            BreadthFirstSearch_Reset(bfs, node);
            ReverseBreadthFirstSearch_Reset(rbfs, node);
            NU_Prepass(bfs, NULL);
            NU_PropagateChildWidthsToParents(rbfs);
            NU_GrowShrinkWidths(bfs, trackWidth);
            NU_CalculateTableColumnWidths(bfs, dfs, trackWidth);
            NU_CalculateTextHeights(bfs);
            NU_PropagateChildHeightsToParents(rbfs);
            NU_GrowShrinkHeights(bfs, trackWidth);
            NU_CalculatePositions(bfs, trackWidth);
        }
    }

    timer_stop();
}
