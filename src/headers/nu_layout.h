#include <omp.h>
#include <freetype/freetype.h>
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <stdbool.h>
#include <math.h>
#include "./text/nu_text.h"
#include "./draw/nu_draw.h"
#include "nu_window.h"

// ---------------------------
// --- UI layout -------------
// ---------------------------

static void NU_Apply_Min_Max_Size_Constraint(struct Node* node)
{   
    node->width = min(max(node->width, node->minWidth), node->maxWidth);
    node->height = min(max(node->height, node->minHeight), node->maxHeight);
}

static void NU_Apply_Min_Max_Width_Constraint(struct Node* node)
{
    node->width = min(max(node->width, node->minWidth), node->maxWidth);
    node->width = max(node->width, node->preferred_width);
}

static void NU_Apply_Min_Max_Height_Constraint(struct Node* node)
{
    node->height = min(max(node->height, node->minHeight), node->maxHeight);
    node->height = max(node->height, node->preferred_height);
}

static void NU_Clear_Node_Sizes()
{
    for (int l=0; l<=__NGUI.deepest_layer; l++)
    {
        NU_Layer* parent_layer = &__NGUI.tree.layers[l];
        NU_Layer* child_layer = &__NGUI.tree.layers[l+1];

        // Iterate over parent layer
        for (int p=0; p<parent_layer->size; p++)
        {       
            struct Node* parent = NU_Layer_Get(parent_layer, p); if (parent->nodeState == 0) continue;
            parent->clippingRootHandle = UINT32_MAX;

            // Iterate over children
            for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
            {
                struct Node* child = NU_Layer_Get(child_layer, i);

                // Hide child if parent is hidden or child is hidden
                child->nodeState = 1;
                if (parent->layoutFlags & HIDDEN || child->layoutFlags & HIDDEN) {
                    child->nodeState = 2; 
                    continue;
                }

                // Set/inherit absolute positioning
                child->positionAbsolute = 0;
                if (parent->positionAbsolute || parent->layoutFlags & POSITION_ABSOLUTE || child->layoutFlags & POSITION_ABSOLUTE) {
                    child->positionAbsolute = 1;
                }

                // Clear clipping inheritance info
                child->clippingRootHandle = UINT32_MAX;

                // Reset position
                child->x = 0.0f;
                child->y = 0.0f;

                // Constrain preferred/min/max width/height -> These are limited by the border and padding
                // Preferred width height bust also be constrained by min/max width/height
                float natural_width = child->borderLeft + child->borderRight + child->padLeft + child->padRight;
                float natural_height = child->borderTop + child->borderBottom + child->padTop + child->padBottom;
                child->minWidth = max(child->minWidth, natural_width);
                child->minHeight = max(child->minHeight, natural_height);
                child->maxWidth = max(max(child->maxWidth, natural_width), child->minWidth);
                child->maxHeight = max(max(child->maxHeight, natural_height), child->minHeight);
                child->preferred_width = max(child->preferred_width, natural_width);
                child->preferred_height = max(child->preferred_height, natural_height);
                child->preferred_width = min(max(child->preferred_width, child->minWidth), child->maxWidth);
                child->preferred_height = min(max(child->preferred_height, child->minHeight), child->maxHeight);

                // Set base width/height and reset content dimensions
                child->width = child->preferred_width;
                child->height = child->preferred_height;
                child->contentWidth = 0;
                child->contentHeight = 0;
            }
        }
    }
}

static void NU_Calculate_Text_Fit_Widths()
{
    // For each layer
    for (uint16_t l=0; l<=__NGUI.deepest_layer; l++)
    {
        NU_Layer* layer = &__NGUI.tree.layers[l];

        #pragma omp parallel for
        for (uint32_t n=0; n<layer->size; n++)
        {   
            struct Node* node = &layer->node_array[n];
            if (node->nodeState == 0 || node->nodeState == 2) continue;

            if (node->textContent != NULL) 
            {
                NU_Font* node_font = Vector_Get(&__NGUI.stylesheet->fonts, node->fontId);
                
                // Calculate text width & height
                float text_width = NU_Calculate_Text_Unwrapped_Width(node_font, node->textContent);
                
                // Calculate minimum text wrap width (longest unbreakable word)
                float min_wrap_width = NU_Calculate_Text_Min_Wrap_Width(node_font, node->textContent);

                // Increase width to account for text (text height will be accounted for later in NU_Calculate_Text_Heights())
                float natural_width = node->padLeft + node->padRight + node->borderLeft + node->borderRight;
                node->width = max(text_width + natural_width, node->preferred_width);

                // Update content width
                node->contentWidth = text_width; 
            }
        }
    }
}

static void NU_Calculate_Fit_Size_Widths()
{
    // Traverse the tree bottom-up
    for (int l=__NGUI.deepest_layer-1; l>=0; l--)
    {
        NU_Layer* parent_layer = &__NGUI.tree.layers[l];
        NU_Layer* child_layer = &__NGUI.tree.layers[l+1];
        
        // Iterate over parent layer
        for (uint32_t p=0; p<parent_layer->size; p++)
        {       
            struct Node* parent = NU_Layer_Get(parent_layer, p);
            if (parent->nodeState == 0 || parent->nodeState == 2) continue;
            int is_layout_horizontal = !(parent->layoutFlags & LAYOUT_VERTICAL);

            // If parent is a window -> set dimensions equal to window
            if (parent->tag == WINDOW) {
                int window_width, window_height;
                SDL_GetWindowSize(parent->window, &window_width, &window_height);
                parent->width = (float) window_width;
                parent->height = (float) window_height;
            }

            // Skip (no children)
            if (parent->childCount == 0) continue; 

            // Accumulate content width
            int visible_children = 0;
            for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
            {
                struct Node* child = NU_Layer_Get(child_layer, i); 

                // Ignore if child is hidden or window or absolutely positioned
                if (child->nodeState == 2 || child->tag == WINDOW || child->layoutFlags & POSITION_ABSOLUTE) continue; 

                // Horizontal Layout
                if (is_layout_horizontal) parent->contentWidth += child->width;

                // Vertical Layout 
                else parent->contentWidth = MAX(parent->contentWidth, child->width);

                visible_children++;
            }

            // Expand parent width to account for content width
            if (is_layout_horizontal && visible_children > 0) parent->contentWidth += (visible_children - 1) * parent->gap;
            if (parent->tag != WINDOW && parent->contentWidth > parent->width) {
                parent->width = parent->contentWidth + parent->borderLeft + parent->borderRight + parent->padLeft + parent->padRight;
                NU_Apply_Min_Max_Width_Constraint(parent);
            }
        }
    }
}

static void NU_Calculate_Fit_Size_Heights()
{
    // Traverse the tree bottom-up
    for (int l=__NGUI.deepest_layer-1; l>= 0; l--)
    {
        NU_Layer* parent_layer = &__NGUI.tree.layers[l];
        NU_Layer* child_layer = &__NGUI.tree.layers[l+1];

        // Iterate over parent layer
        for (uint32_t p=0; p<parent_layer->size; p++)
        {       
            struct Node* parent = NU_Layer_Get(parent_layer, p);
            if (parent->nodeState == 0 || parent->nodeState == 2) continue;
            int is_layout_horizontal = !(parent->layoutFlags & LAYOUT_VERTICAL);

            // Skip (no children)
            if (parent->childCount == 0) continue; 

            // Iterate over children
            int visible_children = 0;
            for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
            {
                struct Node* child = NU_Layer_Get(child_layer, i);

                // Ignore if child is hidden or window or absolutely positioned
                if (child->nodeState == 2 || child->tag == WINDOW || child->layoutFlags & POSITION_ABSOLUTE) continue;

                // Horizontal Layout
                if (is_layout_horizontal) parent->contentHeight = MAX(parent->contentHeight, child->height);
                
                // Vertical Layout 
                else parent->contentHeight += child->height;

                visible_children++;
            }

            // Expand parent height to account for content height
            if (!is_layout_horizontal && visible_children > 0) parent->contentHeight += (visible_children - 1) * parent->gap;
            if (parent->tag != WINDOW) {
                if (!(parent->layoutFlags & OVERFLOW_VERTICAL_SCROLL)) parent->height = parent->contentHeight + parent->borderTop + parent->borderBottom + parent->padTop + parent->padBottom;
                NU_Apply_Min_Max_Height_Constraint(parent);
            }
        }
    }
}

