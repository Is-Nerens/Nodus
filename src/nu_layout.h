#include <freetype/freetype.h>
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <stdbool.h>
#include <math.h>
#include <utils/performance.h>
#include <text/nu_text_layout.h>

static void NU_ApplyMinMaxWidthConstraint(Node* node)
{
    node->width = min(max(node->width, node->public.generic.minWidth), node->public.generic.maxWidth);
    node->width = max(node->width, node->public.generic.preferredWidth);
}

static void NU_ApplyMinMaxHeightConstraint(Node* node)
{
    node->height = min(max(node->height, node->public.generic.minHeight), node->public.generic.maxHeight);
    node->height = max(node->height, node->public.generic.preferredHeight);
}

static void NU_Prepass(BreadthFirstSearch* bfs, Array* scrollAutoNodes)
{
    Node* node;
    while (BreadthFirstSearch_Next(bfs, &node)) {

        // Reset state
        node->stateFlags = 0;

        // Set node hidden (if explicitly hidden OR inherent hiddeness from parent unless node is a window)
        if (node->layoutFlags & HIDDEN || (node->type != NU_TYPE_WINDOW && node->parent != NULL && NodeStateHidden(node->parent))) {
            node->stateFlags |= STATE_FLAG_HIDDEN; // Don't affect layout or draw node
        }
        // Node affects layout
        else {
            node->clippedAncestor = NULL;

            // Set / inherit absolute positioning
            if (node->layoutFlags & POSITION_ABSOLUTE ||
                (node->parent != NULL && NodeStatePosAbsolute(node->parent)))
            {
                node->stateFlags |= STATE_FLAG_POS_ABSOLUTE;
            }

            // Add node to list of scroll auto nodes
            if (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL) {
                Array_Push(scrollAutoNodes, &node);
            }

            // Reset position
            node->x = 0.0f;
            node->y = 0.0f;

            // Compute natural
            float natural_width = node->public.generic.borderLeft + node->public.generic.borderRight + node->public.generic.padLeft + node->public.generic.padRight;
            float natural_height = node->public.generic.borderTop + node->public.generic.borderBottom + node->public.generic.padTop + node->public.generic.padBottom;

            // Enforce constraint -> max > min
            node->public.generic.maxWidth = max(node->public.generic.maxWidth, node->public.generic.minWidth);
            node->public.generic.maxHeight = max(node->public.generic.maxHeight, node->public.generic.minHeight);

            // Enforce constraints -> pref >= min
            float clampedPrefWidth  = min(max(node->public.generic.preferredWidth,  node->public.generic.minWidth),  node->public.generic.maxWidth);
            float clampedPrefHeight = min(max(node->public.generic.preferredHeight, node->public.generic.minHeight), node->public.generic.maxHeight);

            // Set base, enforce constraint -> base >= natural
            node->width = max(clampedPrefWidth, natural_width);
            node->height = max(clampedPrefHeight, natural_height);

            // Reset
            node->contentHeight = 0.0f;
            node->contentHeight = 0.0f;
        }
    }
}

static void NU_CalculateTextFitWidths(BreadthFirstSearch* bfs)
{
    Node* node;
    while (BreadthFirstSearch_Next(bfs, &node)) {

        // Filter out
        if (NodeStateHidden(node) || // Hidden
            node->public.generic.textContent == NULL || // No text content
            (node->type != NU_TYPE_WINDOW && node->type != NU_TYPE_BOX && node->type != NU_TYPE_BUTTON)) { // Not a valid node type
            continue;
        }

        NU_Font* node_font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontId);

        // Calculate text width & height
        float text_width = NU_Calculate_Text_Unwrapped_Width(node_font, node->public.generic.textContent);

        // Calculate minimum text wrap width (longest unbreakable word)
        float min_wrap_width = NU_Calculate_Text_Min_Wrap_Width(node_font, node->public.generic.textContent);

        // Increase width to account for text (text height will be accounted for later in NU_CalculateTextHeights())
        float natural_width = node->public.generic.padLeft + node->public.generic.padRight + node->public.generic.borderLeft + node->public.generic.borderRight;
        node->width = max(text_width + natural_width, node->public.generic.preferredWidth);

        // Update content width
        node->contentHeight = text_width;
    }
}

