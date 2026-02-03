#include <omp.h>
#include <freetype/freetype.h>
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <stdbool.h>
#include <math.h>
#include <utils/performance.h>
#include <rendering/text/nu_text_layout.h>

static void NU_ApplyMinMaxWidthConstraint(NodeP* node)
{
    node->node.width = min(max(node->node.width, node->node.minWidth), node->node.maxWidth);
    node->node.width = max(node->node.width, node->node.preferred_width);
}

static void NU_ApplyMinMaxHeightConstraint(NodeP* node)
{
    node->node.height = min(max(node->node.height, node->node.minHeight), node->node.maxHeight);
    node->node.height = max(node->node.height, node->node.preferred_height);
}

static void NU_Prepass()
{
    DepthFirstSearch dfs = DepthFirstSearch_Create(__NGUI.tree.root);
    NodeP* node;
    while (DepthFirstSearch_Next(&dfs, &node)) {

        // set node hidden
        node->state = 1;
        if (node->node.layoutFlags & HIDDEN || (node->parent != NULL && node->parent->node.layoutFlags & HIDDEN)) {
            node->state = 2; // don't affect layout or draw node
        }
        // node affects layout
        else {
            node->clippedAncestor = NULL;

            // set / inherit absolute positioning
            node->node.positionAbsolute = 0;
            if (node->node.layoutFlags & POSITION_ABSOLUTE || (node->parent != NULL && (
                node->parent->node.positionAbsolute || 
                node->parent->node.layoutFlags & POSITION_ABSOLUTE
            )))
            {
                node->node.positionAbsolute = 1;
            }

            // reset position
            node->node.x = 0.0f;
            node->node.y = 0.0f;

            // constrain preferred/min/max width/height -> These are limited by the border and padding
            // preferred width height bust also be constrained by min/max width/height
            float natural_width = node->node.borderLeft + node->node.borderRight + node->node.padLeft + node->node.padRight;
            float natural_height = node->node.borderTop + node->node.borderBottom + node->node.padTop + node->node.padBottom;
            node->node.minWidth = max(node->node.minWidth, natural_width);
            node->node.minHeight = max(node->node.minHeight, natural_height);
            node->node.maxWidth = max(max(node->node.maxWidth, natural_width), node->node.minWidth);
            node->node.maxHeight = max(max(node->node.maxHeight, natural_height), node->node.minHeight);
            node->node.preferred_width = max(node->node.preferred_width, natural_width);
            node->node.preferred_height = max(node->node.preferred_height, natural_height);
            node->node.preferred_width = min(max(node->node.preferred_width, node->node.minWidth), node->node.maxWidth);
            node->node.preferred_height = min(max(node->node.preferred_height, node->node.minHeight), node->node.maxHeight);

            // Set base width/height and reset content dimensions
            node->node.width = node->node.preferred_width;
            node->node.height = node->node.preferred_height;
            node->node.contentWidth = 0;
            node->node.contentHeight = 0;
        }
    }
    DepthFirstSearch_Free(&dfs);
}

static void NU_CalculateTextFitWidths()
{
    DepthFirstSearch dfs = DepthFirstSearch_Create(__NGUI.tree.root);
    NodeP* node;
    while (DepthFirstSearch_Next(&dfs, &node)) {
        if (node->state == 2 || node->node.textContent == NULL) continue;

        NU_Font* node_font = Vector_Get(&__NGUI.stylesheet->fonts, node->node.fontId);

        // Calculate text width & height
        float text_width = NU_Calculate_Text_Unwrapped_Width(node_font, node->node.textContent);
        
        // Calculate minimum text wrap width (longest unbreakable word)
        float min_wrap_width = NU_Calculate_Text_Min_Wrap_Width(node_font, node->node.textContent);

        // Increase width to account for text (text height will be accounted for later in NU_CalculateTextHeights())
        float natural_width = node->node.padLeft + node->node.padRight + node->node.borderLeft + node->node.borderRight;
        node->node.width = max(text_width + natural_width, node->node.preferred_width);

        // Update content width
        node->node.contentWidth = text_width; 
    }
    DepthFirstSearch_Free(&dfs);
}