static void NU_Grow_Shrink_Child_Node_Widths(struct Node* parent, NU_Layer* child_layer)
{
    float remaining_width = parent->width - parent->padLeft - parent->padRight - parent->borderLeft - parent->borderRight;

    // ---------------------------------------------------------------------------------------
    // --- Expand widths of absolute elements if left and right distances are both defined ---
    // ---------------------------------------------------------------------------------------
    for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
    {
        struct Node* child = NU_Layer_Get(child_layer, i); if (child->nodeState == 2 || child->tag == WINDOW || !(child->layoutFlags & POSITION_ABSOLUTE)) continue;
        if (child->left > 0.0f && child->right > 0.0f) {
            float expanded_width = remaining_width - child->left - child->right;
            if (expanded_width > child->width) child->width = expanded_width;
            NU_Apply_Min_Max_Width_Constraint(child);
        }
    }

    remaining_width -= (!!(parent->layoutFlags & OVERFLOW_VERTICAL_SCROLL)) * 8.0f;

    // ------------------------------------------------
    // If parent lays out children vertically ---------
    // ------------------------------------------------
    if (parent->layoutFlags & LAYOUT_VERTICAL)
    {   
        for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i); if (child->nodeState == 2 || child->tag == WINDOW || child->layoutFlags & POSITION_ABSOLUTE) continue;
            if (child->layoutFlags & GROW_HORIZONTAL && remaining_width > child->width)
            {
                float pad_and_border = child->padLeft + child->padRight + child->borderLeft + child->borderRight;
                child->width = remaining_width; 
                NU_Apply_Min_Max_Width_Constraint(child);
                parent->contentWidth = max(parent->contentWidth, child->width);
            }
        }
    }

    // ------------------------------------------------
    // If parent lays out children horizontally -------
    // ------------------------------------------------
    else
    {   
        // ----------------------------------------------------
        // --- Calculate growable count and remaining width ---
        // ----------------------------------------------------
        uint32_t growable_count = 0;
        int visible_children = 0;
        for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i); if (child->nodeState == 2 || child->tag == WINDOW  || child->layoutFlags & POSITION_ABSOLUTE) continue;
            else remaining_width -= child->width;
            if (child->layoutFlags & GROW_HORIZONTAL && child->tag != WINDOW) growable_count++;
            visible_children++;
        }
        remaining_width -= (visible_children - 1) * parent->gap;
        if (growable_count == 0) return;

        // -------------------------
        // --- Grow child widths ---
        // -------------------------
        while (remaining_width > 0.01f)
        {    
            // --------------------------------------------------------------
            // --- Determine smallest, second smallest and growable count ---
            // --------------------------------------------------------------
            float smallest = 1e20f;
            float second_smallest = 1e30f;
            growable_count = 0;
            for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
                struct Node* child = NU_Layer_Get(child_layer, i); if (child->nodeState == 2 || child->tag == WINDOW || child->layoutFlags & POSITION_ABSOLUTE) continue;
                if ((child->layoutFlags & GROW_HORIZONTAL) && child->width < child->maxWidth) {
                    growable_count++;
                    if (child->width < smallest) {
                        second_smallest = smallest;
                        smallest = child->width;
                    } else if (child->width < second_smallest) {
                        second_smallest = child->width;
                    }
                }
            }

            // ----------------------------
            // --- Compute width to add ---
            // ----------------------------
            float width_to_add = remaining_width / (float)growable_count;
            if (second_smallest > smallest) {
                width_to_add = min(width_to_add, second_smallest - smallest);
            }

            // -----------------------------------------
            // --- Grow width of each eligible child ---
            // -----------------------------------------
            bool grew_any = false;
            for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
                struct Node* child = NU_Layer_Get(child_layer, i); if (child->nodeState == 2 || child->tag == WINDOW || child->layoutFlags & POSITION_ABSOLUTE) continue;        
                if (child->layoutFlags & GROW_HORIZONTAL && child->width < child->maxWidth) { // if child is growable
                    if (child->width == smallest) {
                        float available = child->maxWidth - child->width;
                        float grow = min(width_to_add, available);
                        if (grow > 0.0f) {
                            parent->contentWidth += grow;
                            child->width += grow;
                            remaining_width -= grow;
                            grew_any = true;
                        }
                    }
                }
            }
            if (!grew_any) break;
        }

        // ----------------------------------------
        // --- Shrink overgrown children (text) ---
        // ----------------------------------------
        while (remaining_width < -0.01f)
        {
            // --------------------------------------------------------------
            // --- determine smallest, second smallest and shrinkable count ---
            // --------------------------------------------------------------
            float largest = -1e20f;
            float second_largest = -1e30f;
            int shrinkable_count = 0;
            for (uint16_t i = parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
                struct Node* child = NU_Layer_Get(child_layer, i); if (child->nodeState == 2 || child->tag == WINDOW || child->layoutFlags & POSITION_ABSOLUTE) continue;
                if ((child->layoutFlags & GROW_HORIZONTAL) && child->width > child->minWidth) {
                    shrinkable_count++;
                    if (child->width > largest) {
                        second_largest = largest;
                        largest = child->width;
                    } else if (child->width > second_largest) {
                        second_largest = child->width;
                    }
                }
            }

            // ---------------------------------
            // --- Compute width to subtract ---
            // ---------------------------------
            float width_to_subtract = -remaining_width / (float)shrinkable_count;
            if (second_largest < largest && second_largest >= 0) {
                width_to_subtract = min(width_to_subtract, largest - second_largest);
            }

            // -------------------------------------------
            // --- Shrink width of each eligible child ---
            // -------------------------------------------
            bool shrunk_any = false;
            for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
                struct Node* child = NU_Layer_Get(child_layer, i); if (child->nodeState == 2 || child->tag == WINDOW || child->layoutFlags & POSITION_ABSOLUTE) continue;
                if ((child->layoutFlags & GROW_HORIZONTAL) && child->width > child->minWidth) {
                    if (child->width == largest) {
                        float available = child->width - child->minWidth;
                        float shrink = min(width_to_subtract, available);
                        if (shrink > 0.0f) {
                            parent->contentWidth -= shrink;
                            child->width -= shrink;
                            remaining_width += shrink;
                            shrunk_any = true;
                        }
                    }
                }
            }
            if (!shrunk_any) break;
        }
    }
}

