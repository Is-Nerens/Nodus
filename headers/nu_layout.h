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
    node->width = min(max(node->width, node->min_width), node->max_width);
    node->height = min(max(node->height, node->min_height), node->max_height);
}

static void NU_Apply_Min_Max_Width_Constraint(struct Node* node)
{
    node->width = min(max(node->width, node->min_width), node->max_width);
    node->width = max(node->width, node->preferred_width);
}

static void NU_Apply_Min_Max_Height_Constraint(struct Node* node)
{
    node->height = min(max(node->height, node->min_height), node->max_height);
    node->height = max(node->height, node->preferred_height);
}

static void NU_Clear_Node_Sizes()
{
    for (int l=0; l<=__nu_global_gui.deepest_layer; l++)
    {
        NU_Layer* parent_layer = &__nu_global_gui.tree.layers[l];
        NU_Layer* child_layer = &__nu_global_gui.tree.layers[l+1];

        // Iterate over parent layer
        for (int p=0; p<parent_layer->size; p++)
        {       
            struct Node* parent = NU_Layer_Get(parent_layer, p); if (parent->node_present == 0) continue;
            parent->clipping_root_handle = UINT32_MAX;

            // Iterate over children
            for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
            {
                struct Node* child = NU_Layer_Get(child_layer, i);

                // Hide child if parent is hidden or child is hidden
                child->node_present = 1;
                if (parent->layout_flags & HIDDEN || child->layout_flags & HIDDEN) {
                    child->node_present = 2; 
                    continue;
                }

                // Set/inherit absolute positioning
                child->position_absolute = 0;
                if (parent->position_absolute || parent->layout_flags & POSITION_ABSOLUTE || child->layout_flags & POSITION_ABSOLUTE) {
                    child->position_absolute = 1;
                }

                // Clear clipping inheritance info
                child->clipping_root_handle = UINT32_MAX;

                // Reset position
                child->x = 0.0f;
                child->y = 0.0f;

                // Constrain preferred/min/max width/height -> These are limited by the border and padding
                // Preferred width height bust also be constrained by min/max width/height
                float natural_width = child->border_left + child->border_right + child->pad_left + child->pad_right;
                float natural_height = child->border_top + child->border_bottom + child->pad_top + child->pad_bottom;
                child->min_width = max(child->min_width, natural_width);
                child->min_height = max(child->min_height, natural_height);
                child->max_width = max(max(child->max_width, natural_width), child->min_width);
                child->max_height = max(max(child->max_height, natural_height), child->min_height);
                child->preferred_width = max(child->preferred_width, natural_width);
                child->preferred_height = max(child->preferred_height, natural_height);
                child->preferred_width = min(max(child->preferred_width, child->min_width), child->max_width);
                child->preferred_height = min(max(child->preferred_height, child->min_height), child->max_height);

                // Set base width/height and reset content dimensions
                child->width = child->preferred_width;
                child->height = child->preferred_height;
                child->content_width = 0;
                child->content_height = 0;
            }
        }
    }
}

static void NU_Calculate_Text_Fit_Widths()
{
    // For each layer
    for (uint16_t l=0; l<=__nu_global_gui.deepest_layer; l++)
    {
        NU_Layer* layer = &__nu_global_gui.tree.layers[l];

        #pragma omp parallel for
        for (uint32_t n=0; n<layer->size; n++)
        {   
            struct Node* node = &layer->node_array[n];
            if (node->node_present == 0 || node->node_present == 2) continue;

            if (node->text_content != NULL) 
            {
                NU_Font* node_font = Vector_Get(&__nu_global_gui.stylesheet->fonts, node->font_id);
                
                // Calculate text width & height
                float text_width = NU_Calculate_Text_Unwrapped_Width(node_font, node->text_content);
                
                // Calculate minimum text wrap width (longest unbreakable word)
                float min_wrap_width = NU_Calculate_Text_Min_Wrap_Width(node_font, node->text_content);

                // Increase width to account for text (text height will be accounted for later in NU_Calculate_Text_Heights())
                float natural_width = node->pad_left + node->pad_right + node->border_left + node->border_right;
                node->width = max(text_width + natural_width, node->preferred_width);

                // Update content width
                node->content_width = text_width; 
            }
        }
    }
}

static void NU_Calculate_Fit_Size_Widths()
{
    // Traverse the tree bottom-up
    for (int l=__nu_global_gui.deepest_layer-1; l>=0; l--)
    {
        NU_Layer* parent_layer = &__nu_global_gui.tree.layers[l];
        NU_Layer* child_layer = &__nu_global_gui.tree.layers[l+1];
        
        // Iterate over parent layer
        for (uint32_t p=0; p<parent_layer->size; p++)
        {       
            struct Node* parent = NU_Layer_Get(parent_layer, p);
            if (parent->node_present == 0 || parent->node_present == 2) continue;
            int is_layout_horizontal = !(parent->layout_flags & LAYOUT_VERTICAL);

            // If parent is a window -> set dimensions equal to window
            if (parent->tag == WINDOW) {
                int window_width, window_height;
                SDL_GetWindowSize(parent->window, &window_width, &window_height);
                parent->width = (float) window_width;
                parent->height = (float) window_height;
            }

            // Skip (no children)
            if (parent->child_count == 0) continue; 

            // Accumulate content width
            int visible_children = 0;
            for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
            {
                struct Node* child = NU_Layer_Get(child_layer, i); 

                // Ignore if child is hidden or window or absolutely positioned
                if (child->node_present == 2 || child->tag == WINDOW || child->layout_flags & POSITION_ABSOLUTE) continue; 

                // Horizontal Layout
                if (is_layout_horizontal) parent->content_width += child->width;

                // Vertical Layout 
                else parent->content_width = MAX(parent->content_width, child->width);

                visible_children++;
            }

            // Expand parent width to account for content width
            if (is_layout_horizontal && visible_children > 0) parent->content_width += (visible_children - 1) * parent->gap;
            if (parent->tag != WINDOW && parent->content_width > parent->width) {
                parent->width = parent->content_width + parent->border_left + parent->border_right + parent->pad_left + parent->pad_right;
                NU_Apply_Min_Max_Width_Constraint(parent);
            }
        }
    }
}

static void NU_Calculate_Fit_Size_Heights()
{
    // Traverse the tree bottom-up
    for (int l=__nu_global_gui.deepest_layer-1; l>= 0; l--)
    {
        NU_Layer* parent_layer = &__nu_global_gui.tree.layers[l];
        NU_Layer* child_layer = &__nu_global_gui.tree.layers[l+1];

        // Iterate over parent layer
        for (uint32_t p=0; p<parent_layer->size; p++)
        {       
            struct Node* parent = NU_Layer_Get(parent_layer, p);
            if (parent->node_present == 0 || parent->node_present == 2) continue;
            int is_layout_horizontal = !(parent->layout_flags & LAYOUT_VERTICAL);

            // Skip (no children)
            if (parent->child_count == 0) continue; 

            // Iterate over children
            int visible_children = 0;
            for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
            {
                struct Node* child = NU_Layer_Get(child_layer, i);

                // Ignore if child is hidden or window or absolutely positioned
                if (child->node_present == 2 || child->tag == WINDOW || child->layout_flags & POSITION_ABSOLUTE) continue;

                // Horizontal Layout
                if (is_layout_horizontal) parent->content_height = MAX(parent->content_height, child->height);
                
                // Vertical Layout 
                else parent->content_height += child->height;

                visible_children++;
            }

            // Expand parent height to account for content height
            if (!is_layout_horizontal && visible_children > 0) parent->content_height += (visible_children - 1) * parent->gap;
            if (parent->tag != WINDOW) {
                if (!(parent->layout_flags & OVERFLOW_VERTICAL_SCROLL)) parent->height = parent->content_height + parent->border_top + parent->border_bottom + parent->pad_top + parent->pad_bottom;
                NU_Apply_Min_Max_Height_Constraint(parent);
            }
        }
    }
}