static void NU_CalculateFitSizeWidths(ReverseBreadthFirstSearch* rbfs)
{
    Node* node;
    while (ReverseBreadthFirstSearch_Next(rbfs, &node)) {
        if (NodeStateHidden(node)) continue;

        int is_layout_horizontal = !(node->layoutFlags & LAYOUT_VERTICAL);

        // If node is a window -> set dimensions equal to window
        if (node->type == NU_TYPE_WINDOW) {
            int winWidth, winHeight;
            SDL_Window* window = GetSDL_Window(&GUI.winManager, node->windowID);
            SDL_GetWindowSizeInPixels(window, &winWidth, &winHeight);
            node->width = (float)winWidth;
            node->height = (float)winHeight;
        }

        // Calculate content width from children's widths
        int visibleChildren = 0;
        Node* child = node->firstChild;
        while(child != NULL) {

            if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW && !(child->layoutFlags & POSITION_ABSOLUTE)) {
                if (is_layout_horizontal) node->contentHeight += child->width;
                else node->contentHeight = MAX(node->contentHeight, child->width);
                visibleChildren++;
            }

            // move to next child
            child = child->nextSibling;
        }

        // Expand width to account for content width
        if (is_layout_horizontal && visibleChildren > 0) node->contentHeight += (visibleChildren - 1) * node->public.generic.gap;
        if (node->type != NU_TYPE_WINDOW && node->contentHeight > node->width) {
            node->width = node->contentHeight + node->public.generic.borderLeft + node->public.generic.borderRight + node->public.generic.padLeft + node->public.generic.padRight;
            NU_ApplyMinMaxWidthConstraint(node);
        }
    }
}

static void NU_CalculateFitSizeHeights(ReverseBreadthFirstSearch* rbfs)
{
    Node* node;
    while (ReverseBreadthFirstSearch_Next(rbfs, &node)) {
        if (NodeStateHidden(node)) continue;

        int is_layout_horizontal = !(node->layoutFlags & LAYOUT_VERTICAL);

        // Calculate content height from children's heights
        int visibleChildren = 0;
        Node* child = node->firstChild;
        while(child != NULL) {

            if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW && !(child->layoutFlags & POSITION_ABSOLUTE)) {
                if (is_layout_horizontal) node->contentHeight = MAX(node->contentHeight, child->height);
                else node->contentHeight += child->height;
                visibleChildren++;
            }

            // move to next child
            child = child->nextSibling;
        }

        // Expand node height to account for content height
        if (!is_layout_horizontal && visibleChildren > 0) node->contentHeight += (visibleChildren - 1) * node->public.generic.gap;
        if (node->type != NU_TYPE_WINDOW) {
            if (!(node->layoutFlags & OVERFLOW_VERTICAL_SCROLL)) node->height = node->contentHeight + node->public.generic.borderTop + node->public.generic.borderBottom + node->public.generic.padTop + node->public.generic.padBottom;
            NU_ApplyMinMaxHeightConstraint(node);
        }
    }
}