static void NU_Grow_Shrink_Child_Node_Heights(struct Node* parent, NU_Layer* child_layer)
{
    float remaining_height = parent->height - parent->padTop - parent->padBottom - parent->borderTop - parent->borderBottom;
    
    // ----------------------------------------------------------------------------------------
    // --- Expand heights of absolute elements if top and bottom distances are both defined ---
    // ----------------------------------------------------------------------------------------
    for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
    {
        struct Node* child = NU_Layer_Get(child_layer, i); if (child->nodeState == 2 || child->tag == WINDOW || !(child->layoutFlags & POSITION_ABSOLUTE)) continue;
        if (child->top > 0.0f && child->bottom > 0.0f) {
            float expanded_height = remaining_height - child->top - child->bottom;
            if (expanded_height > child->height) child->height = expanded_height;
            NU_Apply_Min_Max_Height_Constraint(child);
        }
    }

    remaining_height -= (!!(parent->layoutFlags & OVERFLOW_HORIZONTAL_SCROLL)) * 8.0f;

    if (!(parent->layoutFlags & LAYOUT_VERTICAL))
    {
        for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i); if (child->nodeState == 2 || child->tag == WINDOW || child->layoutFlags & POSITION_ABSOLUTE) continue;
            if (child->layoutFlags & GROW_VERTICAL && remaining_height > child->height)
            {  
                float pad_and_border = child->padTop + child->padBottom + child->borderTop + child->borderBottom;
                child->height = remaining_height; 
                NU_Apply_Min_Max_Height_Constraint(child);
                parent->contentHeight = max(parent->contentHeight, child->height);
            }
        }
    }
    else
    {
        // -----------------------------------------------------
        // --- Calculate growable count and remaining height ---
        // -----------------------------------------------------
        uint32_t growable_count = 0;
        int visible_children = 0;
        for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->nodeState == 2 || child->tag == WINDOW || child->layoutFlags & POSITION_ABSOLUTE) continue;
            else remaining_height -= child->height;
            if (child->layoutFlags & GROW_VERTICAL && child->tag != WINDOW) growable_count++;
            visible_children++;
        }
        remaining_height -= (visible_children - 1) * parent->gap;
        if (growable_count == 0) return;

        // --------------------------
        // --- Grow child heights ---
        // --------------------------
        while (remaining_height > 0.01f)
        {
            // --------------------------------------------------------------
            // --- Determine smallest, second smallest and growable count ---
            // --------------------------------------------------------------
            float smallest = 1e20f;
            float second_smallest = 1e30f;
            growable_count = 0;
            for (uint16_t i=parent->firstChildIndex; i< parent->firstChildIndex + parent->childCount; i++) {
                struct Node* child = NU_Layer_Get(child_layer, i); if (child->nodeState == 2 || child->tag == WINDOW || child->layoutFlags & POSITION_ABSOLUTE) continue;
                if ((child->layoutFlags & GROW_VERTICAL) && child->height < child->maxHeight) {
                    growable_count++;
                    if (child->height < smallest) {
                        second_smallest = smallest;
                        smallest = child->height;
                    } else if (child->height < second_smallest) {
                        second_smallest = child->height;
                    }
                }
            }
            
            // -----------------------------
            // --- Compute height to add ---
            // -----------------------------
            float height_to_add = remaining_height / (float)growable_count;
            if (second_smallest > smallest) { 
                height_to_add = min(height_to_add, second_smallest - smallest);
            }

            // ------------------------------------------
            // --- Grow height of each eligible child ---
            // ------------------------------------------
            bool grew_any = false;
            for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
                struct Node* child = NU_Layer_Get(child_layer, i); if (child->nodeState == 2 || child->tag == WINDOW || child->layoutFlags & POSITION_ABSOLUTE) continue;
                if (child->layoutFlags & GROW_VERTICAL && child->height < child->maxHeight) { // if child is growable
                    if (child->height == smallest) {
                        float available = child->maxHeight - child->height;
                        float grow = min(height_to_add, available);
                        if (grow > 0.0f) {
                            parent->contentHeight += grow;
                            child->height += grow;
                            remaining_height -= grow;
                            grew_any = true;
                        }
                    }
                }
            }
            if (!grew_any) break;
        }

        // ----------------------------------------
        // --- Shrink overgrown children (text) ---
        // ----------------------------------------
        while (remaining_height < -0.01f)
        {
            // --------------------------------------------------------------
            // --- determine smallest, second smallest and shrinkable count ---
            // --------------------------------------------------------------
            float largest = -1e20f;
            float second_largest = -1e30f;
            int shrinkable_count = 0;
            for (uint16_t i = parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
                struct Node* child = NU_Layer_Get(child_layer, i); if (child->nodeState == 2 || child->tag == WINDOW || child->layoutFlags & POSITION_ABSOLUTE) continue;
                if ((child->layoutFlags & GROW_VERTICAL) && child->height > child->minHeight) {
                    shrinkable_count++;
                    if (child->height > largest) {
                        second_largest = largest;
                        largest = child->height;
                    } else if (child->height > second_largest) {
                        second_largest = child->height;
                    }
                }
            }

            // ---------------------------------
            // --- Compute height to subtract ---
            // ---------------------------------
            float height_to_subtract = -remaining_height / (float)shrinkable_count;
            if (second_largest < largest && second_largest >= 0) {
                height_to_subtract = min(height_to_subtract, largest - second_largest);
            }

            // -------------------------------------------
            // --- Shrink height of each eligible child ---
            // -------------------------------------------
            bool shrunk_any = false;
            for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
                struct Node* child = NU_Layer_Get(child_layer, i); if (child->nodeState == 2 || child->tag == WINDOW || child->layoutFlags & POSITION_ABSOLUTE) continue;
                if ((child->layoutFlags & GROW_VERTICAL) && child->height > child->minHeight) {
                    if (child->height == largest) {
                        float available = child->height - child->minHeight;
                        float shrink = min(height_to_subtract, available);
                        if (shrink > 0.0f) {
                            parent->contentHeight -= shrink;
                            child->height -= shrink;
                            remaining_height += shrink;
                            shrunk_any = true;
                        }
                    }
                }
            }
            if (!shrunk_any) break;
        }
    }
}

static void NU_Grow_Shrink_Widths()
{
    for (uint16_t l=0; l<=__NGUI.deepest_layer; l++)
    {
        NU_Layer* parent_layer = &__NGUI.tree.layers[l];
        NU_Layer* child_layer = &__NGUI.tree.layers[l+1];

        for (int p=0; p<parent_layer->size; p++)
        {       
            struct Node* parent = NU_Layer_Get(parent_layer, p);
            if (parent->nodeState == 0 || parent->nodeState == 2 || parent->tag == ROW || parent->tag == TABLE) continue;

            NU_Grow_Shrink_Child_Node_Widths(parent, child_layer);
        }
    }
}

static void NU_Grow_Shrink_Heights()
{
    for (uint16_t l=0; l<=__NGUI.deepest_layer; l++)
    {
        NU_Layer* parent_layer = &__NGUI.tree.layers[l];
        NU_Layer* child_layer = &__NGUI.tree.layers[l+1];

        for (int p=0; p<parent_layer->size; p++)
        {       
            struct Node* parent = NU_Layer_Get(parent_layer, p);
            if (parent->nodeState == 0 || parent->nodeState == 2 || parent->tag == TABLE) continue;

            NU_Grow_Shrink_Child_Node_Heights(parent, child_layer);
        }
    }
}

static void NU_Calculate_Table_Column_Widths()
{
    for (uint16_t l=0; l<=__NGUI.deepest_layer; l++)
    {
        NU_Layer* table_layer = &__NGUI.tree.layers[l];

        for (int i=0; i<table_layer->size; i++)
        {       
            struct Node* table = NU_Layer_Get(table_layer, i);
            if (table->nodeState == 0 || table->nodeState == 2 || table->tag != TABLE || table->childCount == 0) continue;


            struct Vector widest_cell_in_each_column;
            Vector_Reserve(&widest_cell_in_each_column, sizeof(float), 25);
            NU_Layer* row_layer = &__NGUI.tree.layers[l+1];

            // ------------------------------------------------------------
            // --- Calculate the widest cell width in each table column ---
            // ------------------------------------------------------------
            for (uint16_t r=table->firstChildIndex; r<table->firstChildIndex + table->childCount; r++)
            {
                struct Node* row = NU_Layer_Get(row_layer, r);
                if (row->nodeState == 2 || row->childCount == 0) break;

                // Iterate over cells in row
                int cell_index = 0;
                NU_Layer* cell_layer = &__NGUI.tree.layers[l+2];
                for (uint16_t c=row->firstChildIndex; c<row->firstChildIndex + row->childCount; c++)
                {
                    // Expand the vector if there are more columns that the vector has capacity for
                    if (cell_index == widest_cell_in_each_column.size) {
                        float val = 0;
                        Vector_Push(&widest_cell_in_each_column, &val);
                    }

                    // Get current column width and update if cell is wider
                    float* val = Vector_Get(&widest_cell_in_each_column, cell_index);
                    struct Node* cell = NU_Layer_Get(cell_layer, c);
                    if (cell->nodeState == 2) continue;
                    if (cell->width > *val) {
                        *val = cell->width;
                    }
                    cell_index++;
                }
            }

            // -----------------------------------------------
            // --- Apply widest column widths to all cells ---
            // -----------------------------------------------
            float table_inner_width = table->width - table->borderLeft - table->borderRight - table->padLeft - table->padRight - (!!(table->layoutFlags & OVERFLOW_VERTICAL_SCROLL)) * 8.0f;
            float remaining_table_inner_width = table_inner_width;
            for (int k=0; k<widest_cell_in_each_column.size; k++) {
                remaining_table_inner_width -= *(float*)Vector_Get(&widest_cell_in_each_column, k);
            }
            float used_table_width = table_inner_width - remaining_table_inner_width;

            // Interate over all the rows in the table
            for (uint16_t r=table->firstChildIndex; r<table->firstChildIndex + table->childCount; r++)
            {
                struct Node* row = NU_Layer_Get(row_layer, r);
                if (row->nodeState == 2) continue;
                NU_Layer* cell_layer = &__NGUI.tree.layers[l+2];

                row->width = table_inner_width;

                // Reduce available growth space by acounting for row pad, border and child gaps
                float row_border_pad_gap = row->borderLeft + row->borderRight + row->padLeft + row->padRight;
                if (row->gap != 0.0f) {
                    int visible_cells = 0;
                    for (uint16_t c=row->firstChildIndex; c<row->firstChildIndex + row->childCount; c++) {
                        struct Node* cell = NU_Layer_Get(cell_layer, c);
                        if (cell->nodeState == 2) continue;
                        visible_cells++;
                    }
                    row_border_pad_gap += row->gap * (visible_cells - 1);
                }

                // Grow the width of all cells 
                int cell_index = 0;
                for (uint16_t c=row->firstChildIndex; c<row->firstChildIndex + row->childCount; c++)
                {
                    struct Node* cell = NU_Layer_Get(cell_layer, c);
                    if (cell->nodeState == 2) continue;
                    float column_width = *(float*)Vector_Get(&widest_cell_in_each_column, cell_index);
                    float proportion = column_width / (used_table_width);
                    cell->width = column_width + (remaining_table_inner_width - row_border_pad_gap) * proportion;
                    cell_index++;
                }
            }
            Vector_Free(&widest_cell_in_each_column);
        }
    }
}

