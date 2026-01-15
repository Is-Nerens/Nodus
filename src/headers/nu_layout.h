#include <omp.h>
#include <freetype/freetype.h>
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <stdbool.h>
#include <math.h>
#include <rendering/text/nu_text_layout.h>

static void NU_Apply_Min_Max_Size_Constraint(NodeP* node)
{   
    node->node.width = min(max(node->node.width, node->node.minWidth), node->node.maxWidth);
    node->node.height = min(max(node->node.height, node->node.minHeight), node->node.maxHeight);
}

static void NU_Apply_Min_Max_Width_Constraint(NodeP* node)
{
    node->node.width = min(max(node->node.width, node->node.minWidth), node->node.maxWidth);
    node->node.width = max(node->node.width, node->node.preferred_width);
}

static void NU_Apply_Min_Max_Height_Constraint(NodeP* node)
{
    node->node.height = min(max(node->node.height, node->node.minHeight), node->node.maxHeight);
    node->node.height = max(node->node.height, node->node.preferred_height);
}

static void NU_Prepass()
{
    for (int l=0; l<=__NGUI.tree.depth-1; l++)
    {
        Layer* parentlayer = &__NGUI.tree.layers[l];
        Layer* childlayer = &__NGUI.tree.layers[l+1];

        if (parentlayer->nodeArray == NULL) printf("massive issue\n");

        // Iterate over parent layer
        for (int p=0; p<parentlayer->size; p++)
        {       
            NodeP* parent = LayerGet(parentlayer, p); if (parent->state == 0) continue;
            parent->clippingRootHandle = UINT32_MAX;

            // Iterate over children
            for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
            {
                NodeP* child = LayerGet(childlayer, i);

                // Hide child if parent is hidden or child is hidden
                child->state = 1;
                if (parent->node.layoutFlags & HIDDEN || child->node.layoutFlags & HIDDEN) {
                    child->state = 2; 
                    continue;
                }

                // Set/inherit absolute positioning
                child->node.positionAbsolute = 0;
                if (parent->node.positionAbsolute || parent->node.layoutFlags & POSITION_ABSOLUTE || child->node.layoutFlags & POSITION_ABSOLUTE) {
                    child->node.positionAbsolute = 1;
                }

                // Clear clipping inheritance info
                child->clippingRootHandle = UINT32_MAX;

                // Reset position
                child->node.x = 0.0f;
                child->node.y = 0.0f;

                // Constrain preferred/min/max width/height -> These are limited by the border and padding
                // Preferred width height bust also be constrained by min/max width/height
                float natural_width = child->node.borderLeft + child->node.borderRight + child->node.padLeft + child->node.padRight;
                float natural_height = child->node.borderTop + child->node.borderBottom + child->node.padTop + child->node.padBottom;
                child->node.minWidth = max(child->node.minWidth, natural_width);
                child->node.minHeight = max(child->node.minHeight, natural_height);
                child->node.maxWidth = max(max(child->node.maxWidth, natural_width), child->node.minWidth);
                child->node.maxHeight = max(max(child->node.maxHeight, natural_height), child->node.minHeight);
                child->node.preferred_width = max(child->node.preferred_width, natural_width);
                child->node.preferred_height = max(child->node.preferred_height, natural_height);
                child->node.preferred_width = min(max(child->node.preferred_width, child->node.minWidth), child->node.maxWidth);
                child->node.preferred_height = min(max(child->node.preferred_height, child->node.minHeight), child->node.maxHeight);

                // Set base width/height and reset content dimensions
                child->node.width = child->node.preferred_width;
                child->node.height = child->node.preferred_height;
                child->node.contentWidth = 0;
                child->node.contentHeight = 0;
            }
        }
    }
}

static void NU_Calculate_Text_Fit_Widths()
{
    // For each layer
    for (uint32_t l=0; l<=__NGUI.tree.depth-1; l++)
    {
        Layer* layer = &__NGUI.tree.layers[l];

        #pragma omp parallel for
        for (uint32_t n=0; n<layer->size; n++)
        {   
            NodeP* node = &layer->nodeArray[n];
            if (node->state == 0 || node->state == 2) continue;

            if (node->node.textContent != NULL) 
            {
                NU_Font* node_font = Vector_Get(&__NGUI.stylesheet->fonts, node->node.fontId);
                
                // Calculate text width & height
                float text_width = NU_Calculate_Text_Unwrapped_Width(node_font, node->node.textContent);
                
                // Calculate minimum text wrap width (longest unbreakable word)
                float min_wrap_width = NU_Calculate_Text_Min_Wrap_Width(node_font, node->node.textContent);

                // Increase width to account for text (text height will be accounted for later in NU_Calculate_Text_Heights())
                float natural_width = node->node.padLeft + node->node.padRight + node->node.borderLeft + node->node.borderRight;
                node->node.width = max(text_width + natural_width, node->node.preferred_width);

                // Update content width
                node->node.contentWidth = text_width; 
            }
        }
    }
}