static void NU_CalculateFitSizeWidths()
{
    ReverseBreadthFirstSearch rbfs = ReverseBreadthFirstSearch_Create(__NGUI.tree.root);
    NodeP* node;
    while (ReverseBreadthFirstSearch_Next(&rbfs, &node)) {
        if (node->state == 2) continue;

        int is_layout_horizontal = !(node->node.layoutFlags & LAYOUT_VERTICAL);

        // If node is a window -> set dimensions equal to window
        if (node->type == NU_WINDOW) {
            int winWidth, winHeight;
            SDL_GetWindowSize(node->node.window, &winWidth, &winHeight);
            node->node.width = (float)winWidth;
            node->node.height = (float)winHeight;
        }

        // Calculate content width from children's widths
        int visibleChildren = 0;
        NodeP* child = node->firstChild;
        while(child != NULL) {

            if (child->state != 2 && child->type != NU_WINDOW && !(child->node.layoutFlags & POSITION_ABSOLUTE)) {
                if (is_layout_horizontal) node->node.contentWidth += child->node.width;
                else node->node.contentWidth = MAX(node->node.contentWidth, child->node.width);
                visibleChildren++;
            }

            // move to next child
            child = child->nextSibling;
        }

        // Expand width to account for content width
        if (is_layout_horizontal && visibleChildren > 0) node->node.contentWidth += (visibleChildren - 1) * node->node.gap;
        if (node->type != NU_WINDOW && node->node.contentWidth > node->node.width) {
            node->node.width = node->node.contentWidth + node->node.borderLeft + node->node.borderRight + node->node.padLeft + node->node.padRight;
            NU_ApplyMinMaxWidthConstraint(node);
        }
    }
    ReverseBreadthFirstSearch_Free(&rbfs);
}

static void NU_CalculateFitSizeHeights()
{
    ReverseBreadthFirstSearch rbfs = ReverseBreadthFirstSearch_Create(__NGUI.tree.root);
    NodeP* node;
    while (ReverseBreadthFirstSearch_Next(&rbfs, &node)) {
        if (node->state == 2) continue;

        int is_layout_horizontal = !(node->node.layoutFlags & LAYOUT_VERTICAL);

        // Calculate content height from children's heights
        int visibleChildren = 0;
        NodeP* child = node->firstChild;
        while(child != NULL) {

            if (child->state != 2 && child->type != NU_WINDOW && !(child->node.layoutFlags & POSITION_ABSOLUTE)) {
                if (is_layout_horizontal) node->node.contentHeight = MAX(node->node.contentHeight, child->node.height);
                else node->node.contentHeight += child->node.height;
                visibleChildren++;
            }

            // move to next child
            child = child->nextSibling;
        }

        // Expand node height to account for content height
        if (!is_layout_horizontal && visibleChildren > 0) node->node.contentHeight += (visibleChildren - 1) * node->node.gap;
        if (node->type != NU_WINDOW) {
            if (!(node->node.layoutFlags & OVERFLOW_VERTICAL_SCROLL)) node->node.height = node->node.contentHeight + node->node.borderTop + node->node.borderBottom + node->node.padTop + node->node.padBottom;
            NU_ApplyMinMaxHeightConstraint(node);
        }
    }
    ReverseBreadthFirstSearch_Free(&rbfs);
}