static void NU_Calculate_Text_Heights()
{
    #pragma omp parallel for
    for (uint16_t l=0; l<=__NGUI.deepest_layer; l++)
    {
        NU_Layer* layer = &__NGUI.tree.layers[l];

        // Iterate over layer
        for (int i=0; i<layer->size; i++)
        {       
            struct Node* node = &layer->node_array[i];
            if (node->nodeState == 0 || node->nodeState == 2) continue;

            if (node->textContent != NULL) 
            {
                NU_Font* node_font = Vector_Get(&__NGUI.stylesheet->fonts, node->fontId);

                // Compute available inner width
                float inner_width = node->width - node->borderLeft - node->borderRight - node->padLeft - node->padRight;

                // Calculate text height
                float text_height = NU_Calculate_FreeText_Height_From_Wrap_Width(node_font, node->textContent, inner_width);

                // Increase height to account for text
                float natural_height = node->padTop + node->padBottom + node->borderTop + node->borderBottom;
                node->height = max(text_height + natural_height, node->preferred_height);
                
                // Update content height
                node->contentHeight = text_height;
            }
        }
    }
}

static void NU_Horizontally_Place_Children(struct Node* parent, NU_Layer* child_layer)
{
    // Children placed top to bottom
    if (parent->layoutFlags & LAYOUT_VERTICAL)
    {   
        for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->nodeState == 2) continue;

            // Position absolute
            if (child->layoutFlags & POSITION_ABSOLUTE) 
            {
                child->x = parent->x + parent->padLeft + parent->borderLeft;
                if (child->left > 0.0f) {
                    child->x = parent->x + child->left + parent->padLeft + parent->borderLeft;
                }
                else if (child->right > 0.0f) {
                    float inner_width = parent->width - parent->padLeft - parent->padRight - parent->borderLeft - parent->borderRight;
                    child->x = parent->x + inner_width - child->width - child->right;
                }
            }

            // Position relative
            else
            {
                float remaning_width = (parent->width - parent->padLeft - parent->padRight - parent->borderLeft - parent->borderRight) - child->width;
                float x_align_offset = remaning_width * 0.5f * (float)parent->horizontalAlignment;
                child->x = parent->x + parent->padLeft + parent->borderLeft + x_align_offset;
            }
        }
    }

    // Children placed left to right
    else
    {
        // Calculate remaining width (optimise this by caching this value inside parent's content width variable)
        float remaining_width = (parent->width - parent->padLeft - parent->padRight - parent->borderLeft - parent->borderRight) - (parent->childCount - 1) * parent->gap - (!!(parent->layoutFlags & OVERFLOW_VERTICAL_SCROLL)) * 8.0f;
        for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->nodeState == 2) continue;
            if (child->layoutFlags & POSITION_ABSOLUTE) continue; // Skip because absolute child doen't affect flow
            if (child->tag == WINDOW) remaining_width += parent->gap;
            else remaining_width -= child->width;
        }

        float cursor_x = 0.0f;

        for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->nodeState == 2) continue;
            
            // Position absolute
            if (child->layoutFlags & POSITION_ABSOLUTE) 
            {
                child->x = parent->x + parent->padLeft + parent->borderLeft;
                if (child->left > 0.0f) {
                    child->x = parent->x + child->left + parent->padLeft + parent->borderLeft;
                }
                else if (child->right > 0.0f) {
                    float inner_width = parent->width - parent->padLeft - parent->padRight - parent->borderLeft - parent->borderRight;
                    child->x = parent->x + inner_width - child->width - child->right;
                }
            }

            // Position relative
            else 
            {
                float x_align_offset = remaining_width * 0.5f * (float)parent->horizontalAlignment;
                child->x = parent->x + parent->padLeft + parent->borderLeft + cursor_x + x_align_offset;
                cursor_x += child->width + parent->gap;
            }
        }
    }
}

static void NU_Vertically_Place_Children(struct Node* parent, NU_Layer* child_layer)
{
    float y_scroll_offset = 0.0f;
    if (parent->layoutFlags & OVERFLOW_VERTICAL_SCROLL && parent->childCount > 0 && 
        parent->contentHeight > parent->height - parent->padTop - parent->padBottom - parent->borderTop - parent->borderBottom) 
    {
        float track_h = parent->height - parent->borderTop - parent->borderBottom;
        float inner_height_w_pad = track_h - parent->padTop - parent->padBottom;
        float inner_proportion_of_content_height = inner_height_w_pad / parent->contentHeight;
        float thumb_h = inner_proportion_of_content_height * track_h;
        float content_scroll_range = parent->contentHeight - inner_height_w_pad;
        float thumb_scroll_range = track_h - thumb_h;
        float scroll_factor = content_scroll_range / max(thumb_scroll_range, 1.0f);
        y_scroll_offset += (-parent->scrollV * (track_h - thumb_h)) * scroll_factor;

        struct Node* first_child = NU_Layer_Get(child_layer, parent->firstChildIndex);
        if (first_child->nodeState != 2 && first_child->tag == THEAD) {
            first_child->y -= y_scroll_offset;
        }
    }

    // Children placed left to right
    if (!(parent->layoutFlags & LAYOUT_VERTICAL))
    {   
        for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->nodeState == 2) continue;

            // Position absolute
            if (child->layoutFlags & POSITION_ABSOLUTE) 
            {
                child->y = parent->y + parent->padTop + parent->borderTop;
                if (child->top > 0.0f) {
                    child->y = parent->y + child->top + parent->padTop + parent->borderTop;
                }
                else if (child->bottom > 0.0f) {
                    float inner_height = parent->height - parent->padTop - parent->padBottom - parent->borderTop - parent->borderBottom;
                    child->y = parent->y + inner_height - child->height - child->bottom;
                }
            }

            // Position relative
            else 
            {
                float remaining_height = (parent->height - parent->padTop - parent->padBottom - parent->borderTop - parent->borderBottom) - child->height;
                float y_align_offset = remaining_height * 0.5f * (float)parent->verticalAlignment;
                child->y += parent->y + parent->padTop + parent->borderTop + y_align_offset + y_scroll_offset;
            }
        }
    }

    // Children placed top to bottom
    else
    {
        // Calculate remaining height (optimise this by caching this value inside parent's content height variable)
        float remaining_height = (parent->height - parent->padTop - parent->padBottom - parent->borderTop - parent->borderBottom) - (parent->childCount - 1) * parent->gap - (!!(parent->layoutFlags & OVERFLOW_HORIZONTAL_SCROLL)) * 8.0f;
        for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->nodeState == 2) continue;
            if (child->tag == WINDOW) remaining_height += parent->gap;
            else remaining_height -= child->height;
        }

        float cursor_y = 0.0f;

        for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->nodeState == 2) continue;

            // Position relative
            if (child->layoutFlags & POSITION_ABSOLUTE) 
            {
                child->y = parent->y + parent->padTop + parent->borderTop;
                if (child->top > 0.0f) {
                    child->y = parent->y + child->top + parent->padTop + parent->borderTop;
                }
                else if (child->bottom > 0.0f) {
                    float inner_height = parent->height - parent->padTop - parent->padBottom - parent->borderTop - parent->borderBottom;
                    child->y = parent->y + inner_height - child->height - child->bottom;
                }
            }

            // Position relative
            else 
            {
                float y_align_offset = remaining_height * 0.5f * (float)parent->verticalAlignment;
                child->y += parent->y + parent->padTop + parent->borderTop + cursor_y + y_align_offset + y_scroll_offset;
                cursor_y += child->height + parent->gap;
            }
        }
    }
}

static void NU_Calculate_Positions()
{
    for (uint16_t l=0; l<=__NGUI.deepest_layer; l++) 
    {
        NU_Layer* parent_layer = &__NGUI.tree.layers[l];
        NU_Layer* child_layer = &__NGUI.tree.layers[l+1];
        
        // Iterate over parent layer
        for (uint32_t p=0; p<parent_layer->size; p++) // For node in layer
        {   
            struct Node* parent = NU_Layer_Get(parent_layer, p);
            if (parent->nodeState == 0 || parent->nodeState == 2) continue;

            if (parent->tag == WINDOW)
            {
                parent->x = 0;
                parent->y = 0;
            }

            NU_Horizontally_Place_Children(parent, child_layer);
            NU_Vertically_Place_Children(parent, child_layer);
        }
    }
}