static void NU_GrowShrinkChildWidths(Node* node, float scrollbarThickness)
{
    float remainingWidth = node->width - node->public.generic.padLeft - node->public.generic.padRight - node->public.generic.borderLeft - node->public.generic.borderRight;
    remainingWidth -= !!(node->layoutFlags & OVERFLOW_VERTICAL_SCROLL) * scrollbarThickness;

    // ---------------------------------------------------------------------------------------
    // --- Expand widths of absolute elements if left and right distances are both defined ---
    // ---------------------------------------------------------------------------------------
    Node* child = node->firstChild;
    while (child != NULL) {

        if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
            child->layoutFlags & POSITION_ABSOLUTE &&
            child->public.generic.left >= 0.0f && child->public.generic.right >= 0.0f)
        {
            float expandedWidth = remainingWidth - child->public.generic.left - child->public.generic.right;
            if (expandedWidth > child->width) child->width = expandedWidth;
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
        while (child != NULL) {

            if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
                !(child->layoutFlags & POSITION_ABSOLUTE) &&
                child->layoutFlags & GROW_HORIZONTAL &&
                remainingWidth > child->width)
            {
                child->width = remainingWidth;
                NU_ApplyMinMaxWidthConstraint(child);
                node->contentHeight = max(node->contentHeight, child->width);
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
        while (child != NULL) {

            if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW && !(child->layoutFlags & POSITION_ABSOLUTE))
            {
                remainingWidth -= child->width;
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
            while (child != NULL) {

                if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
                    !(child->layoutFlags & POSITION_ABSOLUTE) &&
                    (child->layoutFlags & GROW_HORIZONTAL) &&
                    child->width < child->public.generic.maxWidth)
                {
                    growable++;
                    if (child->width < smallest) {
                        secondSmallest = smallest;
                        smallest = child->width;
                    } else if (child->width < secondSmallest) {
                        secondSmallest = child->width;
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
            while (child != NULL) {

                // if child is growable
                if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
                    !(child->layoutFlags & POSITION_ABSOLUTE) &&
                    child->layoutFlags & GROW_HORIZONTAL &&
                    child->width < child->public.generic.maxWidth &&
                    child->width == smallest)
                {
                    float available = child->public.generic.maxWidth - child->width;
                    float grow = min(width_to_add, available);
                    if (grow > 0.0f) {
                        node->contentHeight += grow;
                        child->width += grow;
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
            while (child != NULL) {
                if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
                    !(child->layoutFlags & POSITION_ABSOLUTE) &&
                    (child->layoutFlags & GROW_HORIZONTAL) &&
                    child->width > child->public.generic.minWidth)
                {
                    shrinkableCount++;
                    if (child->width > largest) {
                        secondLargest = largest;
                        largest = child->width;
                    } else if (child->width > secondLargest) {
                        secondLargest = child->width;
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
            while (child != NULL) {
                if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
                    !(child->layoutFlags & POSITION_ABSOLUTE) &&
                    (child->layoutFlags & GROW_HORIZONTAL) &&
                    child->width > child->public.generic.minWidth &&
                    child->width == largest)
                {
                    float available = child->width - child->public.generic.minWidth;
                    float shrink = min(width_to_subtract, available);
                    if (shrink > 0.0f) {
                        node->contentHeight -= shrink;
                        child->width -= shrink;
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
    float remainingHeight = node->height - node->public.generic.padTop - node->public.generic.padBottom - node->public.generic.borderTop - node->public.generic.borderBottom;
    remainingHeight -= !!(node->layoutFlags & OVERFLOW_HORIZONTAL_SCROLL) * scrollbarThickness;

    // ----------------------------------------------------------------------------------------
    // --- Expand heights of absolute elements if top and bottom distances are both defined ---
    // ----------------------------------------------------------------------------------------
    Node* child = node->firstChild;
    while(child != NULL) {

        if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
            child->layoutFlags & POSITION_ABSOLUTE &&
            child->public.generic.top >= 0.0f && child->public.generic.bottom >= 0.0f)
        {
            float expandedHeight = remainingHeight - child->public.generic.top - child->public.generic.bottom;
            if (expandedHeight > child->height) child->height = expandedHeight;
            NU_ApplyMinMaxHeightConstraint(child);
        }

        // move to the next child
        child = child->nextSibling;
    }

    if (!(node->layoutFlags & LAYOUT_VERTICAL))
    {
        child = node->firstChild;
        while(child != NULL) {

            if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
                !(child->layoutFlags & POSITION_ABSOLUTE) &&
                child->layoutFlags & GROW_VERTICAL &&
                remainingHeight > child->height)
            {
                child->height = remainingHeight;
                NU_ApplyMinMaxHeightConstraint(child);
                node->contentHeight = max(node->contentHeight, child->height);
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
        while(child != NULL) {

            if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW && !(child->layoutFlags & POSITION_ABSOLUTE))
            {
                remainingHeight -= child->height;
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
            while(child != NULL) {

                if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
                    !(child->layoutFlags & POSITION_ABSOLUTE) &&
                    (child->layoutFlags & GROW_VERTICAL) &&
                    child->height < child->public.generic.maxHeight)
                {
                    growable++;
                    if (child->height < smallest) {
                        secondSmallest = smallest;
                        smallest = child->height;
                    } else if (child->height < secondSmallest) {
                        secondSmallest = child->height;
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
            while(child != NULL) {

                if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW &&
                    !(child->layoutFlags & POSITION_ABSOLUTE) &&
                    (child->layoutFlags & GROW_VERTICAL) &&
                    child->height < child->public.generic.maxHeight &&
                    child->height == smallest)
                {
                    float available = child->public.generic.maxHeight - child->height;
                    float grow = min(height_to_add, available);
                    if (grow > 0.0f) {
                        node->contentHeight += grow;
                        child->height += grow;
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
        if (NodeStateHidden(node) || node->type == NU_TYPE_ROW || node->type == NU_TYPE_TABLE) continue;
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

static void NU_CalculateTableColumnWidths(BreadthFirstSearch* bfs, float scrollbarThickness)
{
    DepthFirstSearch flexWidthDFS = DepthFirstSearch_Reserve();

    Node* node;
    while(BreadthFirstSearch_Next(bfs, &node))
    {
        if (NodeStateHidden(node) || node->type != NU_TYPE_TABLE || node->childCount == 0) continue;

        float columnWidths[4096]; int columnWidthsCount = 0;

        // ------------------------------------------------------------
        // --- Calculate the widest cell width in each table column ---
        // ------------------------------------------------------------
        Node* row = node->firstChild;
        while(row != NULL) {

            // Ignore hidden rows
            if (NodeStateHidden(row)) { row = row->nextSibling; continue; }

            int cellIndex = 0;
            Node* cell = row->firstChild;
            while(cell != NULL) {

                if (NodeStateHidden(cell)) {
                    cell = cell->nextSibling; continue;
                }

                // Initialise widest cell width to 0 (in array)
                if (cellIndex == columnWidthsCount) {
                    columnWidths[cellIndex] = 0;
                    columnWidthsCount++;
                }

                // Get current column width and update if cell is wider
                if (cell->width > columnWidths[cellIndex]) columnWidths[cellIndex] = cell->width;

                cellIndex++;
                cell = cell->nextSibling;
            }

            row = row->nextSibling;
        }

        // -----------------------------------------------
        // --- Apply widest column widths to all cells ---
        // -----------------------------------------------
        float tableInnerWidth = node->width - node->public.generic.borderLeft - node->public.generic.borderRight - node->public.generic.padLeft - node->public.generic.padRight;
        tableInnerWidth -= !!(node->layoutFlags & OVERFLOW_VERTICAL_SCROLL) * scrollbarThickness;
        float remainingTableInnerWidth = tableInnerWidth;
        for (int k=0; k<columnWidthsCount; k++) { remainingTableInnerWidth -= columnWidths[k]; }
        float used_table_width = tableInnerWidth - remainingTableInnerWidth;

        // Interate over all the rows in the table
        row = node->firstChild;
        while(row != NULL) {

            // Ignore hidden rows
            if (NodeStateHidden(row)) { row = row->nextSibling; continue; }

            row->width = tableInnerWidth;

            // Reduce available growth space by acounting for row pad, border and child gaps
            float row_border_pad_gap = row->public.generic.borderLeft + row->public.generic.borderRight + row->public.generic.padLeft + row->public.generic.padRight;
            if (row->public.generic.gap != 0.0f) {
                int visibleCells = 0;

                // iterate over cells in row
                Node* cell = row->firstChild;
                while(cell != NULL) {
                    if (NodeStateHidden(cell)) visibleCells++;
                    cell = cell->nextSibling;
                }
                row_border_pad_gap += row->public.generic.gap * (visibleCells - 1);
            }

            // Grow the width of all cells
            int cellIndex = 0;
            Node* cell = row->firstChild;
            while(cell != NULL) {

                // Ignore hidden cells
                if (NodeStateHidden(cell)) { cell = cell->nextSibling; continue; }

                // Compute and set cell width
                float proportion = columnWidths[cellIndex] / (used_table_width);
                cell->width = columnWidths[cellIndex] + (remainingTableInnerWidth - row_border_pad_gap) * proportion;

                // Grow subtree in cell
                Node* dfsNode;
                DepthFirstSearch_Reset(&flexWidthDFS, cell);
                while(DepthFirstSearch_Next(&flexWidthDFS, &dfsNode)) {
                    NU_GrowShrinkChildWidths(dfsNode, scrollbarThickness);
                }

                // Move to the next cell
                cellIndex++;
                cell = cell->nextSibling;
            }

            // Move to the next row
            row = row->nextSibling;
        }
    }

    DepthFirstSearch_Free(&flexWidthDFS);
}

static void NU_CalculateTextHeights(BreadthFirstSearch* bfs)
{
    Node* node;
    while (BreadthFirstSearch_Next(bfs, &node)) {

        // Filter out
        if (NodeStateHidden(node) || node->type == NU_TYPE_FRAME) continue;
        if (NodeStateHidden(node) || // Hidden
            node->public.generic.textContent == NULL || // No text content
            (node->type != NU_TYPE_WINDOW && node->type != NU_TYPE_BOX && node->type != NU_TYPE_BUTTON && node->type != NU_TYPE_INPUT)) { // Not a valid node type
            continue;
        }

        if (node->type == NU_TYPE_INPUT) {
            NU_Font* node_font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontId);

            // Set input height equal to line height
            node->height = node_font->line_height +
            node->public.generic.padTop + node->public.generic.padBottom +
            node->public.generic.borderTop + node->public.generic.borderBottom;
            node->contentHeight = node_font->line_height;
        }
        else {
            NU_Font* node_font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontId);

            // Compute available inner width
            float inner_width = node->width - node->public.generic.borderLeft - node->public.generic.borderRight - node->public.generic.padLeft - node->public.generic.padRight;

            // Calculate text height
            float text_height = NU_Calculate_FreeText_Height_From_Wrap_Width(node_font, node->public.generic.textContent, inner_width);

            // Increase height to account for text
            float natural_height = node->public.generic.padTop + node->public.generic.padBottom + node->public.generic.borderTop + node->public.generic.borderBottom;
            node->height = max(text_height + natural_height, node->public.generic.preferredHeight);

            // Update content height
            node->contentHeight = text_height;
        }
    }
}

static void NU_PositionChildrenHorizontally(Node* node, float scrollbarThickness)
{
    // layout dir -> top to bottom
    if (node->layoutFlags & LAYOUT_VERTICAL)
    {
        Node* child = node->firstChild;
        while(child != NULL) {

            if (NodeStateHidden(child) || child->type == NU_TYPE_WINDOW) {
                child = child->nextSibling; continue;
            }

            if (!(child->layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float remaning_width = (node->width - node->public.generic.padLeft - node->public.generic.padRight - node->public.generic.borderLeft - node->public.generic.borderRight) - child->width;
                float x_align_offset = remaning_width * 0.5f * (float)node->horizontalAlignment;
                child->x = node->x + node->public.generic.padLeft + node->public.generic.borderLeft + x_align_offset;
            }
            else { // position absolute
                child->x = node->x + node->public.generic.padLeft + node->public.generic.borderLeft;
                if (child->public.generic.left != INT16_MIN) {
                    child->x = node->x + child->public.generic.left + node->public.generic.padLeft + node->public.generic.borderLeft;
                }
                else if (child->public.generic.right != INT16_MIN) {
                    float inner_width = node->width - node->public.generic.padLeft - node->public.generic.padRight - node->public.generic.borderLeft - node->public.generic.borderRight;
                    child->x = node->x + inner_width - child->width - child->public.generic.right;
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
        float remainingWidth = node->width - node->public.generic.padLeft - node->public.generic.padRight - node->public.generic.borderLeft - node->public.generic.borderRight;
        remainingWidth -= !!(node->layoutFlags & OVERFLOW_VERTICAL_SCROLL) * scrollbarThickness;

        int numChildrenAffectingWidth = 0;
        Node* child = node->firstChild;
        while(child != NULL) {
            if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW && !(child->layoutFlags & POSITION_ABSOLUTE)) {
                remainingWidth -= child->width; numChildrenAffectingWidth++;
            }

            // move to the next child
            child = child->nextSibling;
        }
        remainingWidth -= node->public.generic.gap * (numChildrenAffectingWidth - 1);


        // position children horizontally
        float cursorX = 0.0f;
        child = node->firstChild;
        while(child != NULL) {
            if (NodeStateHidden(child) || child->type == NU_TYPE_WINDOW) {
                child = child->nextSibling; continue;
            }

            if (!(child->layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float x_align_offset = remainingWidth * 0.5f * (float)node->horizontalAlignment;
                child->x += node->x + node->public.generic.padLeft + node->public.generic.borderLeft + cursorX + x_align_offset;
                cursorX += child->width + node->public.generic.gap;
            }
            else { // position absolute
                child->x = node->x + node->public.generic.padLeft + node->public.generic.borderLeft;
                if (child->public.generic.left != INT16_MIN) {
                    child->x = node->x + child->public.generic.left + node->public.generic.padLeft + node->public.generic.borderLeft;
                }
                else if (child->public.generic.right != INT16_MIN) {
                    float inner_width = node->width - node->public.generic.padLeft - node->public.generic.padRight - node->public.generic.borderLeft - node->public.generic.borderRight;
                    child->x = node->x + inner_width - child->width - child->public.generic.right;
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
    if (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL &&
        node->childCount > 0 &&
        node->contentHeight > node->height - node->public.generic.padTop - node->public.generic.padBottom - node->public.generic.borderTop - node->public.generic.borderBottom)
    {
        float track_h = node->height - node->public.generic.borderTop - node->public.generic.borderBottom;
        float inner_height_w_pad = track_h - node->public.generic.padTop - node->public.generic.padBottom;
        float inner_proportion_of_content_height = inner_height_w_pad / node->contentHeight;
        float thumb_h = inner_proportion_of_content_height * track_h;
        float content_scroll_range = node->contentHeight - inner_height_w_pad;
        float thumb_scroll_range = track_h - thumb_h;
        float scroll_factor = content_scroll_range / max(thumb_scroll_range, 1.0f);
        y_scroll_offset += (-node->scrollV * (track_h - thumb_h)) * scroll_factor;

        // undo effect of scroll offset for table header row
        if (node->firstChild->type == NU_TYPE_THEAD) {
            node->firstChild->y -= y_scroll_offset;
        }
    }

    // layout dir -> left to right
    if (!(node->layoutFlags & LAYOUT_VERTICAL))
    {
        Node* child = node->firstChild;
        while(child != NULL) {

            if (NodeStateHidden(child) || child->type == NU_TYPE_WINDOW) {
                child = child->nextSibling; continue;
            }

            if (!(child->layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float remaining_height = (node->height - node->public.generic.padTop - node->public.generic.padBottom - node->public.generic.borderTop - node->public.generic.borderBottom) - child->height;
                float y_align_offset = remaining_height * 0.5f * (float)node->verticalAlignment;
                child->y += node->y + node->public.generic.padTop + node->public.generic.borderTop + y_align_offset + y_scroll_offset;
            }
            else { // position absolute
                child->y = node->y + node->public.generic.padTop + node->public.generic.borderTop;
                if (child->public.generic.top > 0.0f) {
                    child->y = node->y + child->public.generic.top + node->public.generic.padTop + node->public.generic.borderTop;
                }
                else if (child->public.generic.bottom > 0.0f) {
                    float inner_height = node->height - node->public.generic.padTop - node->public.generic.padBottom - node->public.generic.borderTop - node->public.generic.borderBottom;
                    child->y = node->y + inner_height - child->height - child->public.generic.bottom;
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
        float remainingHeight = (node->height - node->public.generic.padTop - node->public.generic.padBottom - node->public.generic.borderTop - node->public.generic.borderBottom);
        remainingHeight -= !!(node->layoutFlags & OVERFLOW_HORIZONTAL_SCROLL) * scrollbarThickness;
        int numChildrenAffectingHeight = 0;
        Node* child = node->firstChild;
        while(child != NULL) {
            if (!NodeStateHidden(child) && child->type != NU_TYPE_WINDOW && !(child->layoutFlags & POSITION_ABSOLUTE)) {
                remainingHeight -= child->height; numChildrenAffectingHeight++;
            }

            // move to the next child
            child = child->nextSibling;
        }
        remainingHeight -= node->public.generic.gap * (numChildrenAffectingHeight - 1);

        // position children vertically
        float cursorY = 0.0f;
        child = node->firstChild;
        while(child != NULL) {
            if (NodeStateHidden(child) || child->type == NU_TYPE_WINDOW) {
                child = child->nextSibling; continue;
            }

            if (!(child->layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float y_align_offset = remainingHeight * 0.5f * (float)node->verticalAlignment;
                child->y += node->y + node->public.generic.padTop + node->public.generic.borderTop + cursorY + y_align_offset + y_scroll_offset;
                cursorY += child->height + node->public.generic.gap;
            }
            else { // position abosolute
                child->y = node->y + node->public.generic.padTop + node->public.generic.borderTop;
                if (child->public.generic.top >= 0.0f) {
                    child->y = node->y + child->public.generic.top + node->public.generic.padTop + node->public.generic.borderTop;
                }
                else if (child->public.generic.bottom >= 0.0f) {
                    float inner_height = node->height - node->public.generic.padTop - node->public.generic.padBottom - node->public.generic.borderTop - node->public.generic.borderBottom;
                    child->y = node->y + inner_height - child->height - child->public.generic.bottom;
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
        if (node->type == NU_TYPE_WINDOW) {
            node->x = 0;
            node->y = 0;
        }
        NU_PositionChildrenHorizontally(node, scrollbarThickness);
        NU_PositionChildrenVertically(node, scrollbarThickness);
    }
}

void NU_Repass(BreadthFirstSearch* bfs)
{
    Node* node;
    while (BreadthFirstSearch_Next(bfs, &node)) {

        if (NodeStateHidden(node)) continue;

        node->contentHeight = 0.0f;
        node->contentHeight = 0.0f;

        Node* child = node->firstChild;
        while(child != NULL) {

            if (NodeStateHidden(child)) {
                child = child->nextSibling;
                continue;
            }

            // reset position
            child->x = 0.0f;
            child->y = 0.0f;

            // Set base width/height and reset content dimensions
            float natural_width = child->public.generic.borderLeft + child->public.generic.borderRight + child->public.generic.padLeft + child->public.generic.padRight;
            float natural_height = child->public.generic.borderTop + child->public.generic.borderBottom + child->public.generic.padTop + child->public.generic.padBottom;
            child->width = max(child->public.generic.preferredWidth, natural_width);
            child->height = max(child->public.generic.preferredHeight, natural_height);
            child->contentWidth = 0;
            child->contentHeight = 0;

            // move to next sibling
            child = child->nextSibling;
        }
    }
}

void NU_Layout()
{
    // RESET TRAVERSAL DATA STRUCTURES
    BreadthFirstSearch* bfs = &GUI.bfs;
    ReverseBreadthFirstSearch* rbfs = &GUI.rbfs;
    BreadthFirstSearch_Reset(bfs, GUI.tree.root);
    ReverseBreadthFirstSearch_Reset(rbfs, GUI.tree.root);

    // RESERVE LIST OF AUTO SCROLL NODES
    Array_Clear(&GUI.layoutScrollAutoNodes);

    // FIRST PASS -> ASSUME SCROLLBARS TAKE UP NO SPACE
    NU_Prepass(bfs, &GUI.layoutScrollAutoNodes);
    NU_CalculateTextFitWidths(bfs);
    NU_CalculateFitSizeWidths(rbfs);
    NU_GrowShrinkWidths(bfs, 0.0f);
    NU_CalculateTableColumnWidths(bfs, 0.0f);
    NU_CalculateTextHeights(bfs);
    NU_CalculateFitSizeHeights(rbfs);
    NU_GrowShrinkHeights(bfs, 0.0f);
    NU_CalculatePositions(bfs, 0.0f);

    // Compute scrollbar thickness
    float thumbWidth = (float)GUI.stylesheet.scrollbarStyle.width - (float)GUI.stylesheet.scrollbarStyle.trackPadLeft - (float)GUI.stylesheet.scrollbarStyle.trackPadRight;

    // Constrain thumb width by thumb border
    if (thumbWidth < GUI.stylesheet.scrollbarStyle.thumbBorderLeft + GUI.stylesheet.scrollbarStyle.thumbBorderRight) {
        thumbWidth = GUI.stylesheet.scrollbarStyle.thumbBorderLeft + GUI.stylesheet.scrollbarStyle.thumbBorderRight;
    }

    // Ensure absolute minimum thumb width of 2px
    if (thumbWidth < 2) thumbWidth = 2;

    // Compute thumb-constrained track width
    float trackWidth = thumbWidth + (float)GUI.stylesheet.scrollbarStyle.trackPadLeft + (float)GUI.stylesheet.scrollbarStyle.trackPadRight;

    // SECOND PASS -> RECOMPUTE OVERFLOWED SCROLL BRANCHES
    for (u32 i=0; i<GUI.layoutScrollAutoNodes.size; i++)
    {
        Node* node = *(Node**)Array_Get(&GUI.layoutScrollAutoNodes, i);
        bool overflowed = node->contentHeight > (node->height - node->public.generic.padTop - node->public.generic.padBottom - node->public.generic.borderTop - node->public.generic.borderBottom);
        if (!overflowed) continue;

        // PERFORM NECESSARY COMPUTATIONS ONLY
        BreadthFirstSearch_Reset(bfs, node);
        ReverseBreadthFirstSearch_Reset(rbfs, node);
        NU_Repass(bfs);
        NU_CalculateTextFitWidths(bfs);
        NU_CalculateFitSizeWidths(rbfs);
        NU_GrowShrinkWidths(bfs, trackWidth);
        NU_CalculateTableColumnWidths(bfs, trackWidth);
        NU_CalculateTextHeights(bfs);
        NU_CalculateFitSizeHeights(rbfs);
        NU_GrowShrinkHeights(bfs, trackWidth);
        NU_CalculatePositions(bfs, trackWidth);
    }
}