static void NU_Calculate_Fit_Size_Widths()
{
    // Traverse the tree bottom-up
    for (int l=__NGUI.tree.depth-2; l>=0; l--)
    {
        Layer* parentlayer = &__NGUI.tree.layers[l];
        Layer* childlayer = &__NGUI.tree.layers[l+1];
        
        // Iterate over parent layer
        for (uint32_t p=0; p<parentlayer->size; p++)
        {       
            NodeP* parent = LayerGet(parentlayer, p);
            if (parent->state == 0 || parent->state == 2) continue;
            int is_layout_horizontal = !(parent->node.layoutFlags & LAYOUT_VERTICAL);

            // If parent is a window -> set dimensions equal to window
            if (parent->type == WINDOW) {
                int window_width, window_height;
                SDL_GetWindowSize(parent->node.window, &window_width, &window_height);
                parent->node.width = (float) window_width;
                parent->node.height = (float) window_height;
            }

            // Skip (no children)
            if (parent->childCount == 0) continue; 

            // Accumulate content width
            int visible_children = 0;
            for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
            {
                NodeP* child = LayerGet(childlayer, i); 

                // Ignore if child is hidden or window or absolutely positioned
                if (child->state == 2 || child->type == WINDOW || child->node.layoutFlags & POSITION_ABSOLUTE) continue; 

                // Horizontal Layout
                if (is_layout_horizontal) parent->node.contentWidth += child->node.width;

                // Vertical Layout 
                else parent->node.contentWidth = MAX(parent->node.contentWidth, child->node.width);

                visible_children++;
            }

            // Expand parent width to account for content width
            if (is_layout_horizontal && visible_children > 0) parent->node.contentWidth += (visible_children - 1) * parent->node.gap;
            if (parent->type != WINDOW && parent->node.contentWidth > parent->node.width) {
                parent->node.width = parent->node.contentWidth + parent->node.borderLeft + parent->node.borderRight + parent->node.padLeft + parent->node.padRight;
                NU_Apply_Min_Max_Width_Constraint(parent);
            }
        }
    }
}

static void NU_Calculate_Fit_Size_Heights()
{
    // Traverse the tree bottom-up
    for (int l=__NGUI.tree.depth-2; l>= 0; l--)
    {
        Layer* parentlayer = &__NGUI.tree.layers[l];
        Layer* childlayer = &__NGUI.tree.layers[l+1];

        // Iterate over parent layer
        for (uint32_t p=0; p<parentlayer->size; p++)
        {       
            NodeP* parent = LayerGet(parentlayer, p);
            if (parent->state == 0 || parent->state == 2) continue;
            int is_layout_horizontal = !(parent->node.layoutFlags & LAYOUT_VERTICAL);

            // Skip (no children)
            if (parent->childCount == 0) continue; 

            // Iterate over children
            int visible_children = 0;
            for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
            {
                NodeP* child = LayerGet(childlayer, i);

                // Ignore if child is hidden or window or absolutely positioned
                if (child->state == 2 || child->type == WINDOW || child->node.layoutFlags & POSITION_ABSOLUTE) continue;

                // Horizontal Layout
                if (is_layout_horizontal) parent->node.contentHeight = MAX(parent->node.contentHeight, child->node.height);
                
                // Vertical Layout 
                else parent->node.contentHeight += child->node.height;

                visible_children++;
            }

            // Expand parent height to account for content height
            if (!is_layout_horizontal && visible_children > 0) parent->node.contentHeight += (visible_children - 1) * parent->node.gap;
            if (parent->type != WINDOW) {
                if (!(parent->node.layoutFlags & OVERFLOW_VERTICAL_SCROLL)) parent->node.height = parent->node.contentHeight + parent->node.borderTop + parent->node.borderBottom + parent->node.padTop + parent->node.padBottom;
                NU_Apply_Min_Max_Height_Constraint(parent);
            }
        }
    }
}