// -----------------------------
// --- Interaction -------------
// -----------------------------
static bool NU_Mouse_Over_Node(struct Node* node, float mouse_x, float mouse_y)
{
    // --- Get clipping info
    float left_wall = node->x;
    float right_wall = node->x + node->width;
    float top_wall = node->y;
    float bottom_wall = node->y + node->height; 
    if (node->clippingRootHandle != UINT32_MAX)
    {
        NU_Clip_Bounds* clip = HashmapGet(&__NGUI.node_clip_map, &node->clippingRootHandle);
        left_wall = max(clip->clip_left, node->x);
        right_wall = min(clip->clip_right, node->x + node->width);
        top_wall = max(clip->clip_top, node->y);
        bottom_wall = min(clip->clip_bottom, node->y + node->height);
    }

    // --- Check if mouse is within clipped bounding box
    bool within_x_bound = mouse_x >= left_wall && mouse_x <= right_wall;
    bool within_y_bound = mouse_y >= top_wall && mouse_y <= bottom_wall;
    if (!(within_x_bound && within_y_bound)) return false; // Not in bounding rect

    // --- Constrain border radii ---
    float borderRadiusBl = node->borderRadiusBl;
    float borderRadiusBr = node->borderRadiusBr;
    float borderRadiusTl = node->borderRadiusTl;
    float borderRadiusTr = node->borderRadiusTr;
    float left_radii_sum   = borderRadiusTl + borderRadiusBl;
    float right_radii_sum  = borderRadiusTr + borderRadiusBr;
    float top_radii_sum    = borderRadiusTl + borderRadiusTr;
    float bottom_radii_sum = borderRadiusBl + borderRadiusBr;
    if (left_radii_sum   > node->height)  { float scale = node->height / left_radii_sum;   borderRadiusTl *= scale; borderRadiusBl *= scale; }
    if (right_radii_sum  > node->height)  { float scale = node->height / right_radii_sum;  borderRadiusTr *= scale; borderRadiusBr *= scale; }
    if (top_radii_sum    > node->width )  { float scale = node->width  / top_radii_sum;    borderRadiusTl *= scale; borderRadiusTr *= scale; }
    if (bottom_radii_sum > node->width )  { float scale = node->width  / bottom_radii_sum; borderRadiusBl *= scale; borderRadiusBr *= scale; }

    // --- Rounded border anchors ---
    vec2 tl_a = { floorf(node->x + borderRadiusTl),               floorf(node->y + borderRadiusTl) };
    vec2 tr_a = { floorf(node->x + node->width - borderRadiusTr), floorf(node->y + borderRadiusTr) };
    vec2 bl_a = { floorf(node->x + borderRadiusBl),               floorf(node->y + node->height - borderRadiusBl) };
    vec2 br_a = { floorf(node->x + node->width - borderRadiusBr), floorf(node->y + node->height - borderRadiusBr) };


    // --- Ensure mouse is not in top left rounded deadzone
    if (mouse_x < tl_a.x && mouse_y < tl_a.y)
    {
        float dist = sqrtf((mouse_x - tl_a.x) * (mouse_x - tl_a.x) + (mouse_y - tl_a.y) * (mouse_y - tl_a.y)); 
        if (dist > borderRadiusTl) return false;
    }

    // --- Ensure mouse is not in top right rounded deadzone
    if (mouse_x > tr_a.x && mouse_y < tr_a.y)
    {
        float dist = sqrtf((mouse_x - tr_a.x) * (mouse_x - tr_a.x) + (mouse_y - tr_a.y) * (mouse_y - tr_a.y)); 
        if (dist > borderRadiusTr) return false;
    }

    // --- Ensure mouse is not in bottom left rounded deadzone
    if (mouse_x < bl_a.x && mouse_y > bl_a.y)
    {
        float dist = sqrtf((mouse_x - bl_a.x) * (mouse_x - bl_a.x) + (mouse_y - bl_a.y) * (mouse_y - bl_a.y)); 
        if (dist > borderRadiusBl) return false;
    }

    // --- Ensure mouse is not in bottom right rounded deadzone
    if (mouse_x > br_a.x && mouse_y > br_a.y)
    {
        float dist = sqrtf((mouse_x - br_a.x) * (mouse_x - br_a.x) + (mouse_y - br_a.y) * (mouse_y - br_a.y)); 
        if (dist > borderRadiusBr) return false;
    }

    return true;
}

static bool NU_Mouse_Over_Node_V_Scrollbar(struct Node* node, float mouse_x, float mouse_y) {
    float track_height = node->height - node->borderTop - node->borderBottom;
    float thumb_height = (track_height / node->contentHeight) * track_height;
    float scroll_thumb_left_wall = node->x + node->width - node->borderRight - 8.0f;
    float scroll_thumb_top_wall = node->y + node->borderTop + (node->scrollV * (track_height - thumb_height));
    bool within_x_bound = mouse_x >= scroll_thumb_left_wall && mouse_x <= scroll_thumb_left_wall + 8.0f;
    bool within_y_bound = mouse_y >= scroll_thumb_top_wall && mouse_y <= scroll_thumb_top_wall + thumb_height;
    return within_x_bound && within_y_bound;
}

void NU_Mouse_Hover()
{   
    // -----------------------------------------------------------
    // --- Remove potential pseudo style from current hovered node
    // -----------------------------------------------------------
    if (__NGUI.hovered_node != UINT32_MAX && __NGUI.hovered_node != __NGUI.mouse_down_node) {
        NU_Apply_Stylesheet_To_Node(NODE(__NGUI.hovered_node), __NGUI.stylesheet);
    }
    uint32_t prev_hovered_node = __NGUI.hovered_node;
    __NGUI.hovered_node = UINT32_MAX;
    __NGUI.scroll_hovered_node = UINT32_MAX;

    if (__NGUI.hovered_window == NULL) return;


    // -----------------------------
    // --- Get local mouse position
    // -----------------------------
    float global_mouse_x, global_mouse_y;
    int window_x, window_y;
    SDL_GetGlobalMouseState(&global_mouse_x, &global_mouse_y);
    SDL_GetWindowPosition(__NGUI.hovered_window, &window_x, &window_y);
    float mouse_x = global_mouse_x - window_x;
    float mouse_y = global_mouse_y - window_y;


    // ----------------------------
    // --- Create a traversal stack
    // ----------------------------
    struct Vector stack;
    Vector_Reserve(&stack, sizeof(struct Node*), 32);
    
    // -----------------------------------------------
    // --- Push all the nodes to start traversing from
    // -----------------------------------------------
    for (uint32_t i=0; i<__NGUI.absoluteRootNodes.size; i++)
    {
        struct Node* absolute_node = *(struct Node**)Vector_Get(&__NGUI.absoluteRootNodes, i);
        if (absolute_node->window == __NGUI.hovered_window && NU_Mouse_Over_Node(absolute_node, mouse_x, mouse_y)) {
            Vector_Push(&stack, &absolute_node);
        }
    }

    for (uint32_t i=0; i<__NGUI.windowNodes.size; i++)
    {
        uint32_t handle = *(uint32_t*)Vector_Get(&__NGUI.windowNodes, i);
        struct Node* node = NODE(handle);
        if (node->window == __NGUI.hovered_window){
            Vector_Push(&stack, &node);
            break;
        }
    }

    // -------------------------
    // --- Traverse the tree ---
    // -------------------------
    bool break_loop = false;
    while (stack.size > 0 && !break_loop) 
    {
        // -----------------
        // --- Pop the stack
        // -----------------
        struct Node* current_node = *(struct Node**)Vector_Get(&stack, stack.size - 1);
        __NGUI.hovered_node = current_node->handle;
        stack.size -= 1;

        if (current_node->tag == BUTTON) continue; // Skip children

        // ------------------------------
        // --- Iterate over children
        // ------------------------------
        NU_Layer* child_layer = &__NGUI.tree.layers[current_node->layer+1];
        for (uint16_t i=current_node->firstChildIndex; i<current_node->firstChildIndex + current_node->childCount; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->nodeState == 2 || 
                child->layoutFlags & POSITION_ABSOLUTE || 
                child->tag == WINDOW ||
                !NU_Mouse_Over_Node(child, mouse_x, mouse_y)) continue; // Skip

            // Check for scroll hover
            if (child->layoutFlags & OVERFLOW_V_PROPERTY) {
                bool overflow_v = child->contentHeight > child->height - child->borderTop - child->borderBottom;
                if (overflow_v) {
                    __NGUI.scroll_hovered_node = child->handle;
                }
            }
            Vector_Push(&stack, &child);
        }
    }


    // ----------------------------------------
    // Apply hover pseudo style to hovered node
    // ----------------------------------------
    if (__NGUI.hovered_node != UINT32_MAX && __NGUI.hovered_node != __NGUI.mouse_down_node) {
        NU_Apply_Pseudo_Style_To_Node(NODE(__NGUI.hovered_node), __NGUI.stylesheet, PSEUDO_HOVER);
    } 
    Vector_Free(&stack);

    // If hovered node change -> must redraw later
    if (prev_hovered_node != __NGUI.hovered_node) {
        __NGUI.awaiting_redraw = true;
    }
}