static void NU_GrowShrinkChildWidths(NodeP* node)
{
    float remainingWidth = node->node.width - node->node.padLeft - node->node.padRight - node->node.borderLeft - node->node.borderRight;

    // ---------------------------------------------------------------------------------------
    // --- Expand widths of absolute elements if left and right distances are both defined ---
    // ---------------------------------------------------------------------------------------
    NodeP* child = node->firstChild;
    while (child != NULL) {

        if (child->state != 2 && child->type != NU_WINDOW && 
            child->node.layoutFlags & POSITION_ABSOLUTE &&
            child->node.left > 0.0f && child->node.right > 0.0f) 
        {
            float expandedWidth = remainingWidth - child->node.left - child->node.right;
            if (expandedWidth > child->node.width) child->node.width = expandedWidth;
            NU_ApplyMinMaxWidthConstraint(child);    
        }

        // move to the next child
        child = child->nextSibling;
    }

    remainingWidth -= (!!(node->node.layoutFlags & OVERFLOW_VERTICAL_SCROLL)) * 8.0f;

    // ------------------------------------------------
    // If node lays out children vertically ---------
    // ------------------------------------------------
    if (node->node.layoutFlags & LAYOUT_VERTICAL)
    {   
        child = node->firstChild;
        while (child != NULL) {

            if (child->state != 2 && child->type != NU_WINDOW &&
                !(child->node.layoutFlags & POSITION_ABSOLUTE) &&
                child->node.layoutFlags & GROW_HORIZONTAL && 
                remainingWidth > child->node.width)
            {
                child->node.width = remainingWidth; 
                NU_ApplyMinMaxWidthConstraint(child);
                node->node.contentWidth = max(node->node.contentWidth, child->node.width);
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

            if (child->state != 2 && child->type != NU_WINDOW && !(child->node.layoutFlags & POSITION_ABSOLUTE))
            {
                remainingWidth -= child->node.width;
                if (child->node.layoutFlags & GROW_HORIZONTAL && child->type != NU_WINDOW) growable++;
                visible++;
            }

            // move to the next child
            child = child->nextSibling;
        }
        remainingWidth -= (visible - 1) * node->node.gap;
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

                if (child->state != 2 && child->type != NU_WINDOW && 
                    !(child->node.layoutFlags & POSITION_ABSOLUTE) &&
                    (child->node.layoutFlags & GROW_HORIZONTAL) && 
                    child->node.width < child->node.maxWidth) 
                {
                    growable++;
                    if (child->node.width < smallest) {
                        secondSmallest = smallest;
                        smallest = child->node.width;
                    } else if (child->node.width < secondSmallest) {
                        secondSmallest = child->node.width;
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
                if (child->state != 2 && child->type != NU_WINDOW && 
                    !(child->node.layoutFlags & POSITION_ABSOLUTE) &&
                    child->node.layoutFlags & GROW_HORIZONTAL && 
                    child->node.width < child->node.maxWidth &&
                    child->node.width == smallest)
                {
                    float available = child->node.maxWidth - child->node.width;
                    float grow = min(width_to_add, available);
                    if (grow > 0.0f) {
                        node->node.contentWidth += grow;
                        child->node.width += grow;
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
                if (child->state != 2 && child->type != NU_WINDOW && 
                    !(child->node.layoutFlags & POSITION_ABSOLUTE) &&
                    (child->node.layoutFlags & GROW_HORIZONTAL) && 
                    child->node.width > child->node.minWidth)
                {
                    shrinkableCount++;
                    if (child->node.width > largest) {
                        secondLargest = largest;
                        largest = child->node.width;
                    } else if (child->node.width > secondLargest) {
                        secondLargest = child->node.width;
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
                if (child->state != 2 && child->type != NU_WINDOW && 
                    !(child->node.layoutFlags & POSITION_ABSOLUTE) && 
                    (child->node.layoutFlags & GROW_HORIZONTAL) && 
                    child->node.width > child->node.minWidth &&
                    child->node.width == largest)
                {
                    float available = child->node.width - child->node.minWidth;
                    float shrink = min(width_to_subtract, available);
                    if (shrink > 0.0f) {
                        node->node.contentWidth -= shrink;
                        child->node.width -= shrink;
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

static void NU_GrowShrinkChildHeights(NodeP* node)
{
    float remainingHeight = node->node.height - node->node.padTop - node->node.padBottom - node->node.borderTop - node->node.borderBottom;
    
    // ----------------------------------------------------------------------------------------
    // --- Expand heights of absolute elements if top and bottom distances are both defined ---
    // ----------------------------------------------------------------------------------------
    NodeP* child = node->firstChild;
    while(child != NULL) {

        if (child->state != 2 && child->type != NU_WINDOW && 
            child->node.layoutFlags & POSITION_ABSOLUTE &&
            child->node.top > 0.0f && child->node.bottom > 0.0f)
        {
            float expandedHeight = remainingHeight - child->node.top - child->node.bottom;
            if (expandedHeight > child->node.height) child->node.height = expandedHeight;
            NU_ApplyMinMaxHeightConstraint(child);
        }

        // move to the next child
        child = child->nextSibling;
    }

    remainingHeight -= (!!(node->node.layoutFlags & OVERFLOW_HORIZONTAL_SCROLL)) * 8.0f;

    if (!(node->node.layoutFlags & LAYOUT_VERTICAL))
    {   
        child = node->firstChild;
        while(child != NULL) {

            if (child->state != 2 && child->type != NU_WINDOW && 
                !(child->node.layoutFlags & POSITION_ABSOLUTE) &&
                child->node.layoutFlags & GROW_VERTICAL && 
                remainingHeight > child->node.height)
            {
                child->node.height = remainingHeight; 
                NU_ApplyMinMaxHeightConstraint(child);
                node->node.contentHeight = max(node->node.contentHeight, child->node.height);
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

            if (child->state != 2 && child->type != NU_WINDOW && !(child->node.layoutFlags & POSITION_ABSOLUTE))
            {
                remainingHeight -= child->node.height;
                if (child->node.layoutFlags & GROW_VERTICAL && child->type != NU_WINDOW) growable++;
                visible++;
            }

            // move to the next child
            child = child->nextSibling;
        }
        remainingHeight -= (visible - 1) * node->node.gap;
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
                
                if (child->state != 2 && child->type != NU_WINDOW && 
                    !(child->node.layoutFlags & POSITION_ABSOLUTE) && 
                    (child->node.layoutFlags & GROW_VERTICAL) && 
                    child->node.height < child->node.maxHeight)
                {
                    growable++;
                    if (child->node.height < smallest) {
                        secondSmallest = smallest;
                        smallest = child->node.height;
                    } else if (child->node.height < secondSmallest) {
                        secondSmallest = child->node.height;
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

                if (child->state != 2 && child->type != NU_WINDOW && 
                    !(child->node.layoutFlags & POSITION_ABSOLUTE) && 
                    (child->node.layoutFlags & GROW_VERTICAL) && 
                    child->node.height < child->node.maxHeight &&
                    child->node.height == smallest)
                {
                    float available = child->node.maxHeight - child->node.height;
                    float grow = min(height_to_add, available);
                    if (grow > 0.0f) {
                        node->node.contentHeight += grow;
                        child->node.height += grow;
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

static void NU_GrowShrinkWidths()
{
    BreadthFirstSearch rbfs = BreadthFirstSearch_Create(__NGUI.tree.root);
    NodeP* node;
    while (BreadthFirstSearch_Next(&rbfs, &node)) {
        if (node->state == 2 || node->type == NU_ROW || node->type == NU_TABLE) continue;
        NU_GrowShrinkChildWidths(node);
    }
    BreadthFirstSearch_Free(&rbfs);
}

static void NU_GrowShrinkHeights()
{
    BreadthFirstSearch rbfs = BreadthFirstSearch_Create(__NGUI.tree.root);
    NodeP* node;
    while (BreadthFirstSearch_Next(&rbfs, &node)) {
        if (node->state == 2 || node->type == NU_TABLE) continue;
        NU_GrowShrinkChildHeights(node);
    }
    BreadthFirstSearch_Free(&rbfs);
}

static void NU_CalculateTableColumnWidths()
{
    DepthFirstSearch bfs = DepthFirstSearch_Create(__NGUI.tree.root);
    NodeP* node;
    while(DepthFirstSearch_Next(&bfs, &node)) {
        if (node->state == 2 || node->type != NU_TABLE || node->childCount == 0) continue;

        struct Vector widest_cell_in_each_column;
        Vector_Reserve(&widest_cell_in_each_column, sizeof(float), 25);

        // ------------------------------------------------------------
        // --- Calculate the widest cell width in each table column ---
        // ------------------------------------------------------------
        NodeP* row = node->firstChild;
        while(row != NULL) {

            if (row->state == 2) {
                row = row->nextSibling; continue;
            }
            int cellIndex = 0;
            NodeP* cell = row->firstChild;
            while(cell != NULL) {

                if (cell->state == 2) {
                    cell = cell->nextSibling; continue;
                }

                // Expand the vector if there are more columns that the vector has capacity for
                if (cellIndex == widest_cell_in_each_column.size) {
                    float val = 0;
                    Vector_Push(&widest_cell_in_each_column, &val);
                }

                // Get current column width and update if cell is wider
                float* val = Vector_Get(&widest_cell_in_each_column, cellIndex);
                if (cell->node.width > *val) {
                    *val = cell->node.width;
                }
                cellIndex++;
                cell = cell->nextSibling;
            }

            row = row->nextSibling;
        }

        // -----------------------------------------------
        // --- Apply widest column widths to all cells ---
        // -----------------------------------------------
        float table_inner_width = node->node.width - node->node.borderLeft - node->node.borderRight - node->node.padLeft - node->node.padRight - (!!(node->node.layoutFlags & OVERFLOW_VERTICAL_SCROLL)) * 8.0f;
        float remaining_table_inner_width = table_inner_width;
        for (int k=0; k<widest_cell_in_each_column.size; k++) {
            remaining_table_inner_width -= *(float*)Vector_Get(&widest_cell_in_each_column, k);
        }
        float used_table_width = table_inner_width - remaining_table_inner_width;

        // Interate over all the rows in the table
        row = node->firstChild;
        while(row != NULL) {

            if (row->state == 2) {
                row = row->nextSibling; continue;
            }

            row->node.width = table_inner_width;

            // Reduce available growth space by acounting for row pad, border and child gaps
            float row_border_pad_gap = row->node.borderLeft + row->node.borderRight + row->node.padLeft + row->node.padRight;
            if (row->node.gap != 0.0f) {
                int visible_cells = 0;

                // iterate over cells in row
                NodeP* cell = row->firstChild;
                while(cell != NULL) {
                    if (cell->state != 2) visible_cells++;
                    cell = cell->nextSibling;
                }
                row_border_pad_gap += row->node.gap * (visible_cells - 1);
            }

            // Grow the width of all cells 
            int cellIndex = 0;
            NodeP* cell = row->firstChild;
            while(cell != NULL) {
                if (cell->state == 2) {
                    cell = cell->nextSibling; continue;
                }
                float column_width = *(float*)Vector_Get(&widest_cell_in_each_column, cellIndex);
                float proportion = column_width / (used_table_width);
                cell->node.width = column_width + (remaining_table_inner_width - row_border_pad_gap) * proportion;
                cellIndex++;
                cell = cell->nextSibling;
            }

            row = row->nextSibling;
        }
        Vector_Free(&widest_cell_in_each_column);
    }
    DepthFirstSearch_Free(&bfs);
}

static void NU_CalculateTextHeights()
{
    DepthFirstSearch dfs = DepthFirstSearch_Create(__NGUI.tree.root);
    NodeP* node;
    while (DepthFirstSearch_Next(&dfs, &node)) {
        if (node->state == 2) continue;

        if (node->node.textContent != NULL) {
            NU_Font* node_font = Vector_Get(&__NGUI.stylesheet->fonts, node->node.fontId);

            // Compute available inner width
            float inner_width = node->node.width - node->node.borderLeft - node->node.borderRight - node->node.padLeft - node->node.padRight;

            // Calculate text height
            float text_height = NU_Calculate_FreeText_Height_From_Wrap_Width(node_font, node->node.textContent, inner_width);

            // Increase height to account for text
            float natural_height = node->node.padTop + node->node.padBottom + node->node.borderTop + node->node.borderBottom;
            node->node.height = max(text_height + natural_height, node->node.preferred_height);
            
            // Update content height
            node->node.contentHeight = text_height;
        } 
        else if (node->type == NU_INPUT) {
            NU_Font* node_font = Vector_Get(&__NGUI.stylesheet->fonts, node->node.fontId);

            // Set input height equal to line height
            node->node.height = node_font->line_height + 
            node->node.padTop + node->node.padBottom + 
            node->node.borderTop + node->node.borderBottom;
            node->node.contentHeight = node_font->line_height;
        }
    }
    DepthFirstSearch_Free(&dfs);
}

static void NU_PositionChildrenHorizontally(NodeP* node)
{
    // layout dir -> top to bottom
    if (node->node.layoutFlags & LAYOUT_VERTICAL)
    {   
        NodeP* child = node->firstChild;
        while(child != NULL) {
            
            if (child->state == 2 || child->type == NU_WINDOW) {
                child = child->nextSibling; continue;
            }

            if (!(child->node.layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float remaning_width = (node->node.width - node->node.padLeft - node->node.padRight - node->node.borderLeft - node->node.borderRight) - child->node.width;
                float x_align_offset = remaning_width * 0.5f * (float)node->node.horizontalAlignment;
                child->node.x = node->node.x + node->node.padLeft + node->node.borderLeft + x_align_offset;
            }
            else { // position absolute
                child->node.x = node->node.x + node->node.padLeft + node->node.borderLeft;
                if (child->node.left > 0.0f) {
                    child->node.x = node->node.x + child->node.left + node->node.padLeft + node->node.borderLeft;
                }
                else if (child->node.right > 0.0f) {
                    float inner_width = node->node.width - node->node.padLeft - node->node.padRight - node->node.borderLeft - node->node.borderRight;
                    child->node.x = node->node.x + inner_width - child->node.width - child->node.right;
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
        float remainingWidth = (node->node.width - node->node.padLeft - node->node.padRight - node->node.borderLeft - node->node.borderRight);
        remainingWidth -= (!!(node->node.layoutFlags & OVERFLOW_VERTICAL_SCROLL)) * 8.0f;
        int numChildrenAffectingWidth = 0;
        NodeP* child = node->firstChild;
        while(child != NULL) {
            if (child->state != 2 && child->type != NU_WINDOW && !(child->node.layoutFlags & POSITION_ABSOLUTE)) {
                remainingWidth -= child->node.width; numChildrenAffectingWidth++;
            }

            // move to the next child
            child = child->nextSibling;
        }
        remainingWidth -= node->node.gap * (numChildrenAffectingWidth - 1);


        // position children horizontally
        float cursorX = 0.0f;
        child = node->firstChild;
        while(child != NULL) {
            if (child->state == 2 || child->type == NU_WINDOW) {
                child = child->nextSibling; continue;
            }
            
            if (!(child->node.layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float x_align_offset = remainingWidth * 0.5f * (float)node->node.horizontalAlignment;
                child->node.x += node->node.x + node->node.padLeft + node->node.borderLeft + cursorX + x_align_offset;
                cursorX += child->node.width + node->node.gap;
            }
            else { // position absolute 
                child->node.x = node->node.x + node->node.padLeft + node->node.borderLeft;
                if (child->node.left > 0.0f) {
                    child->node.x = node->node.x + child->node.left + node->node.padLeft + node->node.borderLeft;
                }
                else if (child->node.right > 0.0f) {
                    float inner_width = node->node.width - node->node.padLeft - node->node.padRight - node->node.borderLeft - node->node.borderRight;
                    child->node.x = node->node.x + inner_width - child->node.width - child->node.right;
                }
            }

            // move to the next child
            child = child->nextSibling;
        }
    }
}

static void NU_PositionChildrenVertically(NodeP* node)
{
    float y_scroll_offset = 0.0f;
    if (node->node.layoutFlags & OVERFLOW_VERTICAL_SCROLL && 
        node->childCount > 0 && 
        node->node.contentHeight > node->node.height - node->node.padTop - node->node.padBottom - node->node.borderTop - node->node.borderBottom) 
    {
        float track_h = node->node.height - node->node.borderTop - node->node.borderBottom;
        float inner_height_w_pad = track_h - node->node.padTop - node->node.padBottom;
        float inner_proportion_of_content_height = inner_height_w_pad / node->node.contentHeight;
        float thumb_h = inner_proportion_of_content_height * track_h;
        float content_scroll_range = node->node.contentHeight - inner_height_w_pad;
        float thumb_scroll_range = track_h - thumb_h;
        float scroll_factor = content_scroll_range / max(thumb_scroll_range, 1.0f);
        y_scroll_offset += (-node->node.scrollV * (track_h - thumb_h)) * scroll_factor;

        // undo effect of scroll offset for table header row
        if (node->firstChild->state != 2 && node->firstChild->type == NU_THEAD) {
            node->firstChild->node.y -= y_scroll_offset;
        }
    }

    // layout dir -> left to right
    if (!(node->node.layoutFlags & LAYOUT_VERTICAL))
    {   
        NodeP* child = node->firstChild;
        while(child != NULL) {

            if (child->state == 2 || child->type == NU_WINDOW) {
                child = child->nextSibling; continue;
            }

            if (!(child->node.layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float remaining_height = (node->node.height - node->node.padTop - node->node.padBottom - node->node.borderTop - node->node.borderBottom) - child->node.height;
                float y_align_offset = remaining_height * 0.5f * (float)node->node.verticalAlignment;
                child->node.y += node->node.y + node->node.padTop + node->node.borderTop + y_align_offset + y_scroll_offset;
            }
            else { // position absolute 
                child->node.y = node->node.y + node->node.padTop + node->node.borderTop;
                if (child->node.top > 0.0f) {
                    child->node.y = node->node.y + child->node.top + node->node.padTop + node->node.borderTop;
                }
                else if (child->node.bottom > 0.0f) {
                    float inner_height = node->node.height - node->node.padTop - node->node.padBottom - node->node.borderTop - node->node.borderBottom;
                    child->node.y = node->node.y + inner_height - child->node.height - child->node.bottom;
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
        float remainingHeight = (node->node.height - node->node.padTop - node->node.padBottom - node->node.borderTop - node->node.borderBottom);
        remainingHeight -= (!!(node->node.layoutFlags & OVERFLOW_HORIZONTAL_SCROLL)) * 8.0f;
        int numChildrenAffectingHeight = 0;
        NodeP* child = node->firstChild;
        while(child != NULL) {
            if (child->state != 2 && child->type != NU_WINDOW && !(child->node.layoutFlags & POSITION_ABSOLUTE)) {
                remainingHeight -= child->node.height; numChildrenAffectingHeight++;
            }

            // move to the next child
            child = child->nextSibling;
        }
        remainingHeight -= node->node.gap * (numChildrenAffectingHeight - 1);



        // position children vertically
        float cursorY = 0.0f;
        child = node->firstChild;
        while(child != NULL) {
            if (child->state == 2 || child->type == NU_WINDOW) {
                child = child->nextSibling; continue;
            }

            if (!(child->node.layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float y_align_offset = remainingHeight * 0.5f * (float)node->node.verticalAlignment;
                child->node.y += node->node.y + node->node.padTop + node->node.borderTop + cursorY + y_align_offset + y_scroll_offset;
                cursorY += child->node.height + node->node.gap;
            }
            else { // position abosolute
                child->node.y = node->node.y + node->node.padTop + node->node.borderTop;
                if (child->node.top > 0.0f) {
                    child->node.y = node->node.y + child->node.top + node->node.padTop + node->node.borderTop;
                }
                else if (child->node.bottom > 0.0f) {
                    float inner_height = node->node.height - node->node.padTop - node->node.padBottom - node->node.borderTop - node->node.borderBottom;
                    child->node.y = node->node.y + inner_height - child->node.height - child->node.bottom;
                }
            }

            // move to the next child
            child = child->nextSibling;
        }
    }
}

static void NU_CalculatePositions()
{
    BreadthFirstSearch dfs = BreadthFirstSearch_Create(__NGUI.tree.root);
    NodeP* node;
    while (BreadthFirstSearch_Next(&dfs, &node)) {

        if (node->state == 2) continue;

        if (node->type == NU_WINDOW) {
            node->node.x = 0;
            node->node.y = 0;
        }

        NU_PositionChildrenHorizontally(node);
        NU_PositionChildrenVertically(node);
    }
    BreadthFirstSearch_Free(&dfs);
}

void NU_Layout()
{
    NU_Prepass();
    NU_CalculateTextFitWidths();
    NU_CalculateFitSizeWidths();  
    NU_GrowShrinkWidths();
    NU_CalculateTableColumnWidths();
    NU_CalculateTextHeights();
    NU_CalculateFitSizeHeights();
    NU_GrowShrinkHeights();
    NU_CalculatePositions();
}