static void NU_Grow_Shrink_Child_Node_Widths(struct Node* parent, NU_Layer* child_layer)
{
    float remaining_width = parent->width - parent->pad_left - parent->pad_right - parent->border_left - parent->border_right;

    // ---------------------------------------------------------------------------------------
    // --- Expand widths of absolute elements if left and right distances are both defined ---
    // ---------------------------------------------------------------------------------------
    for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
    {
        struct Node* child = NU_Layer_Get(child_layer, i); if (child->node_present == 2 || child->tag == WINDOW || !(child->layout_flags & POSITION_ABSOLUTE)) continue;
        if (child->left > 0.0f && child->right > 0.0f) {
            float expanded_width = remaining_width - child->left - child->right;
            if (expanded_width > child->width) child->width = expanded_width;
            NU_Apply_Min_Max_Width_Constraint(child);
        }
    }

    remaining_width -= (!!(parent->layout_flags & OVERFLOW_VERTICAL_SCROLL)) * 8.0f;

    // ------------------------------------------------
    // If parent lays out children vertically ---------
    // ------------------------------------------------
    if (parent->layout_flags & LAYOUT_VERTICAL)
    {   
        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i); if (child->node_present == 2 || child->tag == WINDOW || child->layout_flags & POSITION_ABSOLUTE) continue;
            if (child->layout_flags & GROW_HORIZONTAL && remaining_width > child->width)
            {
                float pad_and_border = child->pad_left + child->pad_right + child->border_left + child->border_right;
                child->width = remaining_width; 
                NU_Apply_Min_Max_Width_Constraint(child);
                parent->content_width = max(parent->content_width, child->width);
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
        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i); if (child->node_present == 2 || child->tag == WINDOW  || child->layout_flags & POSITION_ABSOLUTE) continue;
            else remaining_width -= child->width;
            if (child->layout_flags & GROW_HORIZONTAL && child->tag != WINDOW) growable_count++;
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
            for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++) {
                struct Node* child = NU_Layer_Get(child_layer, i); if (child->node_present == 2 || child->tag == WINDOW || child->layout_flags & POSITION_ABSOLUTE) continue;
                if ((child->layout_flags & GROW_HORIZONTAL) && child->width < child->max_width) {
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
            for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++) {
                struct Node* child = NU_Layer_Get(child_layer, i); if (child->node_present == 2 || child->tag == WINDOW || child->layout_flags & POSITION_ABSOLUTE) continue;        
                if (child->layout_flags & GROW_HORIZONTAL && child->width < child->max_width) { // if child is growable
                    if (child->width == smallest) {
                        float available = child->max_width - child->width;
                        float grow = min(width_to_add, available);
                        if (grow > 0.0f) {
                            parent->content_width += grow;
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
            for (uint16_t i = parent->first_child_index; i<parent->first_child_index + parent->child_count; i++) {
                struct Node* child = NU_Layer_Get(child_layer, i); if (child->node_present == 2 || child->tag == WINDOW || child->layout_flags & POSITION_ABSOLUTE) continue;
                if ((child->layout_flags & GROW_HORIZONTAL) && child->width > child->min_width) {
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
            for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++) {
                struct Node* child = NU_Layer_Get(child_layer, i); if (child->node_present == 2 || child->tag == WINDOW || child->layout_flags & POSITION_ABSOLUTE) continue;
                if ((child->layout_flags & GROW_HORIZONTAL) && child->width > child->min_width) {
                    if (child->width == largest) {
                        float available = child->width - child->min_width;
                        float shrink = min(width_to_subtract, available);
                        if (shrink > 0.0f) {
                            parent->content_width -= shrink;
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
    float remaining_height = parent->height - parent->pad_top - parent->pad_bottom - parent->border_top - parent->border_bottom;
    
    // ----------------------------------------------------------------------------------------
    // --- Expand heights of absolute elements if top and bottom distances are both defined ---
    // ----------------------------------------------------------------------------------------
    for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
    {
        struct Node* child = NU_Layer_Get(child_layer, i); if (child->node_present == 2 || child->tag == WINDOW || !(child->layout_flags & POSITION_ABSOLUTE)) continue;
        if (child->top > 0.0f && child->bottom > 0.0f) {
            float expanded_height = remaining_height - child->top - child->bottom;
            if (expanded_height > child->height) child->height = expanded_height;
            NU_Apply_Min_Max_Height_Constraint(child);
        }
    }

    remaining_height -= (!!(parent->layout_flags & OVERFLOW_HORIZONTAL_SCROLL)) * 8.0f;

    if (!(parent->layout_flags & LAYOUT_VERTICAL))
    {
        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i); if (child->node_present == 2 || child->tag == WINDOW || child->layout_flags & POSITION_ABSOLUTE) continue;
            if (child->layout_flags & GROW_VERTICAL && remaining_height > child->height)
            {  
                float pad_and_border = child->pad_top + child->pad_bottom + child->border_top + child->border_bottom;
                child->height = remaining_height; 
                NU_Apply_Min_Max_Height_Constraint(child);
                parent->content_height = max(parent->content_height, child->height);
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
        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->node_present == 2 || child->tag == WINDOW || child->layout_flags & POSITION_ABSOLUTE) continue;
            else remaining_height -= child->height;
            if (child->layout_flags & GROW_VERTICAL && child->tag != WINDOW) growable_count++;
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
            for (uint16_t i=parent->first_child_index; i< parent->first_child_index + parent->child_count; i++) {
                struct Node* child = NU_Layer_Get(child_layer, i); if (child->node_present == 2 || child->tag == WINDOW || child->layout_flags & POSITION_ABSOLUTE) continue;
                if ((child->layout_flags & GROW_VERTICAL) && child->height < child->max_height) {
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
            for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++) {
                struct Node* child = NU_Layer_Get(child_layer, i); if (child->node_present == 2 || child->tag == WINDOW || child->layout_flags & POSITION_ABSOLUTE) continue;
                if (child->layout_flags & GROW_VERTICAL && child->height < child->max_height) { // if child is growable
                    if (child->height == smallest) {
                        float available = child->max_height - child->height;
                        float grow = min(height_to_add, available);
                        if (grow > 0.0f) {
                            parent->content_height += grow;
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
            for (uint16_t i = parent->first_child_index; i<parent->first_child_index + parent->child_count; i++) {
                struct Node* child = NU_Layer_Get(child_layer, i); if (child->node_present == 2 || child->tag == WINDOW || child->layout_flags & POSITION_ABSOLUTE) continue;
                if ((child->layout_flags & GROW_VERTICAL) && child->height > child->min_height) {
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
            for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++) {
                struct Node* child = NU_Layer_Get(child_layer, i); if (child->node_present == 2 || child->tag == WINDOW || child->layout_flags & POSITION_ABSOLUTE) continue;
                if ((child->layout_flags & GROW_VERTICAL) && child->height > child->min_height) {
                    if (child->height == largest) {
                        float available = child->height - child->min_height;
                        float shrink = min(height_to_subtract, available);
                        if (shrink > 0.0f) {
                            parent->content_height -= shrink;
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
    for (uint16_t l=0; l<=__nu_global_gui.deepest_layer; l++)
    {
        NU_Layer* parent_layer = &__nu_global_gui.tree.layers[l];
        NU_Layer* child_layer = &__nu_global_gui.tree.layers[l+1];

        for (int p=0; p<parent_layer->size; p++)
        {       
            struct Node* parent = NU_Layer_Get(parent_layer, p);
            if (parent->node_present == 0 || parent->node_present == 2 || parent->tag == ROW || parent->tag == TABLE) continue;

            NU_Grow_Shrink_Child_Node_Widths(parent, child_layer);
        }
    }
}

static void NU_Grow_Shrink_Heights()
{
    for (uint16_t l=0; l<=__nu_global_gui.deepest_layer; l++)
    {
        NU_Layer* parent_layer = &__nu_global_gui.tree.layers[l];
        NU_Layer* child_layer = &__nu_global_gui.tree.layers[l+1];

        for (int p=0; p<parent_layer->size; p++)
        {       
            struct Node* parent = NU_Layer_Get(parent_layer, p);
            if (parent->node_present == 0 || parent->node_present == 2 || parent->tag == TABLE) continue;

            NU_Grow_Shrink_Child_Node_Heights(parent, child_layer);
        }
    }
}

static void NU_Calculate_Table_Column_Widths()
{
    for (uint16_t l=0; l<=__nu_global_gui.deepest_layer; l++)
    {
        NU_Layer* table_layer = &__nu_global_gui.tree.layers[l];

        for (int i=0; i<table_layer->size; i++)
        {       
            struct Node* table = NU_Layer_Get(table_layer, i);
            if (table->node_present == 0 || table->node_present == 2 || table->tag != TABLE || table->child_count == 0) continue;


            struct Vector widest_cell_in_each_column;
            Vector_Reserve(&widest_cell_in_each_column, sizeof(float), 25);
            NU_Layer* row_layer = &__nu_global_gui.tree.layers[l+1];

            // ------------------------------------------------------------
            // --- Calculate the widest cell width in each table column ---
            // ------------------------------------------------------------
            for (uint16_t r=table->first_child_index; r<table->first_child_index + table->child_count; r++)
            {
                struct Node* row = NU_Layer_Get(row_layer, r);
                if (row->node_present == 2 || row->child_count == 0) break;

                // Iterate over cells in row
                int cell_index = 0;
                NU_Layer* cell_layer = &__nu_global_gui.tree.layers[l+2];
                for (uint16_t c=row->first_child_index; c<row->first_child_index + row->child_count; c++)
                {
                    // Expand the vector if there are more columns that the vector has capacity for
                    if (cell_index == widest_cell_in_each_column.size) {
                        float val = 0;
                        Vector_Push(&widest_cell_in_each_column, &val);
                    }

                    // Get current column width and update if cell is wider
                    float* val = Vector_Get(&widest_cell_in_each_column, cell_index);
                    struct Node* cell = NU_Layer_Get(cell_layer, c);
                    if (cell->node_present == 2) continue;
                    if (cell->width > *val) {
                        *val = cell->width;
                    }
                    cell_index++;
                }
            }

            // -----------------------------------------------
            // --- Apply widest column widths to all cells ---
            // -----------------------------------------------
            float table_inner_width = table->width - table->border_left - table->border_right - table->pad_left - table->pad_right - (!!(table->layout_flags & OVERFLOW_VERTICAL_SCROLL)) * 8.0f;
            float remaining_table_inner_width = table_inner_width;
            for (int k=0; k<widest_cell_in_each_column.size; k++) {
                remaining_table_inner_width -= *(float*)Vector_Get(&widest_cell_in_each_column, k);
            }
            float used_table_width = table_inner_width - remaining_table_inner_width;

            // Interate over all the rows in the table
            for (uint16_t r=table->first_child_index; r<table->first_child_index + table->child_count; r++)
            {
                struct Node* row = NU_Layer_Get(row_layer, r);
                if (row->node_present == 2) continue;
                NU_Layer* cell_layer = &__nu_global_gui.tree.layers[l+2];

                row->width = table_inner_width;

                // Reduce available growth space by acounting for row pad, border and child gaps
                float row_border_pad_gap = row->border_left + row->border_right + row->pad_left + row->pad_right;
                if (row->gap != 0.0f) {
                    int visible_cells = 0;
                    for (uint16_t c=row->first_child_index; c<row->first_child_index + row->child_count; c++) {
                        struct Node* cell = NU_Layer_Get(cell_layer, c);
                        if (cell->node_present == 2) continue;
                        visible_cells++;
                    }
                    row_border_pad_gap += row->gap * (visible_cells - 1);
                }

                // Grow the width of all cells 
                int cell_index = 0;
                for (uint16_t c=row->first_child_index; c<row->first_child_index + row->child_count; c++)
                {
                    struct Node* cell = NU_Layer_Get(cell_layer, c);
                    if (cell->node_present == 2) continue;
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
    for (uint16_t l=0; l<=__nu_global_gui.deepest_layer; l++)
    {
        NU_Layer* layer = &__nu_global_gui.tree.layers[l];

        // Iterate over layer
        for (int i=0; i<layer->size; i++)
        {       
            struct Node* node = &layer->node_array[i];
            if (node->node_present == 0 || node->node_present == 2) continue;

            if (node->text_content != NULL) 
            {
                NU_Font* node_font = Vector_Get(&__nu_global_gui.stylesheet->fonts, node->font_id);

                // Compute available inner width
                float inner_width = node->width - node->border_left - node->border_right - node->pad_left - node->pad_right;

                // Calculate text height
                float text_height = NU_Calculate_FreeText_Height_From_Wrap_Width(node_font, node->text_content, inner_width);

                // Increase height to account for text
                float natural_height = node->pad_top + node->pad_bottom + node->border_top + node->border_bottom;
                node->height = max(text_height + natural_height, node->preferred_height);
                
                // Update content height
                node->content_height = text_height;
            }
        }
    }
}

static void NU_Horizontally_Place_Children(struct Node* parent, NU_Layer* child_layer)
{
    // Children placed top to bottom
    if (parent->layout_flags & LAYOUT_VERTICAL)
    {   
        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->node_present == 2) continue;

            // Position absolute
            if (child->layout_flags & POSITION_ABSOLUTE) 
            {
                child->x = parent->x + parent->pad_left + parent->border_left;
                if (child->left > 0.0f) {
                    child->x = parent->x + child->left + parent->pad_left + parent->border_left;
                }
                else if (child->right > 0.0f) {
                    float inner_width = parent->width - parent->pad_left - parent->pad_right - parent->border_left - parent->border_right;
                    child->x = parent->x + inner_width - child->width - child->right;
                }
            }

            // Position relative
            else
            {
                float remaning_width = (parent->width - parent->pad_left - parent->pad_right - parent->border_left - parent->border_right) - child->width;
                float x_align_offset = remaning_width * 0.5f * (float)parent->horizontal_alignment;
                child->x = parent->x + parent->pad_left + parent->border_left + x_align_offset;
            }
        }
    }

    // Children placed left to right
    else
    {
        // Calculate remaining width (optimise this by caching this value inside parent's content width variable)
        float remaining_width = (parent->width - parent->pad_left - parent->pad_right - parent->border_left - parent->border_right) - (parent->child_count - 1) * parent->gap - (!!(parent->layout_flags & OVERFLOW_VERTICAL_SCROLL)) * 8.0f;
        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->node_present == 2) continue;
            if (child->layout_flags & POSITION_ABSOLUTE) continue; // Skip because absolute child doen't affect flow
            if (child->tag == WINDOW) remaining_width += parent->gap;
            else remaining_width -= child->width;
        }

        float cursor_x = 0.0f;

        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->node_present == 2) continue;
            
            // Position absolute
            if (child->layout_flags & POSITION_ABSOLUTE) 
            {
                child->x = parent->x + parent->pad_left + parent->border_left;
                if (child->left > 0.0f) {
                    child->x = parent->x + child->left + parent->pad_left + parent->border_left;
                }
                else if (child->right > 0.0f) {
                    float inner_width = parent->width - parent->pad_left - parent->pad_right - parent->border_left - parent->border_right;
                    child->x = parent->x + inner_width - child->width - child->right;
                }
            }

            // Position relative
            else 
            {
                float x_align_offset = remaining_width * 0.5f * (float)parent->horizontal_alignment;
                child->x = parent->x + parent->pad_left + parent->border_left + cursor_x + x_align_offset;
                cursor_x += child->width + parent->gap;
            }
        }
    }
}

static void NU_Vertically_Place_Children(struct Node* parent, NU_Layer* child_layer)
{
    float y_scroll_offset = 0.0f;
    if (parent->layout_flags & OVERFLOW_VERTICAL_SCROLL && parent->child_count > 0 && 
        parent->content_height > parent->height - parent->pad_top - parent->pad_bottom - parent->border_top - parent->border_bottom) 
    {
        float track_h = parent->height - parent->border_top - parent->border_bottom;
        float inner_height_w_pad = track_h - parent->pad_top - parent->pad_bottom;
        float inner_proportion_of_content_height = inner_height_w_pad / parent->content_height;
        float thumb_h = inner_proportion_of_content_height * track_h;
        float content_scroll_range = parent->content_height - inner_height_w_pad;
        float thumb_scroll_range = track_h - thumb_h;
        float scroll_factor = content_scroll_range / max(thumb_scroll_range, 1.0f);
        y_scroll_offset += (-parent->scroll_v * (track_h - thumb_h)) * scroll_factor;

        struct Node* first_child = NU_Layer_Get(child_layer, parent->first_child_index);
        if (first_child->node_present != 2 && first_child->tag == THEAD) {
            first_child->y -= y_scroll_offset;
        }
    }

    // Children placed left to right
    if (!(parent->layout_flags & LAYOUT_VERTICAL))
    {   
        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->node_present == 2) continue;

            // Position absolute
            if (child->layout_flags & POSITION_ABSOLUTE) 
            {
                child->y = parent->y + parent->pad_top + parent->border_top;
                if (child->top > 0.0f) {
                    child->y = parent->y + child->top + parent->pad_top + parent->border_top;
                }
                else if (child->bottom > 0.0f) {
                    float inner_height = parent->height - parent->pad_top - parent->pad_bottom - parent->border_top - parent->border_bottom;
                    child->y = parent->y + inner_height - child->height - child->bottom;
                }
            }

            // Position relative
            else 
            {
                float remaining_height = (parent->height - parent->pad_top - parent->pad_bottom - parent->border_top - parent->border_bottom) - child->height;
                float y_align_offset = remaining_height * 0.5f * (float)parent->vertical_alignment;
                child->y += parent->y + parent->pad_top + parent->border_top + y_align_offset + y_scroll_offset;
            }
        }
    }

    // Children placed top to bottom
    else
    {
        // Calculate remaining height (optimise this by caching this value inside parent's content height variable)
        float remaining_height = (parent->height - parent->pad_top - parent->pad_bottom - parent->border_top - parent->border_bottom) - (parent->child_count - 1) * parent->gap - (!!(parent->layout_flags & OVERFLOW_HORIZONTAL_SCROLL)) * 8.0f;
        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->node_present == 2) continue;
            if (child->tag == WINDOW) remaining_height += parent->gap;
            else remaining_height -= child->height;
        }

        float cursor_y = 0.0f;

        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->node_present == 2) continue;

            // Position relative
            if (child->layout_flags & POSITION_ABSOLUTE) 
            {
                child->y = parent->y + parent->pad_top + parent->border_top;
                if (child->top > 0.0f) {
                    child->y = parent->y + child->top + parent->pad_top + parent->border_top;
                }
                else if (child->bottom > 0.0f) {
                    float inner_height = parent->height - parent->pad_top - parent->pad_bottom - parent->border_top - parent->border_bottom;
                    child->y = parent->y + inner_height - child->height - child->bottom;
                }
            }

            // Position relative
            else 
            {
                float y_align_offset = remaining_height * 0.5f * (float)parent->vertical_alignment;
                child->y += parent->y + parent->pad_top + parent->border_top + cursor_y + y_align_offset + y_scroll_offset;
                cursor_y += child->height + parent->gap;
            }
        }
    }
}

static void NU_Calculate_Positions()
{
    for (uint16_t l=0; l<=__nu_global_gui.deepest_layer; l++) 
    {
        NU_Layer* parent_layer = &__nu_global_gui.tree.layers[l];
        NU_Layer* child_layer = &__nu_global_gui.tree.layers[l+1];
        
        // Iterate over parent layer
        for (uint32_t p=0; p<parent_layer->size; p++) // For node in layer
        {   
            struct Node* parent = NU_Layer_Get(parent_layer, p);
            if (parent->node_present == 0 || parent->node_present == 2) continue;

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

// Checks against rounded corners
static bool NU_Mouse_Over_Node(struct Node* node, float mouse_x, float mouse_y)
{
    // --- Get clipping info
    float left_wall = node->x;
    float right_wall = node->x + node->width;
    float top_wall = node->y;
    float bottom_wall = node->y + node->height; 
    if (node->clipping_root_handle != UINT32_MAX)
    {
        NU_Clip_Bounds* clip = Hashmap_Get(&__nu_global_gui.node_clip_map, &node->clipping_root_handle);
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
    float border_radius_bl = node->border_radius_bl;
    float border_radius_br = node->border_radius_br;
    float border_radius_tl = node->border_radius_tl;
    float border_radius_tr = node->border_radius_tr;
    float left_radii_sum   = border_radius_tl + border_radius_bl;
    float right_radii_sum  = border_radius_tr + border_radius_br;
    float top_radii_sum    = border_radius_tl + border_radius_tr;
    float bottom_radii_sum = border_radius_bl + border_radius_br;
    if (left_radii_sum   > node->height)  { float scale = node->height / left_radii_sum;   border_radius_tl *= scale; border_radius_bl *= scale; }
    if (right_radii_sum  > node->height)  { float scale = node->height / right_radii_sum;  border_radius_tr *= scale; border_radius_br *= scale; }
    if (top_radii_sum    > node->width )  { float scale = node->width  / top_radii_sum;    border_radius_tl *= scale; border_radius_tr *= scale; }
    if (bottom_radii_sum > node->width )  { float scale = node->width  / bottom_radii_sum; border_radius_bl *= scale; border_radius_br *= scale; }

    // --- Rounded border anchors ---
    vec2 tl_a = { floorf(node->x + border_radius_tl),               floorf(node->y + border_radius_tl) };
    vec2 tr_a = { floorf(node->x + node->width - border_radius_tr), floorf(node->y + border_radius_tr) };
    vec2 bl_a = { floorf(node->x + border_radius_bl),               floorf(node->y + node->height - border_radius_bl) };
    vec2 br_a = { floorf(node->x + node->width - border_radius_br), floorf(node->y + node->height - border_radius_br) };


    // --- Ensure mouse is not in top left rounded deadzone
    if (mouse_x < tl_a.x && mouse_y < tl_a.y)
    {
        float dist = sqrtf((mouse_x - tl_a.x) * (mouse_x - tl_a.x) + (mouse_y - tl_a.y) * (mouse_y - tl_a.y)); 
        if (dist > border_radius_tl) return false;
    }

    // --- Ensure mouse is not in top right rounded deadzone
    if (mouse_x > tr_a.x && mouse_y < tr_a.y)
    {
        float dist = sqrtf((mouse_x - tr_a.x) * (mouse_x - tr_a.x) + (mouse_y - tr_a.y) * (mouse_y - tr_a.y)); 
        if (dist > border_radius_tr) return false;
    }

    // --- Ensure mouse is not in bottom left rounded deadzone
    if (mouse_x < bl_a.x && mouse_y > bl_a.y)
    {
        float dist = sqrtf((mouse_x - bl_a.x) * (mouse_x - bl_a.x) + (mouse_y - bl_a.y) * (mouse_y - bl_a.y)); 
        if (dist > border_radius_bl) return false;
    }

    // --- Ensure mouse is not in bottom right rounded deadzone
    if (mouse_x > br_a.x && mouse_y > br_a.y)
    {
        float dist = sqrtf((mouse_x - br_a.x) * (mouse_x - br_a.x) + (mouse_y - br_a.y) * (mouse_y - br_a.y)); 
        if (dist > border_radius_br) return false;
    }

    return true;
}

// Check against scrollbar v thumb
static bool NU_Mouse_Over_Node_V_Scrollbar(struct Node* node, float mouse_x, float mouse_y) {
    float track_height = node->height - node->border_top - node->border_bottom;
    float thumb_height = (track_height / node->content_height) * track_height;
    float scroll_thumb_left_wall = node->x + node->width - node->border_right - 8.0f;
    float scroll_thumb_top_wall = node->y + node->border_top + (node->scroll_v * (track_height - thumb_height));
    bool within_x_bound = mouse_x >= scroll_thumb_left_wall && mouse_x <= scroll_thumb_left_wall + 8.0f;
    bool within_y_bound = mouse_y >= scroll_thumb_top_wall && mouse_y <= scroll_thumb_top_wall + thumb_height;
    return within_x_bound && within_y_bound;
}


void NU_Mouse_Hover()
{   
    // -----------------------------------------------------------
    // --- Remove potential pseudo style from current hovered node
    // -----------------------------------------------------------
    if (__nu_global_gui.hovered_node != UINT32_MAX && __nu_global_gui.hovered_node != __nu_global_gui.mouse_down_node) {
        NU_Apply_Stylesheet_To_Node(NODE(__nu_global_gui.hovered_node), __nu_global_gui.stylesheet);
    }
    uint32_t prev_hovered_node = __nu_global_gui.hovered_node;
    __nu_global_gui.hovered_node = UINT32_MAX;
    __nu_global_gui.scroll_hovered_node = UINT32_MAX;

    if (__nu_global_gui.hovered_window == NULL) return;


    // -----------------------------
    // --- Get local mouse position
    // -----------------------------
    float global_mouse_x, global_mouse_y;
    int window_x, window_y;
    SDL_GetGlobalMouseState(&global_mouse_x, &global_mouse_y);
    SDL_GetWindowPosition(__nu_global_gui.hovered_window, &window_x, &window_y);
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
    for (uint32_t i=0; i<__nu_global_gui.absolute_root_nodes.size; i++)
    {
        struct Node* absolute_node = *(struct Node**)Vector_Get(&__nu_global_gui.absolute_root_nodes, i);
        if (absolute_node->window == __nu_global_gui.hovered_window && NU_Mouse_Over_Node(absolute_node, mouse_x, mouse_y)) {
            Vector_Push(&stack, &absolute_node);
        }
    }

    for (uint32_t i=0; i<__nu_global_gui.window_nodes.size; i++)
    {
        struct Node* node = *(struct Node**)Vector_Get(&__nu_global_gui.window_nodes, i);
        if (node->window == __nu_global_gui.hovered_window){
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
        __nu_global_gui.hovered_node = current_node->handle;
        stack.size -= 1;

        if (current_node->tag == BUTTON) continue; // Skip children

        // ------------------------------
        // --- Iterate over children
        // ------------------------------
        NU_Layer* child_layer = &__nu_global_gui.tree.layers[current_node->layer+1];
        for (uint16_t i=current_node->first_child_index; i<current_node->first_child_index + current_node->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->node_present == 2 || 
                child->layout_flags & POSITION_ABSOLUTE || 
                child->tag == WINDOW ||
                !NU_Mouse_Over_Node(child, mouse_x, mouse_y)) continue; // Skip

            // Check for scroll hover
            if (child->layout_flags & OVERFLOW_V_PROPERTY) {
                bool overflow_v = child->content_height > child->height - child->border_top - child->border_bottom;
                if (overflow_v) {
                    __nu_global_gui.scroll_hovered_node = child->handle;
                }
            }
            Vector_Push(&stack, &child);
        }
    }



    // ----------------------------------------
    // Apply hover pseudo style to hovered node
    // ----------------------------------------
    if (__nu_global_gui.hovered_node != UINT32_MAX && __nu_global_gui.hovered_node != __nu_global_gui.mouse_down_node) {
        NU_Apply_Pseudo_Style_To_Node(NODE(__nu_global_gui.hovered_node), __nu_global_gui.stylesheet, PSEUDO_HOVER);
    } 
    Vector_Free(&stack);

    // If hovered node change -> must redraw later
    if (prev_hovered_node != __nu_global_gui.hovered_node) {
        __nu_global_gui.awaiting_redraw = true;
    }
}



// -----------------------------
// --- Rendering ---------------
// -----------------------------
void NU_Add_Text_Mesh(Vertex_RGB_UV_List* vertices, Index_List* indices, struct Node* node)
{
    // Compute inner dimensions (content area)
    float inner_width  = node->width  - node->border_left - node->border_right - node->pad_left - node->pad_right;
    float inner_height = node->height - node->border_top  - node->border_bottom - node->pad_top - node->pad_bottom;
    float remaining_w = inner_width  - node->content_width;
    float remaining_h = inner_height - node->content_height;
    float x_align_offset = remaining_w * 0.5f * (float)node->horizontal_text_alignment;
    float y_align_offset = remaining_h * 0.5f * (float)node->vertical_text_alignment;

    // Top-left corner of the content area
    float textPosX = node->x + node->border_left + node->pad_left + x_align_offset;
    float textPosY = node->y + node->border_top  + node->pad_top + y_align_offset;

    // Draw wrapped text inside inner_width
    float r = (float)node->text_r / 255.0f;
    float g = (float)node->text_g / 255.0f;
    float b = (float)node->text_b / 255.0f;
    NU_Font* node_font = Vector_Get(&__nu_global_gui.stylesheet->fonts, node->font_id);
    NU_Generate_Text_Mesh(vertices, indices, node_font, node->text_content, floorf(textPosX), floorf(textPosY), r, g, b, inner_width);
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
    for (int i=0; i<__nu_global_gui.windows.size; i++) {
        SDL_Window* window = *(SDL_Window**) Vector_Get(&__nu_global_gui.windows, i);
        if (window == node->window) {
            return i;
        }
    }
    return 0;
}

void NU_Populate_Draw_Lists()
{
    // Initialise draw lists
    uint32_t window_count = __nu_global_gui.windows.size;
    for (uint32_t i=0; i<window_count; i++) 
    {
        NU_Window_Draw_Lists* lists = Vector_Get(&__nu_global_gui.windows_draw_lists, i);
        Vector_Reserve(&lists->relative_node_list, sizeof(struct Node*), 512);
        Vector_Reserve(&lists->clipped_relative_node_list, sizeof(struct Node*), 64);
        Vector_Reserve(&lists->absolute_node_list, sizeof(struct Node*), 64);
        Vector_Reserve(&lists->clipped_absolute_node_list, sizeof(struct Node*), 16);
        Vector_Reserve(&lists->canvas_node_list, sizeof(struct Node*), 4);
        Vector_Reserve(&lists->clipped_canvas_node_list, sizeof(struct Node*), 1);
    }

    // Clear clip map
    Hashmap* node_clip_map = &__nu_global_gui.node_clip_map;
    Hashmap_Clear(&__nu_global_gui.node_clip_map);

    __nu_global_gui.absolute_root_nodes.size = 0;

    

    NU_Window_Draw_Lists* root_lists = Vector_Get(&__nu_global_gui.windows_draw_lists, 0);


    struct Node* root = &__nu_global_gui.tree.layers[0].node_array[0];
    Vector_Push(&root_lists->relative_node_list, &root);
    for (int l=0; l<=__nu_global_gui.deepest_layer; l++)
    {
        NU_Layer* parent_layer = &__nu_global_gui.tree.layers[l];
        NU_Layer* child_layer = &__nu_global_gui.tree.layers[l+1];

        // Iterate over parent layer
        for (int p=0; p<parent_layer->size; p++)
        {       
            struct Node* parent = NU_Layer_Get(parent_layer, p);
            if (parent->node_present == 0) continue;
            if (parent->node_present == 2) { // Parent not visible -> children must inherit this
                for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++) {
                    struct Node* child = NU_Layer_Get(child_layer, i);
                    child->node_present = 2;
                }
                continue;
            }

            // Precompute once (optimisation)
            float parent_inner_x, parent_inner_y, parent_inner_width, parent_inner_height = 0;
            if (parent->layout_flags & OVERFLOW_VERTICAL_SCROLL) {
                parent_inner_y = parent->y + parent->border_top + parent->pad_top;
                parent_inner_height = parent->height - parent->border_top - parent->border_bottom - parent->pad_top - parent->pad_bottom;

                // Handle case where parent is a table with a thead
                if (parent->tag == TABLE) {
                    struct Node* first_child = NU_Layer_Get(child_layer, parent->first_child_index);
                    if (first_child->tag == THEAD) {
                        parent_inner_y += first_child->height;
                        parent_inner_height -= first_child->height;
                    }
                }
            }
            if (parent->layout_flags & OVERFLOW_HORIZONTAL_SCROLL) {
                parent_inner_x = parent->x + parent->border_left + parent->pad_left;
                parent_inner_width = parent->width - parent->border_left - parent->border_right - parent->pad_left - parent->pad_right;
            }

            // Iterate over children
            for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
            {
                struct Node* child = NU_Layer_Get(child_layer, i); if (child->node_present == 2) continue;

                // If child not visible in window bounds -> mark as hidden
                int window_index, w, h;
                window_index = NU_Draw_Find_Window(child);
                SDL_Window* window = *(SDL_Window**) Vector_Get(&__nu_global_gui.windows, window_index);
                SDL_GetWindowSize(window, &w, &h);
                if (!Is_Node_Visible_In_Window(child, (float)w, (float)h)) {
                    child->node_present = 2; 
                    continue;
                }

                // Add child to list of root absolute nodes
                if (child->layout_flags & POSITION_ABSOLUTE) {
                    Vector_Push(&__nu_global_gui.absolute_root_nodes, &child);
                }

                // Get correct draw lists
                NU_Window_Draw_Lists* lists = Vector_Get(&__nu_global_gui.windows_draw_lists, window_index);



                // Skip node if not visible in parent (due to overflow)
                if (parent->layout_flags & OVERFLOW_VERTICAL_SCROLL && child->tag != THEAD) 
                {   
                    int intersect_state = Node_Vertical_Overlap_State(child, parent_inner_y, parent_inner_height);

                    // Child not inside parent -> hide in this draw pass
                    if (intersect_state == 0) {
                        child->node_present = 2; 
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
                        if (child->position_absolute)
                        {
                            // If parent is also clipped -> merge clips (stack clipping behaviour)
                            if (parent->clipping_root_handle != UINT32_MAX) {
                                NU_Clip_Bounds* parent_clip = Hashmap_Get(node_clip_map, &parent->clipping_root_handle);
                                clip.clip_top = fmaxf(clip.clip_top, parent_clip->clip_top);
                                clip.clip_bottom = fminf(clip.clip_bottom, parent_clip->clip_bottom);
                            }
                            
                            // Add clipping to hashmap
                            Hashmap_Set(node_clip_map, &child->handle, &clip);
                            child->clipping_root_handle = child->handle; // Set clip root to self

                            // Append node to correct window clipped node list
                            Vector_Push(&lists->clipped_absolute_node_list, &child);
                            continue;
                        }
                        // Position Relative -> main drawing list
                        else
                        {
                            // If parent is also clipped -> merge clips (stack clipping behaviour)
                            if (parent->clipping_root_handle != UINT32_MAX) {
                                NU_Clip_Bounds* parent_clip = Hashmap_Get(node_clip_map, &parent->clipping_root_handle);
                                clip.clip_top = fmaxf(clip.clip_top, parent_clip->clip_top);
                                clip.clip_bottom = fminf(clip.clip_bottom, parent_clip->clip_bottom);
                            }
                            
                            // Add clipping to hashmap
                            Hashmap_Set(node_clip_map, &child->handle, &clip);
                            child->clipping_root_handle = child->handle; // Set clip root to self

                            // Append node to correct window clipped node list
                            Vector_Push(&lists->clipped_relative_node_list, &child);
                            continue;
                        }
                    }
                }

                // If parent is clipped -> child inherits clip from parent
                if (parent->clipping_root_handle != UINT32_MAX) {
                    child->clipping_root_handle = parent->clipping_root_handle;

                    // Position Absolute -> secondary drawing list
                    if (child->position_absolute)
                    {
                        Vector_Push(&lists->clipped_absolute_node_list, &child);
                    }
                    // Position Relative -> main drawing list
                    else
                    {
                        Vector_Push(&lists->clipped_relative_node_list, &child);
                    }
                    continue;
                }



                // Neither child nor parent is clipped -> append node to correct window node list
                if (child->position_absolute) // Position Absolute -> secondary drawing list
                {
                    Vector_Push(&lists->absolute_node_list, &child);
                }
                else // Position Relative -> main drawing list
                {
                    Vector_Push(&lists->relative_node_list, &child);
                }

                // Append node to canvas API drawing list
                if (child->tag == CANVAS) {
                    Vector_Push(&lists->canvas_node_list, &child);
                }
            }
        }
    }
}

void NU_Draw()
{
    uint32_t window_count = __nu_global_gui.windows.size;

    // Initialise text vertex and index buffers (per font)
    Vertex_RGB_UV_List text_relative_vertex_buffers[__nu_global_gui.stylesheet->fonts.size];
    Vertex_RGB_UV_List text_absolute_vertex_buffers[__nu_global_gui.stylesheet->fonts.size];
    Index_List text_relative_index_buffers[__nu_global_gui.stylesheet->fonts.size];
    Index_List text_absolute_index_buffers[__nu_global_gui.stylesheet->fonts.size];
    for (uint32_t i=0; i<__nu_global_gui.stylesheet->fonts.size; i++)
    {
        Vertex_RGB_UV_List_Init(&text_relative_vertex_buffers[i], 512);
        Index_List_Init(&text_relative_index_buffers[i], 512);
        Vertex_RGB_UV_List_Init(&text_absolute_vertex_buffers[i], 128);
        Index_List_Init(&text_absolute_index_buffers[i], 128);
    }

    // For each window
    for (uint32_t i=0; i<window_count; i++)
    {
        NU_Window_Draw_Lists* lists = Vector_Get(&__nu_global_gui.windows_draw_lists, i);
        SDL_Window* window = *(SDL_Window**) Vector_Get(&__nu_global_gui.windows, i);
        SDL_GL_MakeCurrent(window, __nu_global_gui.gl_ctx);

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
        for (int n=0; n<lists->relative_node_list.size; n++) { // Construct border rect vertices and indices for each node
            struct Node* node = *(struct Node**) Vector_Get(&lists->relative_node_list, n);
            Construct_Border_Rect(node, w_fl, h_fl, &border_rect_vertices, &border_rect_indices);
            if (node->layout_flags & OVERFLOW_VERTICAL_SCROLL 
                && node->content_height > (node->height - node->pad_top - node->pad_bottom - node->border_top - node->border_bottom)) {
                Construct_Scroll_Thumb(node, w_fl, h_fl, &border_rect_vertices, &border_rect_indices);
            }
        }
        Draw_Vertex_RGB_List(&border_rect_vertices, &border_rect_indices, w_fl, h_fl, 0.0f, 0.0f); // Draw border rects in one call
        Vertex_RGB_List_Free(&border_rect_vertices);
        Index_List_Free(&border_rect_indices);


        // --- Draw Canvas API Content ---
        for (uint32_t n=0; n<lists->canvas_node_list.size; n++)
        {
            struct Node* canvas_node = *(struct Node**) Vector_Get(&lists->canvas_node_list, n);
            float offset_x = canvas_node->x + canvas_node->border_left + canvas_node->pad_left;
            float offset_y = canvas_node->y + canvas_node->border_top + canvas_node->pad_top;
            float clip_top    = canvas_node->y + canvas_node->border_top + canvas_node->pad_top;
            float clip_bottom = canvas_node->y + canvas_node->height - canvas_node->border_bottom - canvas_node->pad_bottom;
            float clip_left   = canvas_node->x + canvas_node->border_left + canvas_node->pad_left;
            float clip_right  = canvas_node->x + canvas_node->width - canvas_node->border_right - canvas_node->pad_right;
            NU_Canvas_Context* ctx = Hashmap_Get(&__nu_global_gui.canvas_contexts, &canvas_node->handle);
            Draw_Clipped_Vertex_RGB_List(
                &ctx->vertices, 
                &ctx->indices,
                w_fl, h_fl, 
                offset_x, offset_y,
                clip_top, clip_bottom, clip_left, clip_right 
            );
        }


        // --- Construct Text Meshes & Draw Images ---
        for (int n=0; n<lists->relative_node_list.size; n++) 
        {

            // Construct text mesh for node
            struct Node* node = *(struct Node**) Vector_Get(&lists->relative_node_list, n);
            if (node->text_content != NULL) {
                Vertex_RGB_UV_List* text_vertices = &text_relative_vertex_buffers[node->font_id];
                Index_List* text_indices = &text_relative_index_buffers[node->font_id];
                NU_Add_Text_Mesh(text_vertices, text_indices, node);
            }

            if (node->gl_image_handle) // Draw image
            {
                float inner_width  = node->width - node->border_left - node->border_right - node->pad_left - node->pad_right;
                float inner_height = node->height - node->border_top - node->border_bottom - node->pad_top - node->pad_bottom;
                float x = node->x + node->border_left + (float)node->pad_left;
                float y = node->y + node->border_top + (float)node->pad_top;
                NU_Draw_Image(
                    x, y, 
                    inner_width, inner_height, 
                    w_fl, h_fl, 
                    -1.0f, 100000.0f, -1.0f, 100000.0f,
                    node->gl_image_handle);
            }
        }
        for (uint32_t t=0; t<__nu_global_gui.stylesheet->fonts.size; t++) // Draw text
        {
            Vertex_RGB_UV_List* text_vertices = &text_relative_vertex_buffers[t];
            Index_List* text_indices = &text_relative_index_buffers[t];
            NU_Font* node_font = Vector_Get(&__nu_global_gui.stylesheet->fonts, t);
            NU_Render_Text(text_vertices, text_indices, node_font, w_fl, h_fl, -1.0f, 100000.0f, -1.0f, 100000.0f);
            text_vertices->size = 0;
            text_indices->size = 0;
        }


        // --- Draw Partially Visible Nodes ---
        for (int n=0; n<lists->clipped_relative_node_list.size; n++) {
            struct Node* node = *(struct Node**) Vector_Get(&lists->clipped_relative_node_list, n);
            NU_Clip_Bounds* clip = (NU_Clip_Bounds*)Hashmap_Get(&__nu_global_gui.node_clip_map, &node->clipping_root_handle);

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
            if (node->text_content != NULL) 
            {
                Vertex_RGB_UV_List clipped_text_vertices;
                Index_List clipped_text_indices;
                Vertex_RGB_UV_List_Init(&clipped_text_vertices, 1000);
                Index_List_Init(&clipped_text_indices, 600);
                NU_Font* node_font = Vector_Get(&__nu_global_gui.stylesheet->fonts, node->font_id);
                NU_Add_Text_Mesh(&clipped_text_vertices, &clipped_text_indices, node);
                NU_Render_Text(&clipped_text_vertices, &clipped_text_indices, node_font, w_fl, h_fl, clip->clip_top, clip->clip_bottom, clip->clip_left, clip->clip_right);
                Vertex_RGB_UV_List_Free(&clipped_text_vertices);
                Index_List_Free(&clipped_text_indices);
            }

            // Draw image
            if (node->gl_image_handle) 
            {
                float inner_width  = node->width - node->border_left - node->border_right - node->pad_left - node->pad_right;
                float inner_height = node->height - node->border_top - node->border_bottom - node->pad_top - node->pad_bottom;
                NU_Draw_Image(
                    node->x + node->border_left + node->pad_left, 
                    node->y + node->border_top + node->pad_top, 
                    inner_width, inner_height, 
                    w_fl, h_fl, 
                    clip->clip_top, clip->clip_bottom, clip->clip_left, clip->clip_right,
                    node->gl_image_handle);
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
        for (int n=0; n<lists->absolute_node_list.size; n++) { // Construct border rect vertices and indices for each node
            struct Node* node = *(struct Node**) Vector_Get(&lists->absolute_node_list, n);
            Construct_Border_Rect(node, w_fl, h_fl, &border_rect_vertices, &border_rect_indices);
            if (node->layout_flags & OVERFLOW_VERTICAL_SCROLL 
                && node->content_height > (node->height - node->pad_top - node->pad_bottom - node->border_top - node->border_bottom)) {
                Construct_Scroll_Thumb(node, w_fl, h_fl, &border_rect_vertices, &border_rect_indices);
            }
        }
        Draw_Vertex_RGB_List(&border_rect_vertices, &border_rect_indices, w_fl, h_fl, 0.0f, 0.0f); // Draw border rects in one call
        Vertex_RGB_List_Free(&border_rect_vertices);
        Index_List_Free(&border_rect_indices);


        // --- Construct Text Meshes & Draw Images ---
        for (int n=0; n<lists->absolute_node_list.size; n++) 
        {

            // Construct text mesh for node
            struct Node* node = *(struct Node**) Vector_Get(&lists->absolute_node_list, n);
            if (node->text_content != NULL) {
                Vertex_RGB_UV_List* text_vertices = &text_absolute_vertex_buffers[node->font_id];
                Index_List* text_indices = &text_absolute_index_buffers[node->font_id];
                NU_Add_Text_Mesh(text_vertices, text_indices, node);
            }

            if (node->gl_image_handle) // Draw image
            {
                float inner_width  = node->width - node->border_left - node->border_right - node->pad_left - node->pad_right;
                float inner_height = node->height - node->border_top - node->border_bottom - node->pad_top - node->pad_bottom;
                float x = node->x + node->border_left + (float)node->pad_left;
                float y = node->y + node->border_top + (float)node->pad_top;
                NU_Draw_Image(
                    x, y, 
                    inner_width, inner_height, 
                    w_fl, h_fl, 
                    -1.0f, 100000.0f, -1.0f, 100000.0f,
                    node->gl_image_handle);
            }
        }
        for (uint32_t t=0; t<__nu_global_gui.stylesheet->fonts.size; t++) // Draw text
        {
            Vertex_RGB_UV_List* text_vertices = &text_absolute_vertex_buffers[t];
            Index_List* text_indices = &text_absolute_index_buffers[t];
            NU_Font* node_font = Vector_Get(&__nu_global_gui.stylesheet->fonts, t);
            NU_Render_Text(text_vertices, text_indices, node_font, w_fl, h_fl, -1.0f, 100000.0f, -1.0f, 100000.0f);
            text_vertices->size = 0;
            text_indices->size = 0;
        }

        // --- Draw Partially Visible Nodes ---
        for (int n=0; n<lists->clipped_absolute_node_list.size; n++) {
            struct Node* node = *(struct Node**) Vector_Get(&lists->clipped_absolute_node_list, n);
            NU_Clip_Bounds* clip = (NU_Clip_Bounds*)Hashmap_Get(&__nu_global_gui.node_clip_map, &node->clipping_root_handle);

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
            if (node->text_content != NULL) 
            {
                Vertex_RGB_UV_List clipped_text_vertices;
                Index_List clipped_text_indices;
                Vertex_RGB_UV_List_Init(&clipped_text_vertices, 1000);
                Index_List_Init(&clipped_text_indices, 600);
                NU_Font* node_font = Vector_Get(&__nu_global_gui.stylesheet->fonts, node->font_id);
                NU_Add_Text_Mesh(&clipped_text_vertices, &clipped_text_indices, node);
                NU_Render_Text(&clipped_text_vertices, &clipped_text_indices, node_font, w_fl, h_fl, clip->clip_top, clip->clip_bottom, clip->clip_left, clip->clip_right);
                Vertex_RGB_UV_List_Free(&clipped_text_vertices);
                Index_List_Free(&clipped_text_indices);
            }

            // Draw image
            if (node->gl_image_handle) 
            {
                float inner_width  = node->width - node->border_left - node->border_right - node->pad_left - node->pad_right;
                float inner_height = node->height - node->border_top - node->border_bottom - node->pad_top - node->pad_bottom;
                NU_Draw_Image(
                    node->x + node->border_left + node->pad_left, 
                    node->y + node->border_top + node->pad_top, 
                    inner_width, inner_height, 
                    w_fl, h_fl, 
                    clip->clip_top, clip->clip_bottom, clip->clip_left, clip->clip_right,
                    node->gl_image_handle);
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
        NU_Window_Draw_Lists* lists = Vector_Get(&__nu_global_gui.windows_draw_lists, i);
        Vector_Free(&lists->relative_node_list);
        Vector_Free(&lists->clipped_relative_node_list);
        Vector_Free(&lists->absolute_node_list);
        Vector_Free(&lists->clipped_absolute_node_list);
        Vector_Free(&lists->canvas_node_list);
        Vector_Free(&lists->clipped_canvas_node_list);
    }
    for (uint32_t i=0; i<__nu_global_gui.stylesheet->fonts.size; i++) {
        Vertex_RGB_UV_List_Free(&text_relative_vertex_buffers[i]);
        Index_List_Free(&text_relative_index_buffers[i]);
        Vertex_RGB_UV_List_Free(&text_absolute_vertex_buffers[i]);
        Index_List_Free(&text_absolute_index_buffers[i]);
    }

    __nu_global_gui.awaiting_redraw = false;
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