// -----------------------------
// --- Rendering ---------------
// -----------------------------
void NU_Add_Text_Mesh(Vertex_RGB_UV_List* vertices, Index_List* indices, struct Node* node)
{
    // Compute inner dimensions (content area)
    float inner_width  = node->width  - node->borderLeft - node->borderRight - node->padLeft - node->padRight;
    float inner_height = node->height - node->borderTop  - node->borderBottom - node->padTop - node->padBottom;
    float remaining_w = inner_width  - node->contentWidth;
    float remaining_h = inner_height - node->contentHeight;
    float x_align_offset = remaining_w * 0.5f * (float)node->horizontalTextAlignment;
    float y_align_offset = remaining_h * 0.5f * (float)node->verticalTextAlignment;

    // Top-left corner of the content area
    float textPosX = node->x + node->borderLeft + node->padLeft + x_align_offset;
    float textPosY = node->y + node->borderTop  + node->padTop + y_align_offset;

    // Draw wrapped text inside inner_width
    float r = (float)node->textR / 255.0f;
    float g = (float)node->textG / 255.0f;
    float b = (float)node->textB / 255.0f;
    NU_Font* node_font = Vector_Get(&__NGUI.stylesheet->fonts, node->fontId);
    NU_Generate_Text_Mesh(vertices, indices, node_font, node->textContent, floorf(textPosX), floorf(textPosY), r, g, b, inner_width);
}

static bool Is_Node_Visible_In_Window(struct Node* node, float window_width, float window_height)
{
    // Compute the node's content rectangle
    float left   = node->x;
    float top    = node->y;
    float right  = left + node->width;
    float bottom = top  + node->height;

    // Check for any overlap with the window bounds
    bool visible = !(right < 0 || bottom < 0 || left > window_width || top > window_height);
    return visible;
}

static bool Is_Node_Visible_In_Bounds(struct Node* node, float x, float y, float w, float h)
{
    // Compute the node's content rectangle
    float left   = node->x;
    float top    = node->y;
    float right  = left + node->width;
    float bottom = top + node->height;

    // Compute the bounds rectangle
    float bounds_left   = x;
    float bounds_top    = y;
    float bounds_right  = x + w;
    float bounds_bottom = y + h;

    // Check for any overlap between node and bounds
    bool visible = !(right < bounds_left || bottom < bounds_top || left > bounds_right || top > bounds_bottom);
    return visible;
}

static int Node_Vertical_Overlap_State(struct Node* node, float y, float h)
{
    float top    = node->y;
    float bottom = top + node->height;

    float bounds_top    = y;
    float bounds_bottom = y + h;

    // No intersection
    if (bottom <= bounds_top || top >= bounds_bottom)
        return 0;

    // Fully inside
    if (top >= bounds_top && bottom <= bounds_bottom)
        return 2;

    // Partial overlap
    return 1;
}

static int NU_Draw_Find_Window(struct Node* node) {
    for (int i=0; i<__NGUI.windows.size; i++) {
        SDL_Window* window = *(SDL_Window**) Vector_Get(&__NGUI.windows, i);
        if (window == node->window) {
            return i;
        }
    }
    return 0;
}

void NU_Populate_Draw_Lists()
{
    // Initialise draw lists
    uint32_t window_count = __NGUI.windows.size;
    for (uint32_t i=0; i<window_count; i++) 
    {
        NU_Window_Draw_Lists* lists = Vector_Get(&__NGUI.windowsDrawLists, i);
        Vector_Reserve(&lists->relativeNodeList, sizeof(struct Node*), 512);
        Vector_Reserve(&lists->clippedRelativeNodeList, sizeof(struct Node*), 64);
        Vector_Reserve(&lists->absoluteNodeList, sizeof(struct Node*), 64);
        Vector_Reserve(&lists->clippedAbsoluteNodeList, sizeof(struct Node*), 16);
        Vector_Reserve(&lists->canvasNodeList, sizeof(struct Node*), 4);
        Vector_Reserve(&lists->clippedCanvasNodeList, sizeof(struct Node*), 1);
    }

    // Clear clip map
    Hashmap* node_clip_map = &__NGUI.node_clip_map;
    HashmapClear(&__NGUI.node_clip_map);

    __NGUI.absoluteRootNodes.size = 0;

    

    NU_Window_Draw_Lists* root_lists = Vector_Get(&__NGUI.windowsDrawLists, 0);


    struct Node* root = &__NGUI.tree.layers[0].node_array[0];
    Vector_Push(&root_lists->relativeNodeList, &root);
    for (int l=0; l<=__NGUI.deepest_layer; l++)
    {
        NU_Layer* parent_layer = &__NGUI.tree.layers[l];
        NU_Layer* child_layer = &__NGUI.tree.layers[l+1];

        // Iterate over parent layer
        for (int p=0; p<parent_layer->size; p++)
        {       
            struct Node* parent = NU_Layer_Get(parent_layer, p);
            if (parent->nodeState == 0) continue;
            if (parent->nodeState == 2) { // Parent not visible -> children must inherit this
                for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
                    struct Node* child = NU_Layer_Get(child_layer, i);
                    child->nodeState = 2;
                }
                continue;
            }

            // Precompute once (optimisation)
            float parent_inner_x, parent_inner_y, parent_inner_width, parent_inner_height = 0;
            if (parent->layoutFlags & OVERFLOW_VERTICAL_SCROLL) {
                parent_inner_y = parent->y + parent->borderTop + parent->padTop;
                parent_inner_height = parent->height - parent->borderTop - parent->borderBottom - parent->padTop - parent->padBottom;

                // Handle case where parent is a table with a thead
                if (parent->tag == TABLE) {
                    struct Node* first_child = NU_Layer_Get(child_layer, parent->firstChildIndex);
                    if (first_child->tag == THEAD) {
                        parent_inner_y += first_child->height;
                        parent_inner_height -= first_child->height;
                    }
                }
            }
            if (parent->layoutFlags & OVERFLOW_HORIZONTAL_SCROLL) {
                parent_inner_x = parent->x + parent->borderLeft + parent->padLeft;
                parent_inner_width = parent->width - parent->borderLeft - parent->borderRight - parent->padLeft - parent->padRight;
            }

            // Iterate over children
            for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
            {
                struct Node* child = NU_Layer_Get(child_layer, i); if (child->nodeState == 2) continue;

                // If child not visible in window bounds -> mark as hidden
                int window_index, w, h;
                window_index = NU_Draw_Find_Window(child);
                SDL_Window* window = *(SDL_Window**) Vector_Get(&__NGUI.windows, window_index);
                SDL_GetWindowSize(window, &w, &h);
                if (!Is_Node_Visible_In_Window(child, (float)w, (float)h)) {
                    child->nodeState = 2; 
                    continue;
                }

                // Add child to list of root absolute nodes
                if (child->layoutFlags & POSITION_ABSOLUTE) {
                    Vector_Push(&__NGUI.absoluteRootNodes, &child);
                }

                // Get correct draw lists
                NU_Window_Draw_Lists* lists = Vector_Get(&__NGUI.windowsDrawLists, window_index);



                // Skip node if not visible in parent (due to overflow)
                if (parent->layoutFlags & OVERFLOW_VERTICAL_SCROLL && child->tag != THEAD) 
                {   
                    int intersect_state = Node_Vertical_Overlap_State(child, parent_inner_y, parent_inner_height);

                    // Child not inside parent -> hide in this draw pass
                    if (intersect_state == 0) {
                        child->nodeState = 2; 
                        continue;
                    }

                    // Child overlaps parent boundary
                    else if (intersect_state == 1) 
                    {
                        // Determine clipping
                        NU_Clip_Bounds clip;
                        clip.clip_top = fmaxf(child->y - 1, parent_inner_y);
                        clip.clip_bottom = fminf(child->y + child->height, parent_inner_y + parent_inner_height);
                        clip.clip_left = -1.0f;
                        clip.clip_right = 1000000.0f;

                        // Position Absolute -> secondary drawing list
                        if (child->positionAbsolute)
                        {
                            // If parent is also clipped -> merge clips (stack clipping behaviour)
                            if (parent->clippingRootHandle != UINT32_MAX) {
                                NU_Clip_Bounds* parent_clip = HashmapGet(node_clip_map, &parent->clippingRootHandle);
                                clip.clip_top = fmaxf(clip.clip_top, parent_clip->clip_top);
                                clip.clip_bottom = fminf(clip.clip_bottom, parent_clip->clip_bottom);
                            }
                            
                            // Add clipping to hashmap
                            HashmapSet(node_clip_map, &child->handle, &clip);
                            child->clippingRootHandle = child->handle; // Set clip root to self

                            // Append node to correct window clipped node list
                            Vector_Push(&lists->clippedAbsoluteNodeList, &child);
                            continue;
                        }
                        // Position Relative -> main drawing list
                        else
                        {
                            // If parent is also clipped -> merge clips (stack clipping behaviour)
                            if (parent->clippingRootHandle != UINT32_MAX) {
                                NU_Clip_Bounds* parent_clip = HashmapGet(node_clip_map, &parent->clippingRootHandle);
                                clip.clip_top = fmaxf(clip.clip_top, parent_clip->clip_top);
                                clip.clip_bottom = fminf(clip.clip_bottom, parent_clip->clip_bottom);
                            }
                            
                            // Add clipping to hashmap
                            HashmapSet(node_clip_map, &child->handle, &clip);
                            child->clippingRootHandle = child->handle; // Set clip root to self

                            // Append node to correct window clipped node list
                            Vector_Push(&lists->clippedRelativeNodeList, &child);
                            continue;
                        }
                    }
                }

                // If parent is clipped -> child inherits clip from parent
                if (parent->clippingRootHandle != UINT32_MAX) {
                    child->clippingRootHandle = parent->clippingRootHandle;

                    // Position Absolute -> secondary drawing list
                    if (child->positionAbsolute)
                    {
                        Vector_Push(&lists->clippedAbsoluteNodeList, &child);
                    }
                    // Position Relative -> main drawing list
                    else
                    {
                        Vector_Push(&lists->clippedRelativeNodeList, &child);
                    }
                    continue;
                }



                // Neither child nor parent is clipped -> append node to correct window node list
                if (child->positionAbsolute) // Position Absolute -> secondary drawing list
                {
                    Vector_Push(&lists->absoluteNodeList, &child);
                }
                else // Position Relative -> main drawing list
                {
                    Vector_Push(&lists->relativeNodeList, &child);
                }

                // Append node to canvas API drawing list
                if (child->tag == CANVAS) {
                    Vector_Push(&lists->canvasNodeList, &child);
                }
            }
        }
    }
}