static void NU_Grow_Shrink_Child_Node_Widths(NodeP* parent, Layer* childlayer)
{
    float remaining_width = parent->node.width - parent->node.padLeft - parent->node.padRight - parent->node.borderLeft - parent->node.borderRight;

    // ---------------------------------------------------------------------------------------
    // --- Expand widths of absolute elements if left and right distances are both defined ---
    // ---------------------------------------------------------------------------------------
    for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
    {
        NodeP* child = LayerGet(childlayer, i); if (child->state == 2 || child->type == WINDOW || !(child->node.layoutFlags & POSITION_ABSOLUTE)) continue;
        if (child->node.left > 0.0f && child->node.right > 0.0f) {
            float expanded_width = remaining_width - child->node.left - child->node.right;
            if (expanded_width > child->node.width) child->node.width = expanded_width;
            NU_Apply_Min_Max_Width_Constraint(child);
        }
    }

    remaining_width -= (!!(parent->node.layoutFlags & OVERFLOW_VERTICAL_SCROLL)) * 8.0f;

    // ------------------------------------------------
    // If parent lays out children vertically ---------
    // ------------------------------------------------
    if (parent->node.layoutFlags & LAYOUT_VERTICAL)
    {   
        for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            NodeP* child = LayerGet(childlayer, i); if (child->state == 2 || child->type == WINDOW || child->node.layoutFlags & POSITION_ABSOLUTE) continue;
            if (child->node.layoutFlags & GROW_HORIZONTAL && remaining_width > child->node.width)
            {
                float pad_and_border = child->node.padLeft + child->node.padRight + child->node.borderLeft + child->node.borderRight;
                child->node.width = remaining_width; 
                NU_Apply_Min_Max_Width_Constraint(child);
                parent->node.contentWidth = max(parent->node.contentWidth, child->node.width);
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
        for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            NodeP* child = LayerGet(childlayer, i); if (child->state == 2 || child->type == WINDOW  || child->node.layoutFlags & POSITION_ABSOLUTE) continue;
            else remaining_width -= child->node.width;
            if (child->node.layoutFlags & GROW_HORIZONTAL && child->type != WINDOW) growable_count++;
            visible_children++;
        }
        remaining_width -= (visible_children - 1) * parent->node.gap;
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
            for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
                NodeP* child = LayerGet(childlayer, i); if (child->state == 2 || child->type == WINDOW || child->node.layoutFlags & POSITION_ABSOLUTE) continue;
                if ((child->node.layoutFlags & GROW_HORIZONTAL) && child->node.width < child->node.maxWidth) {
                    growable_count++;
                    if (child->node.width < smallest) {
                        second_smallest = smallest;
                        smallest = child->node.width;
                    } else if (child->node.width < second_smallest) {
                        second_smallest = child->node.width;
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
            for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
                NodeP* child = LayerGet(childlayer, i); if (child->state == 2 || child->type == WINDOW || child->node.layoutFlags & POSITION_ABSOLUTE) continue;        
                if (child->node.layoutFlags & GROW_HORIZONTAL && child->node.width < child->node.maxWidth) { // if child is growable
                    if (child->node.width == smallest) {
                        float available = child->node.maxWidth - child->node.width;
                        float grow = min(width_to_add, available);
                        if (grow > 0.0f) {
                            parent->node.contentWidth += grow;
                            child->node.width += grow;
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
            for (uint32_t i = parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
                NodeP* child = LayerGet(childlayer, i); if (child->state == 2 || child->type == WINDOW || child->node.layoutFlags & POSITION_ABSOLUTE) continue;
                if ((child->node.layoutFlags & GROW_HORIZONTAL) && child->node.width > child->node.minWidth) {
                    shrinkable_count++;
                    if (child->node.width > largest) {
                        second_largest = largest;
                        largest = child->node.width;
                    } else if (child->node.width > second_largest) {
                        second_largest = child->node.width;
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
            for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
                NodeP* child = LayerGet(childlayer, i); if (child->state == 2 || child->type == WINDOW || child->node.layoutFlags & POSITION_ABSOLUTE) continue;
                if ((child->node.layoutFlags & GROW_HORIZONTAL) && child->node.width > child->node.minWidth) {
                    if (child->node.width == largest) {
                        float available = child->node.width - child->node.minWidth;
                        float shrink = min(width_to_subtract, available);
                        if (shrink > 0.0f) {
                            parent->node.contentWidth -= shrink;
                            child->node.width -= shrink;
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

static void NU_Grow_Shrink_Child_Node_Heights(NodeP* parent, Layer* childlayer)
{
    float remaining_height = parent->node.height - parent->node.padTop - parent->node.padBottom - parent->node.borderTop - parent->node.borderBottom;
    
    // ----------------------------------------------------------------------------------------
    // --- Expand heights of absolute elements if top and bottom distances are both defined ---
    // ----------------------------------------------------------------------------------------
    for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
    {
        NodeP* child = LayerGet(childlayer, i); if (child->state == 2 || child->type == WINDOW || !(child->node.layoutFlags & POSITION_ABSOLUTE)) continue;
        if (child->node.top > 0.0f && child->node.bottom > 0.0f) {
            float expanded_height = remaining_height - child->node.top - child->node.bottom;
            if (expanded_height > child->node.height) child->node.height = expanded_height;
            NU_Apply_Min_Max_Height_Constraint(child);
        }
    }

    remaining_height -= (!!(parent->node.layoutFlags & OVERFLOW_HORIZONTAL_SCROLL)) * 8.0f;

    if (!(parent->node.layoutFlags & LAYOUT_VERTICAL))
    {
        for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            NodeP* child = LayerGet(childlayer, i); if (child->state == 2 || child->type == WINDOW || child->node.layoutFlags & POSITION_ABSOLUTE) continue;
            if (child->node.layoutFlags & GROW_VERTICAL && remaining_height > child->node.height)
            {  
                float pad_and_border = child->node.padTop + child->node.padBottom + child->node.borderTop + child->node.borderBottom;
                child->node.height = remaining_height; 
                NU_Apply_Min_Max_Height_Constraint(child);
                parent->node.contentHeight = max(parent->node.contentHeight, child->node.height);
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
        for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            NodeP* child = LayerGet(childlayer, i);
            if (child->state == 2 || child->type == WINDOW || child->node.layoutFlags & POSITION_ABSOLUTE) continue;
            else remaining_height -= child->node.height;
            if (child->node.layoutFlags & GROW_VERTICAL && child->type != WINDOW) growable_count++;
            visible_children++;
        }
        remaining_height -= (visible_children - 1) * parent->node.gap;
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
            for (uint32_t i=parent->firstChildIndex; i< parent->firstChildIndex + parent->childCount; i++) {
                NodeP* child = LayerGet(childlayer, i); if (child->state == 2 || child->type == WINDOW || child->node.layoutFlags & POSITION_ABSOLUTE) continue;
                if ((child->node.layoutFlags & GROW_VERTICAL) && child->node.height < child->node.maxHeight) {
                    growable_count++;
                    if (child->node.height < smallest) {
                        second_smallest = smallest;
                        smallest = child->node.height;
                    } else if (child->node.height < second_smallest) {
                        second_smallest = child->node.height;
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
            for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
                NodeP* child = LayerGet(childlayer, i); if (child->state == 2 || child->type == WINDOW || child->node.layoutFlags & POSITION_ABSOLUTE) continue;
                if (child->node.layoutFlags & GROW_VERTICAL && child->node.height < child->node.maxHeight) { // if child is growable
                    if (child->node.height == smallest) {
                        float available = child->node.maxHeight - child->node.height;
                        float grow = min(height_to_add, available);
                        if (grow > 0.0f) {
                            parent->node.contentHeight += grow;
                            child->node.height += grow;
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
            for (uint32_t i = parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
                NodeP* child = LayerGet(childlayer, i); if (child->state == 2 || child->type == WINDOW || child->node.layoutFlags & POSITION_ABSOLUTE) continue;
                if ((child->node.layoutFlags & GROW_VERTICAL) && child->node.height > child->node.minHeight) {
                    shrinkable_count++;
                    if (child->node.height > largest) {
                        second_largest = largest;
                        largest = child->node.height;
                    } else if (child->node.height > second_largest) {
                        second_largest = child->node.height;
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
            for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
                NodeP* child = LayerGet(childlayer, i); if (child->state == 2 || child->type == WINDOW || child->node.layoutFlags & POSITION_ABSOLUTE) continue;
                if ((child->node.layoutFlags & GROW_VERTICAL) && child->node.height > child->node.minHeight) {
                    if (child->node.height == largest) {
                        float available = child->node.height - child->node.minHeight;
                        float shrink = min(height_to_subtract, available);
                        if (shrink > 0.0f) {
                            parent->node.contentHeight -= shrink;
                            child->node.height -= shrink;
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
    for (uint32_t l=0; l<=__NGUI.tree.depth-1; l++)
    {
        Layer* parentlayer = &__NGUI.tree.layers[l];
        Layer* childlayer = &__NGUI.tree.layers[l+1];

        for (int p=0; p<parentlayer->size; p++)
        {       
            NodeP* parent = LayerGet(parentlayer, p);
            if (parent->state == 0 || parent->state == 2 || parent->type == ROW || parent->type == TABLE) continue;

            NU_Grow_Shrink_Child_Node_Widths(parent, childlayer);
        }
    }
}

static void NU_Grow_Shrink_Heights()
{
    for (uint32_t l=0; l<=__NGUI.tree.depth-1; l++)
    {
        Layer* parentlayer = &__NGUI.tree.layers[l];
        Layer* childlayer = &__NGUI.tree.layers[l+1];

        for (int p=0; p<parentlayer->size; p++)
        {       
            NodeP* parent = LayerGet(parentlayer, p);
            if (parent->state == 0 || parent->state == 2 || parent->type == TABLE) continue;

            NU_Grow_Shrink_Child_Node_Heights(parent, childlayer);
        }
    }
}

static void NU_Calculate_Table_Column_Widths()
{
    for (uint32_t l=0; l<=__NGUI.tree.depth-1; l++)
    {
        Layer* table_layer = &__NGUI.tree.layers[l];

        for (int i=0; i<table_layer->size; i++)
        {       
            NodeP* table = LayerGet(table_layer, i);
            if (table->state == 0 || table->state == 2 || table->type != TABLE || table->childCount == 0) continue;


            struct Vector widest_cell_in_each_column;
            Vector_Reserve(&widest_cell_in_each_column, sizeof(float), 25);
            Layer* row_layer = &__NGUI.tree.layers[l+1];

            // ------------------------------------------------------------
            // --- Calculate the widest cell width in each table column ---
            // ------------------------------------------------------------
            for (uint32_t r=table->firstChildIndex; r<table->firstChildIndex + table->childCount; r++)
            {
                NodeP* row = LayerGet(row_layer, r);
                if (row->state == 2 || row->childCount == 0) break;

                // Iterate over cells in row
                int cell_index = 0;
                Layer* cell_layer = &__NGUI.tree.layers[l+2];
                for (uint32_t c=row->firstChildIndex; c<row->firstChildIndex + row->childCount; c++)
                {
                    // Expand the vector if there are more columns that the vector has capacity for
                    if (cell_index == widest_cell_in_each_column.size) {
                        float val = 0;
                        Vector_Push(&widest_cell_in_each_column, &val);
                    }

                    // Get current column width and update if cell is wider
                    float* val = Vector_Get(&widest_cell_in_each_column, cell_index);
                    NodeP* cell = LayerGet(cell_layer, c);
                    if (cell->state == 2) continue;
                    if (cell->node.width > *val) {
                        *val = cell->node.width;
                    }
                    cell_index++;
                }
            }

            // -----------------------------------------------
            // --- Apply widest column widths to all cells ---
            // -----------------------------------------------
            float table_inner_width = table->node.width - table->node.borderLeft - table->node.borderRight - table->node.padLeft - table->node.padRight - (!!(table->node.layoutFlags & OVERFLOW_VERTICAL_SCROLL)) * 8.0f;
            float remaining_table_inner_width = table_inner_width;
            for (int k=0; k<widest_cell_in_each_column.size; k++) {
                remaining_table_inner_width -= *(float*)Vector_Get(&widest_cell_in_each_column, k);
            }
            float used_table_width = table_inner_width - remaining_table_inner_width;

            // Interate over all the rows in the table
            for (uint32_t r=table->firstChildIndex; r<table->firstChildIndex + table->childCount; r++)
            {
                NodeP* row = LayerGet(row_layer, r);
                if (row->state == 2) continue;
                Layer* cell_layer = &__NGUI.tree.layers[l+2];

                row->node.width = table_inner_width;

                // Reduce available growth space by acounting for row pad, border and child gaps
                float row_border_pad_gap = row->node.borderLeft + row->node.borderRight + row->node.padLeft + row->node.padRight;
                if (row->node.gap != 0.0f) {
                    int visible_cells = 0;
                    for (uint32_t c=row->firstChildIndex; c<row->firstChildIndex + row->childCount; c++) {
                        NodeP* cell = LayerGet(cell_layer, c);
                        if (cell->state == 2) continue;
                        visible_cells++;
                    }
                    row_border_pad_gap += row->node.gap * (visible_cells - 1);
                }

                // Grow the width of all cells 
                int cell_index = 0;
                for (uint32_t c=row->firstChildIndex; c<row->firstChildIndex + row->childCount; c++)
                {
                    NodeP* cell = LayerGet(cell_layer, c);
                    if (cell->state == 2) continue;
                    float column_width = *(float*)Vector_Get(&widest_cell_in_each_column, cell_index);
                    float proportion = column_width / (used_table_width);
                    cell->node.width = column_width + (remaining_table_inner_width - row_border_pad_gap) * proportion;
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
    for (uint32_t l=0; l<=__NGUI.tree.depth-1; l++)
    {
        Layer* layer = &__NGUI.tree.layers[l];

        // Iterate over layer
        for (int i=0; i<layer->size; i++)
        {       
            NodeP* node = &layer->nodeArray[i];
            if (node->state == 0 || node->state == 2) continue;

            if (node->node.textContent != NULL) 
            {
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
        }
    }
}

static void NU_PositionChildrenHorizontally(NodeP* parent, Layer* childlayer)
{
    // layout dir -> top to bottom
    if (parent->node.layoutFlags & LAYOUT_VERTICAL)
    {   
        for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            NodeP* child = LayerGet(childlayer, i); 
            if (child->state == 2 || child->type == WINDOW) continue;

            if (!(child->node.layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float remaning_width = (parent->node.width - parent->node.padLeft - parent->node.padRight - parent->node.borderLeft - parent->node.borderRight) - child->node.width;
                float x_align_offset = remaning_width * 0.5f * (float)parent->node.horizontalAlignment;
                child->node.x = parent->node.x + parent->node.padLeft + parent->node.borderLeft + x_align_offset;
            }
            else { // position absolute
                child->node.x = parent->node.x + parent->node.padLeft + parent->node.borderLeft;
                if (child->node.left > 0.0f) {
                    child->node.x = parent->node.x + child->node.left + parent->node.padLeft + parent->node.borderLeft;
                }
                else if (child->node.right > 0.0f) {
                    float inner_width = parent->node.width - parent->node.padLeft - parent->node.padRight - parent->node.borderLeft - parent->node.borderRight;
                    child->node.x = parent->node.x + inner_width - child->node.width - child->node.right;
                }
            }
        }
    }

    // layout dir -> left to right
    else
    {
        // calculate remaining width (optimise this by caching this value inside parent's content width variable)
        float remainingWidth = (parent->node.width - parent->node.padLeft - parent->node.padRight - parent->node.borderLeft - parent->node.borderRight);
        remainingWidth -= (!!(parent->node.layoutFlags & OVERFLOW_VERTICAL_SCROLL)) * 8.0f;
        int numChildrenAffectingWidth = 0;
        for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
            NodeP* child = LayerGet(childlayer, i);
            if (child->state == 2 || child->type == WINDOW || child->node.layoutFlags & POSITION_ABSOLUTE) continue;
            remainingWidth -= child->node.width; numChildrenAffectingWidth++;
        }
        remainingWidth -= parent->node.gap * (numChildrenAffectingWidth - 1);



        // position children horizontally
        float cursorX = 0.0f;
        for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            NodeP* child = LayerGet(childlayer, i); 
            if (child->state == 2 || child->type == WINDOW) continue;
            
            if (!(child->node.layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float x_align_offset = remainingWidth * 0.5f * (float)parent->node.horizontalAlignment;
                child->node.x = parent->node.x + parent->node.padLeft + parent->node.borderLeft + cursorX + x_align_offset;
                cursorX += child->node.width + parent->node.gap;
            }
            else { // position absolute 
                child->node.x = parent->node.x + parent->node.padLeft + parent->node.borderLeft;
                if (child->node.left > 0.0f) {
                    child->node.x = parent->node.x + child->node.left + parent->node.padLeft + parent->node.borderLeft;
                }
                else if (child->node.right > 0.0f) {
                    float inner_width = parent->node.width - parent->node.padLeft - parent->node.padRight - parent->node.borderLeft - parent->node.borderRight;
                    child->node.x = parent->node.x + inner_width - child->node.width - child->node.right;
                }
            }
        }
    }
}

static void NU_PositionChildrenVertically(NodeP* parent, Layer* childlayer)
{
    float y_scroll_offset = 0.0f;
    if (parent->node.layoutFlags & OVERFLOW_VERTICAL_SCROLL && 
        parent->childCount > 0 && 
        parent->node.contentHeight > parent->node.height - parent->node.padTop - parent->node.padBottom - parent->node.borderTop - parent->node.borderBottom) 
    {
        float track_h = parent->node.height - parent->node.borderTop - parent->node.borderBottom;
        float inner_height_w_pad = track_h - parent->node.padTop - parent->node.padBottom;
        float inner_proportion_of_content_height = inner_height_w_pad / parent->node.contentHeight;
        float thumb_h = inner_proportion_of_content_height * track_h;
        float content_scroll_range = parent->node.contentHeight - inner_height_w_pad;
        float thumb_scroll_range = track_h - thumb_h;
        float scroll_factor = content_scroll_range / max(thumb_scroll_range, 1.0f);
        y_scroll_offset += (-parent->node.scrollV * (track_h - thumb_h)) * scroll_factor;

        // undo effect of scroll offset for table header row
        NodeP* first_child = LayerGet(childlayer, parent->firstChildIndex);
        if (first_child->state != 2 && first_child->type == THEAD) {
            first_child->node.y -= y_scroll_offset;
        }
    }

    // layout dir -> left to right
    if (!(parent->node.layoutFlags & LAYOUT_VERTICAL))
    {   
        for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            NodeP* child = LayerGet(childlayer, i); 
            if (child->state == 2 || child->type == WINDOW) continue;

            if (!(child->node.layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float remaining_height = (parent->node.height - parent->node.padTop - parent->node.padBottom - parent->node.borderTop - parent->node.borderBottom) - child->node.height;
                float y_align_offset = remaining_height * 0.5f * (float)parent->node.verticalAlignment;
                child->node.y += parent->node.y + parent->node.padTop + parent->node.borderTop + y_align_offset + y_scroll_offset;
            }
            else { // position absolute 
                child->node.y = parent->node.y + parent->node.padTop + parent->node.borderTop;
                if (child->node.top > 0.0f) {
                    child->node.y = parent->node.y + child->node.top + parent->node.padTop + parent->node.borderTop;
                }
                else if (child->node.bottom > 0.0f) {
                    float inner_height = parent->node.height - parent->node.padTop - parent->node.padBottom - parent->node.borderTop - parent->node.borderBottom;
                    child->node.y = parent->node.y + inner_height - child->node.height - child->node.bottom;
                }
            }
        }
    }

    // layout dir -> top to bottom
    else
    {
        // calculate remaining height (optimise this by caching this value inside parent's content height variable)
        float remainingHeight = (parent->node.height - parent->node.padTop - parent->node.padBottom - parent->node.borderTop - parent->node.borderBottom);
        remainingHeight -= (!!(parent->node.layoutFlags & OVERFLOW_HORIZONTAL_SCROLL)) * 8.0f;
        int numChildrenAffectingHeight = 0;
        for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
            NodeP* child = LayerGet(childlayer, i);
            if (child->state == 2 || child->type == WINDOW || child->node.layoutFlags & POSITION_ABSOLUTE) continue;
            remainingHeight -= child->node.height; numChildrenAffectingHeight++;
        }
        remainingHeight -= parent->node.gap * (numChildrenAffectingHeight - 1);



        // position children vertically
        float cursorY = 0.0f;
        for (uint32_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            NodeP* child = LayerGet(childlayer, i);
            if (child->state == 2 || child->type == WINDOW) continue;

            if (!(child->node.layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float y_align_offset = remainingHeight * 0.5f * (float)parent->node.verticalAlignment;
                child->node.y += parent->node.y + parent->node.padTop + parent->node.borderTop + cursorY + y_align_offset + y_scroll_offset;
                cursorY += child->node.height + parent->node.gap;
            }
            else { // position abosolute
                child->node.y = parent->node.y + parent->node.padTop + parent->node.borderTop;
                if (child->node.top > 0.0f) {
                    child->node.y = parent->node.y + child->node.top + parent->node.padTop + parent->node.borderTop;
                }
                else if (child->node.bottom > 0.0f) {
                    float inner_height = parent->node.height - parent->node.padTop - parent->node.padBottom - parent->node.borderTop - parent->node.borderBottom;
                    child->node.y = parent->node.y + inner_height - child->node.height - child->node.bottom;
                }
            }
        }
    }
}

static void NU_Calculate_Positions()
{
    for (uint32_t l=0; l<=__NGUI.tree.depth-1; l++) 
    {
        Layer* parentlayer = &__NGUI.tree.layers[l];
        Layer* childlayer = &__NGUI.tree.layers[l+1];
        
        // Iterate over parent layer
        for (uint32_t p=0; p<parentlayer->size; p++) // For node in layer
        {   
            NodeP* parent = LayerGet(parentlayer, p);
            if (parent->state == 0 || parent->state == 2) continue;

            if (parent->type == WINDOW)
            {
                parent->node.x = 0;
                parent->node.y = 0;
            }

            NU_PositionChildrenHorizontally(parent, childlayer);
            NU_PositionChildrenVertically(parent, childlayer);
        }
    }
}

void NU_Layout()
{
    NU_Prepass();
    NU_Calculate_Text_Fit_Widths();
    NU_Calculate_Fit_Size_Widths();  
    NU_Grow_Shrink_Widths();
    NU_Calculate_Table_Column_Widths();
    NU_Calculate_Text_Heights();
    NU_Calculate_Fit_Size_Heights();
    NU_Grow_Shrink_Heights();
    NU_Calculate_Positions();
}







static bool NU_Mouse_Over_Node(NodeP* node, float mouse_x, float mouse_y)
{
    // --- Get clipping info
    float left_wall = node->node.x;
    float right_wall = node->node.x + node->node.width;
    float top_wall = node->node.y;
    float bottom_wall = node->node.y + node->node.height; 
    if (node->clippingRootHandle != UINT32_MAX)
    {
        NU_ClipBounds* clip = HashmapGet(&__NGUI.winManager.clipMap, &node->clippingRootHandle);
        left_wall = max(clip->clip_left, node->node.x);
        right_wall = min(clip->clip_right, node->node.x + node->node.width);
        top_wall = max(clip->clip_top, node->node.y);
        bottom_wall = min(clip->clip_bottom, node->node.y + node->node.height);
    }

    // --- Check if mouse is within clipped bounding box
    bool within_x_bound = mouse_x >= left_wall && mouse_x <= right_wall;
    bool within_y_bound = mouse_y >= top_wall && mouse_y <= bottom_wall;
    if (!(within_x_bound && within_y_bound)) return false; // Not in bounding rect

    // --- Constrain border radii ---
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

    // --- Rounded border anchors ---
    vec2 tl_a = { floorf(node->node.x + borderRadiusTl),               floorf(node->node.y + borderRadiusTl) };
    vec2 tr_a = { floorf(node->node.x + node->node.width - borderRadiusTr), floorf(node->node.y + borderRadiusTr) };
    vec2 bl_a = { floorf(node->node.x + borderRadiusBl),               floorf(node->node.y + node->node.height - borderRadiusBl) };
    vec2 br_a = { floorf(node->node.x + node->node.width - borderRadiusBr), floorf(node->node.y + node->node.height - borderRadiusBr) };


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
    // -----------------------------------------------------------
    // --- Remove potential pseudo style from current hovered node
    // -----------------------------------------------------------
    if (__NGUI.hovered_node != UINT32_MAX && __NGUI.hovered_node != __NGUI.mouse_down_node) {
        NU_Apply_Stylesheet_To_Node(NODE_P(__NGUI.hovered_node), __NGUI.stylesheet);
    }
    uint32_t prev_hovered_node = __NGUI.hovered_node;
    __NGUI.hovered_node = UINT32_MAX;
    __NGUI.scroll_hovered_node = UINT32_MAX;

    if (__NGUI.winManager.hoveredWindow == NULL) return;

    // -----------------------------
    // --- Get local mouse position
    // -----------------------------
    float mouseX, mouseY; GetLocalMouseCoords(&__NGUI.winManager, &mouseX, &mouseY);

    // ----------------------------
    // --- Create a traversal stack
    // ----------------------------
    struct Vector stack;
    Vector_Reserve(&stack, sizeof(NodeP*), 32);
    
    // -----------------------------------------------
    // --- Push all the nodes to start traversing from
    // -----------------------------------------------
    for (uint32_t i=0; i<__NGUI.winManager.absoluteRootNodes.size; i++) {
        NodeP* absolute_node = *(NodeP**)Vector_Get(&__NGUI.winManager.absoluteRootNodes, i);
        if (absolute_node->node.window == __NGUI.winManager.hoveredWindow && NU_Mouse_Over_Node(absolute_node, mouseX, mouseY)) {
            Vector_Push(&stack, &absolute_node);
        }
    }

    for (uint32_t i=0; i<__NGUI.winManager.windowNodes.size; i++) {
        uint32_t handle = *(uint32_t*)Vector_Get(&__NGUI.winManager.windowNodes, i);
        NodeP* node = NODE_P(handle);
        if (node->node.window == __NGUI.winManager.hoveredWindow){
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
        NodeP* current_node = *(NodeP**)Vector_Get(&stack, stack.size - 1);
        __NGUI.hovered_node = current_node->handle;
        stack.size -= 1;

        if (current_node->type == BUTTON) continue; // Skip children

        // ------------------------------
        // --- Iterate over children
        // ------------------------------
        Layer* childlayer = &__NGUI.tree.layers[current_node->layer+1];
        for (uint32_t i=current_node->firstChildIndex; i<current_node->firstChildIndex + current_node->childCount; i++)
        {
            NodeP* child = LayerGet(childlayer, i);
            if (child->state == 2 || 
                child->node.layoutFlags & POSITION_ABSOLUTE || 
                child->type == WINDOW ||
                !NU_Mouse_Over_Node(child, mouseX, mouseY)) continue; // Skip

            // Check for scroll hover
            if (child->node.layoutFlags & OVERFLOW_V_PROPERTY) {
                bool overflow_v = child->node.contentHeight > child->node.height - child->node.borderTop - child->node.borderBottom;
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
        NU_Apply_Pseudo_Style_To_Node(NODE_P(__NGUI.hovered_node), __NGUI.stylesheet, PSEUDO_HOVER);
    } 
    Vector_Free(&stack);

    // If hovered node change -> must redraw later
    if (prev_hovered_node != __NGUI.hovered_node) {
        __NGUI.awaiting_redraw = true;
    }
}