void NU_Draw()
{
    uint32_t window_count = __NGUI.windows.size;

    // Initialise text vertex and index buffers (per font)
    Vertex_RGB_UV_List text_relative_vertex_buffers[__NGUI.stylesheet->fonts.size];
    Vertex_RGB_UV_List text_absolute_vertex_buffers[__NGUI.stylesheet->fonts.size];
    Index_List text_relative_index_buffers[__NGUI.stylesheet->fonts.size];
    Index_List text_absolute_index_buffers[__NGUI.stylesheet->fonts.size];
    for (uint32_t i=0; i<__NGUI.stylesheet->fonts.size; i++)
    {
        Vertex_RGB_UV_List_Init(&text_relative_vertex_buffers[i], 512);
        Index_List_Init(&text_relative_index_buffers[i], 512);
        Vertex_RGB_UV_List_Init(&text_absolute_vertex_buffers[i], 128);
        Index_List_Init(&text_absolute_index_buffers[i], 128);
    }

    // For each window
    for (uint32_t i=0; i<window_count; i++)
    {
        NU_Window_Draw_Lists* lists = Vector_Get(&__NGUI.windowsDrawLists, i);
        SDL_Window* window = *(SDL_Window**) Vector_Get(&__NGUI.windows, i);
        SDL_GL_MakeCurrent(window, __NGUI.gl_ctx);

        // ----------------------------------------
        // --- Clear the window and start new frame
        // ----------------------------------------
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        float w_fl = (float)w; float h_fl = (float)h;
        glViewport(0, 0, w, h); glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);




        // --------------------------------------------------------------------------------------------------------
        // --- Draw Relative Positioned Nodes [First Draw Pass] ===================================================
        // --------------------------------------------------------------------------------------------------------

        // --- Draw Border Rects ---
        Vertex_RGB_List border_rect_vertices;              Index_List border_rect_indices;
        Vertex_RGB_List_Init(&border_rect_vertices, 5000); Index_List_Init(&border_rect_indices, 15000);
        for (int n=0; n<lists->relativeNodeList.size; n++) { // Construct border rect vertices and indices for each node
            struct Node* node = *(struct Node**) Vector_Get(&lists->relativeNodeList, n);
            Construct_Border_Rect(node, w_fl, h_fl, &border_rect_vertices, &border_rect_indices);
            if (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL 
                && node->contentHeight > (node->height - node->padTop - node->padBottom - node->borderTop - node->borderBottom)) {
                Construct_Scroll_Thumb(node, w_fl, h_fl, &border_rect_vertices, &border_rect_indices);
            }
        }
        Draw_Vertex_RGB_List(&border_rect_vertices, &border_rect_indices, w_fl, h_fl, 0.0f, 0.0f); // Draw border rects in one call
        Vertex_RGB_List_Free(&border_rect_vertices);
        Index_List_Free(&border_rect_indices);


        // --- Draw Canvas API Content ---
        for (uint32_t n=0; n<lists->canvasNodeList.size; n++)
        {
            struct Node* canvas_node = *(struct Node**) Vector_Get(&lists->canvasNodeList, n);
            float offset_x = canvas_node->x + canvas_node->borderLeft + canvas_node->padLeft;
            float offset_y = canvas_node->y + canvas_node->borderTop + canvas_node->padTop;
            float clip_top    = canvas_node->y + canvas_node->borderTop + canvas_node->padTop;
            float clip_bottom = canvas_node->y + canvas_node->height - canvas_node->borderBottom - canvas_node->padBottom;
            float clip_left   = canvas_node->x + canvas_node->borderLeft + canvas_node->padLeft;
            float clip_right  = canvas_node->x + canvas_node->width - canvas_node->borderRight - canvas_node->padRight;
            NU_Canvas_Context* ctx = HashmapGet(&__NGUI.canvas_contexts, &canvas_node->handle);
            Draw_Clipped_Vertex_RGB_List(
                &ctx->vertices, 
                &ctx->indices,
                w_fl, h_fl, 
                offset_x, offset_y,
                clip_top, clip_bottom, clip_left, clip_right 
            );
        }


        // --- Construct Text Meshes & Draw Images ---
        for (int n=0; n<lists->relativeNodeList.size; n++) 
        {

            // Construct text mesh for node
            struct Node* node = *(struct Node**) Vector_Get(&lists->relativeNodeList, n);
            if (node->textContent != NULL) {
                Vertex_RGB_UV_List* text_vertices = &text_relative_vertex_buffers[node->fontId];
                Index_List* text_indices = &text_relative_index_buffers[node->fontId];
                NU_Add_Text_Mesh(text_vertices, text_indices, node);
            }

            if (node->glImageHandle) // Draw image
            {
                float inner_width  = node->width - node->borderLeft - node->borderRight - node->padLeft - node->padRight;
                float inner_height = node->height - node->borderTop - node->borderBottom - node->padTop - node->padBottom;
                float x = node->x + node->borderLeft + (float)node->padLeft;
                float y = node->y + node->borderTop + (float)node->padTop;
                NU_Draw_Image(
                    x, y, 
                    inner_width, inner_height, 
                    w_fl, h_fl, 
                    -1.0f, 100000.0f, -1.0f, 100000.0f,
                    node->glImageHandle);
            }
        }
        for (uint32_t t=0; t<__NGUI.stylesheet->fonts.size; t++) // Draw text
        {
            Vertex_RGB_UV_List* text_vertices = &text_relative_vertex_buffers[t];
            Index_List* text_indices = &text_relative_index_buffers[t];
            NU_Font* node_font = Vector_Get(&__NGUI.stylesheet->fonts, t);
            NU_Render_Text(text_vertices, text_indices, node_font, w_fl, h_fl, -1.0f, 100000.0f, -1.0f, 100000.0f);
            text_vertices->size = 0;
            text_indices->size = 0;
        }


        // --- Draw Partially Visible Nodes ---
        for (int n=0; n<lists->clippedRelativeNodeList.size; n++) {
            struct Node* node = *(struct Node**) Vector_Get(&lists->clippedRelativeNodeList, n);
            NU_Clip_Bounds* clip = (NU_Clip_Bounds*)HashmapGet(&__NGUI.node_clip_map, &node->clippingRootHandle);

            // Draw border rects
            Vertex_RGB_List vertices;
            Index_List indices;
            Vertex_RGB_List_Init(&vertices, 100);
            Index_List_Init(&indices, 100);
            Construct_Border_Rect(node, w_fl, h_fl, &vertices, &indices);
            Draw_Clipped_Vertex_RGB_List(&vertices, &indices, w_fl, h_fl, 0.0f, 0.0f, clip->clip_top, clip->clip_bottom, clip->clip_left, clip->clip_right);
            Vertex_RGB_List_Free(&vertices);
            Index_List_Free(&indices);

            // Draw text
            if (node->textContent != NULL) 
            {
                Vertex_RGB_UV_List clipped_text_vertices;
                Index_List clipped_text_indices;
                Vertex_RGB_UV_List_Init(&clipped_text_vertices, 1000);
                Index_List_Init(&clipped_text_indices, 600);
                NU_Font* node_font = Vector_Get(&__NGUI.stylesheet->fonts, node->fontId);
                NU_Add_Text_Mesh(&clipped_text_vertices, &clipped_text_indices, node);
                NU_Render_Text(&clipped_text_vertices, &clipped_text_indices, node_font, w_fl, h_fl, clip->clip_top, clip->clip_bottom, clip->clip_left, clip->clip_right);
                Vertex_RGB_UV_List_Free(&clipped_text_vertices);
                Index_List_Free(&clipped_text_indices);
            }

            // Draw image
            if (node->glImageHandle) 
            {
                float inner_width  = node->width - node->borderLeft - node->borderRight - node->padLeft - node->padRight;
                float inner_height = node->height - node->borderTop - node->borderBottom - node->padTop - node->padBottom;
                NU_Draw_Image(
                    node->x + node->borderLeft + node->padLeft, 
                    node->y + node->borderTop + node->padTop, 
                    inner_width, inner_height, 
                    w_fl, h_fl, 
                    clip->clip_top, clip->clip_bottom, clip->clip_left, clip->clip_right,
                    node->glImageHandle);
            }
        }
        // --------------------------------------------------------------------------------------------------------
        // --- Draw Relative Positioned Nodes [First Draw Pass] ===================================================
        // --------------------------------------------------------------------------------------------------------















        // --------------------------------------------------------------------------------------------------------
        // --- Draw Absolute Positioned Nodes [Second Draw Pass] ==================================================
        // --------------------------------------------------------------------------------------------------------

        // --- Draw Border Rects ---
        Vertex_RGB_List_Init(&border_rect_vertices, 5000); Index_List_Init(&border_rect_indices, 15000);
        for (int n=0; n<lists->absoluteNodeList.size; n++) { // Construct border rect vertices and indices for each node
            struct Node* node = *(struct Node**) Vector_Get(&lists->absoluteNodeList, n);
            Construct_Border_Rect(node, w_fl, h_fl, &border_rect_vertices, &border_rect_indices);
            if (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL 
                && node->contentHeight > (node->height - node->padTop - node->padBottom - node->borderTop - node->borderBottom)) {
                Construct_Scroll_Thumb(node, w_fl, h_fl, &border_rect_vertices, &border_rect_indices);
            }
        }
        Draw_Vertex_RGB_List(&border_rect_vertices, &border_rect_indices, w_fl, h_fl, 0.0f, 0.0f); // Draw border rects in one call
        Vertex_RGB_List_Free(&border_rect_vertices);
        Index_List_Free(&border_rect_indices);


        // --- Construct Text Meshes & Draw Images ---
        for (int n=0; n<lists->absoluteNodeList.size; n++) 
        {

            // Construct text mesh for node
            struct Node* node = *(struct Node**) Vector_Get(&lists->absoluteNodeList, n);
            if (node->textContent != NULL) {
                Vertex_RGB_UV_List* text_vertices = &text_absolute_vertex_buffers[node->fontId];
                Index_List* text_indices = &text_absolute_index_buffers[node->fontId];
                NU_Add_Text_Mesh(text_vertices, text_indices, node);
            }

            if (node->glImageHandle) // Draw image
            {
                float inner_width  = node->width - node->borderLeft - node->borderRight - node->padLeft - node->padRight;
                float inner_height = node->height - node->borderTop - node->borderBottom - node->padTop - node->padBottom;
                float x = node->x + node->borderLeft + (float)node->padLeft;
                float y = node->y + node->borderTop + (float)node->padTop;
                NU_Draw_Image(
                    x, y, 
                    inner_width, inner_height, 
                    w_fl, h_fl, 
                    -1.0f, 100000.0f, -1.0f, 100000.0f,
                    node->glImageHandle);
            }
        }
        for (uint32_t t=0; t<__NGUI.stylesheet->fonts.size; t++) // Draw text
        {
            Vertex_RGB_UV_List* text_vertices = &text_absolute_vertex_buffers[t];
            Index_List* text_indices = &text_absolute_index_buffers[t];
            NU_Font* node_font = Vector_Get(&__NGUI.stylesheet->fonts, t);
            NU_Render_Text(text_vertices, text_indices, node_font, w_fl, h_fl, -1.0f, 100000.0f, -1.0f, 100000.0f);
            text_vertices->size = 0;
            text_indices->size = 0;
        }

        // --- Draw Partially Visible Nodes ---
        for (int n=0; n<lists->clippedAbsoluteNodeList.size; n++) {
            struct Node* node = *(struct Node**) Vector_Get(&lists->clippedAbsoluteNodeList, n);
            NU_Clip_Bounds* clip = (NU_Clip_Bounds*)HashmapGet(&__NGUI.node_clip_map, &node->clippingRootHandle);

            // Draw border rects
            Vertex_RGB_List vertices;
            Index_List indices;
            Vertex_RGB_List_Init(&vertices, 100);
            Index_List_Init(&indices, 100);
            Construct_Border_Rect(node, w_fl, h_fl, &vertices, &indices);
            Draw_Clipped_Vertex_RGB_List(&vertices, &indices, w_fl, h_fl, 0.0f, 0.0f, clip->clip_top, clip->clip_bottom, clip->clip_left, clip->clip_right);
            Vertex_RGB_List_Free(&vertices);
            Index_List_Free(&indices);

            // Draw text
            if (node->textContent != NULL) 
            {
                Vertex_RGB_UV_List clipped_text_vertices;
                Index_List clipped_text_indices;
                Vertex_RGB_UV_List_Init(&clipped_text_vertices, 1000);
                Index_List_Init(&clipped_text_indices, 600);
                NU_Font* node_font = Vector_Get(&__NGUI.stylesheet->fonts, node->fontId);
                NU_Add_Text_Mesh(&clipped_text_vertices, &clipped_text_indices, node);
                NU_Render_Text(&clipped_text_vertices, &clipped_text_indices, node_font, w_fl, h_fl, clip->clip_top, clip->clip_bottom, clip->clip_left, clip->clip_right);
                Vertex_RGB_UV_List_Free(&clipped_text_vertices);
                Index_List_Free(&clipped_text_indices);
            }

            // Draw image
            if (node->glImageHandle) 
            {
                float inner_width  = node->width - node->borderLeft - node->borderRight - node->padLeft - node->padRight;
                float inner_height = node->height - node->borderTop - node->borderBottom - node->padTop - node->padBottom;
                NU_Draw_Image(
                    node->x + node->borderLeft + node->padLeft, 
                    node->y + node->borderTop + node->padTop, 
                    inner_width, inner_height, 
                    w_fl, h_fl, 
                    clip->clip_top, clip->clip_bottom, clip->clip_left, clip->clip_right,
                    node->glImageHandle);
            }
        }
        // --------------------------------------------------------------------------------------------------------
        // --- Draw Absolute Positioned Nodes [Second Draw Pass] ==================================================
        // --------------------------------------------------------------------------------------------------------



        SDL_GL_SwapWindow(window); 
    }
    
    // -----------------------
    // --- Free memory -------
    // -----------------------
    for (uint32_t i=0; i<window_count; i++) {
        NU_Window_Draw_Lists* lists = Vector_Get(&__NGUI.windowsDrawLists, i);
        Vector_Free(&lists->relativeNodeList);
        Vector_Free(&lists->clippedRelativeNodeList);
        Vector_Free(&lists->absoluteNodeList);
        Vector_Free(&lists->clippedAbsoluteNodeList);
        Vector_Free(&lists->canvasNodeList);
        Vector_Free(&lists->clippedCanvasNodeList);
    }
    for (uint32_t i=0; i<__NGUI.stylesheet->fonts.size; i++) {
        Vertex_RGB_UV_List_Free(&text_relative_vertex_buffers[i]);
        Index_List_Free(&text_relative_index_buffers[i]);
        Vertex_RGB_UV_List_Free(&text_absolute_vertex_buffers[i]);
        Index_List_Free(&text_absolute_index_buffers[i]);
    }

    __NGUI.awaiting_redraw = false;
}

void NU_Reflow()
{
    NU_Clear_Node_Sizes();            // Reset dimensions and positions
    NU_Calculate_Text_Fit_Widths();   // Width and height of unwrapped text nodes
    NU_Calculate_Fit_Size_Widths();   
    NU_Grow_Shrink_Widths();
    NU_Calculate_Table_Column_Widths();
    NU_Calculate_Text_Heights();
    NU_Calculate_Fit_Size_Heights();
    NU_Grow_Shrink_Heights();
    NU_Calculate_Positions();
    NU_Populate_Draw_Lists();
}