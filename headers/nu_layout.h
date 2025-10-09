#define NANOVG_GL3_IMPLEMENTATION
#include <omp.h>
#include <nanovg.h>
#include <nanovg_gl.h>
#include <freetype/freetype.h>
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <stdbool.h>
#include <math.h>
#include <nu_text.h>
#include "nu_draw.h"
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
}

static void NU_Apply_Min_Max_Height_Constraint(struct Node* node)
{
    node->height = min(max(node->height, node->min_height), node->max_height);
}

static void NU_Clear_Node_Sizes()
{
    // Iterate Over Every Node
    for (uint16_t l=0; l<=__nu_global_gui.deepest_layer; l++)
    {
        NU_Layer* layer = &__nu_global_gui.tree.layers[l];

        for (int i=0; i<layer->size; i++)
        {
            struct Node* node = &layer->node_array[i];
            if (!node->node_present) continue;


            node->node_present = 1;

            // Reset Node Size
            node->x = 0.0f;
            node->y = 0.0f;

            // Constrain preferred/min/max width/height -> These are limited by the border and padding
            // Preferred width height bust also be constrained by min/max width/height
            float natural_width = node->border_left + node->border_right + node->pad_left + node->pad_right;
            float natural_height = node->border_top + node->border_bottom + node->pad_top + node->pad_bottom;
            node->min_width = max(node->min_width, natural_width);
            node->min_height = max(node->min_height, natural_height);
            node->max_width = max(max(node->max_width, natural_width), node->min_width);
            node->max_height = max(max(node->max_height, natural_height), node->min_height);
            node->preferred_width = max(node->preferred_width, natural_width);
            node->preferred_height = max(node->preferred_height, natural_height);
            node->preferred_width = min(max(node->preferred_width, node->min_width), node->max_width);
            node->preferred_height = min(max(node->preferred_height, node->min_height), node->max_height);

            // Set base width/height and reset content dimensions
            node->width = node->preferred_width;
            node->height = node->preferred_height;
            node->content_width = 0;
            node->content_height = 0;

            // Enforce Window Dimensions (This is very slow and must not be done each time I call Reflow!!!)
            // if (node->tag == WINDOW) {
            //     SDL_SetWindowMinimumSize(node->window, node->min_width, node->min_height);
            // }
        }
    }
}

static float NU_Calculate_Text_Min_Wrap_Width(struct Node* node, float full_width)
{
    const char* text = node->text_content;
    if (!text || !*text) return full_width; // empty string guard
    int slice_start = 0;
    float max_word_width = 0.0f;
    int len = strlen(text);
    for (int i = 0; i <= len; i++) {
        char c = (i < len) ? text[i] : ' '; // add virtual space at end
        if (c == ' ') {
            if (i > slice_start) {
                float bounds[4];
                nvgTextBounds(__nu_global_gui.vg_ctx, 0, 0, text + slice_start, text + i, bounds);
                float width = bounds[2] - bounds[0];
                if (width > max_word_width) max_word_width = width;
            }
            slice_start = i + 1;
        }
    }
    if (max_word_width == 0.0f && len > 0) { // no spaces, measure whole string
        max_word_width = full_width;
    }
    return max_word_width;
}

static void NU_Calculate_Text_Fit_Widths()
{
    // This needs to be recomputed when a node uses a different font!
    float asc, desc, lh;
    int fontID = *(int*) Vector_Get(&__nu_global_gui.font_registry, 0);
    nvgFontFaceId(__nu_global_gui.vg_ctx, fontID);   
    nvgFontSize(__nu_global_gui.vg_ctx, 14);
    nvgTextMetrics(__nu_global_gui.vg_ctx, &asc, &desc, &lh);

    // For each layer
    for (uint16_t l=0; l<=__nu_global_gui.deepest_layer; l++)
    {
        NU_Layer* layer = &__nu_global_gui.tree.layers[l];

        #pragma omp parallel for
        for (uint32_t n=0; n<layer->size; n++)
        {   
            struct Node* node = &layer->node_array[n];
            if (!node->node_present) continue;

            if (node->text_content != NULL) 
            {
                // Calculate text bounds
                float bounds[4];
                nvgTextBounds(__nu_global_gui.vg_ctx, 0, 0, node->text_content, NULL, bounds);
                float text_width = bounds[2] - bounds[0];
                float text_height = lh;
                
                // Calculate minimum text wrap width (longest unbreakable word)
                float min_wrap_width = NU_Calculate_Text_Min_Wrap_Width(node, text_width);

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
    for (int l=__nu_global_gui.deepest_layer-1; l>=0; l--)
    {
        NU_Layer* parent_layer = &__nu_global_gui.tree.layers[l];
        NU_Layer* child_layer = &__nu_global_gui.tree.layers[l+1];
        
        // Iterate over parent layer
        for (uint32_t p=0; p<parent_layer->size; p++)
        {       
            struct Node* parent = NU_Layer_Get(parent_layer, p);
            if (!parent->node_present) continue;

            int is_layout_horizontal = !(parent->layout_flags & LAYOUT_VERTICAL);

            if (parent->tag == WINDOW) {
                int window_width, window_height;
                SDL_GetWindowSize(parent->window, &window_width, &window_height);
                parent->width = (float) window_width;
                parent->height = (float) window_height;
            }

            if (parent->child_count == 0) continue; // Skip acummulating child sizes (no children)

            // Accumulate content width
            for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
            {
                struct Node* child = NU_Layer_Get(child_layer, i);

                if (child->tag == WINDOW && is_layout_horizontal) { 
                    parent->content_width -= parent->gap + child->width; 
                }   
                
                // Horizontal Layout
                if (is_layout_horizontal) parent->content_width += child->width;

                // Vertical Layout 
                else parent->content_width = MAX(parent->content_width, child->width);
            }

            // Expand parent width to account for content width
            if (is_layout_horizontal) parent->content_width += (parent->child_count - 1) * parent->gap;
            if (parent->tag != WINDOW && parent->content_width > parent->width) {
                parent->width = parent->content_width + parent->border_left + parent->border_right + parent->pad_left + parent->pad_right;
                NU_Apply_Min_Max_Width_Constraint(parent);
            }
        }
    }
}

static void NU_Calculate_Fit_Size_Heights()
{
    if (__nu_global_gui.deepest_layer == 0) return;

    for (int l=__nu_global_gui.deepest_layer-1; l>= 0; l--)
    {
        NU_Layer* parent_layer = &__nu_global_gui.tree.layers[l];
        NU_Layer* child_layer = &__nu_global_gui.tree.layers[l+1];

        // Iterate over parent layer
        for (uint32_t p=0; p<parent_layer->size; p++)
        {       
            struct Node* parent = NU_Layer_Get(parent_layer, p);
            if (!parent->node_present) continue;

            int is_layout_horizontal = !(parent->layout_flags & LAYOUT_VERTICAL);

            if (parent->child_count == 0) { continue; } // Skip acummulating child sizes (no children)

            // Iterate over children
            for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
            {
                struct Node* child = NU_Layer_Get(child_layer, i);

                if (child->tag == WINDOW) {
                    if (!is_layout_horizontal) parent->content_height -= parent->gap + child->height;
                }   
                
                // Horizontal Layout
                if (is_layout_horizontal) parent->content_height = MAX(parent->content_height, child->height);
                
                // Vertical Layout 
                else parent->content_height += child->height;
            }

            // Expand parent height to account for content height
            if (!is_layout_horizontal) parent->content_height += (parent->child_count - 1) * parent->gap;
            if (parent->tag != WINDOW) {
                parent->height = parent->content_height + parent->border_top + parent->border_bottom + parent->pad_top + parent->pad_bottom;
                NU_Apply_Min_Max_Height_Constraint(parent);
            }
        }
    }
}

static void NU_Grow_Shrink_Child_Node_Widths(struct Node* parent, NU_Layer* child_layer)
{
    float remaining_width = parent->width - parent->pad_left - parent->pad_right - parent->border_left - parent->border_right - (!!(parent->layout_flags & OVERFLOW_VERTICAL_SCROLL)) * 12.0f;

    // ------------------------------------------------
    // If parent lays out children vertically ---------
    // ------------------------------------------------
    if (parent->layout_flags & LAYOUT_VERTICAL)
    {   
        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
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
        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->tag == WINDOW) remaining_width += parent->gap;
            else remaining_width -= child->width;
            if (child->layout_flags & GROW_HORIZONTAL && child->tag != WINDOW) growable_count++;
        }
        remaining_width -= (parent->child_count - 1) * parent->gap;
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
                struct Node* child = NU_Layer_Get(child_layer, i);
                if ((child->layout_flags & GROW_HORIZONTAL) && child->tag != WINDOW && child->width < child->max_width) {
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
                struct Node* child = NU_Layer_Get(child_layer, i);            
                if (child->layout_flags & GROW_HORIZONTAL && child->tag != WINDOW && child->width < child->max_width) {// if child is growable
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
                struct Node* child = NU_Layer_Get(child_layer, i);
                if ((child->layout_flags & GROW_HORIZONTAL) && child->tag != WINDOW && child->width > child->min_width) {
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
                struct Node* child = NU_Layer_Get(child_layer, i);
                if ((child->layout_flags & GROW_HORIZONTAL) && child->tag != WINDOW && child->width > child->min_width) {
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
    float remaining_height = parent->height - parent->pad_top - parent->pad_bottom - parent->border_top - parent->border_bottom - (!!(parent->layout_flags & OVERFLOW_HORIZONTAL_SCROLL)) * 12.0f;
    if (!(parent->layout_flags & LAYOUT_VERTICAL))
    {
        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
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
        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->tag == WINDOW) remaining_height += parent->gap;
            else remaining_height -= child->height;
            if (child->layout_flags & GROW_VERTICAL && child->tag != WINDOW) growable_count++;
        }
        remaining_height -= (parent->child_count - 1) * parent->gap;
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
                struct Node* child = NU_Layer_Get(child_layer, i);
                if ((child->layout_flags & GROW_VERTICAL) && child->tag != WINDOW && child->height < child->max_height) {
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
                struct Node* child = NU_Layer_Get(child_layer, i);
                if (child->layout_flags & GROW_VERTICAL && child->tag != WINDOW && child->height < child->max_height) { // if child is growable
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
                struct Node* child = NU_Layer_Get(child_layer, i);
                if ((child->layout_flags & GROW_VERTICAL) && child->tag != WINDOW && child->height > child->min_height) {
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
                struct Node* child = NU_Layer_Get(child_layer, i);
                if ((child->layout_flags & GROW_VERTICAL) && child->tag != WINDOW && child->height > child->min_height) {
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

        // Iterate over parent layer
        for (int p=0; p<parent_layer->size; p++)
        {       
            struct Node* parent = NU_Layer_Get(parent_layer, p);
            if (!parent->node_present || parent->tag == ROW || parent->tag == TABLE) continue;

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

        // Iterate over parent layer
        for (int p=0; p<parent_layer->size; p++)
        {       
            struct Node* parent = NU_Layer_Get(parent_layer, p);
            if (!parent->node_present || parent->tag == TABLE) continue;

            NU_Grow_Shrink_Child_Node_Heights(parent, child_layer);
        }
    }
}

static void NU_Calculate_Table_Column_Widths()
{
    for (uint16_t l=0; l<=__nu_global_gui.deepest_layer; l++)
    {
        NU_Layer* table_layer = &__nu_global_gui.tree.layers[l];

        // Iterate over  layer
        for (int i=0; i<table_layer->size; i++)
        {       
            struct Node* node = NU_Layer_Get(table_layer, i);
            if (!node->node_present) continue;

            if (node->tag == TABLE && node->child_count > 0) 
            {

                struct Vector widest_in_each_column;
                Vector_Reserve(&widest_in_each_column, sizeof(float), 50);
                NU_Layer* row_layer = &__nu_global_gui.tree.layers[l+1];

                // ------------------------------------------------------------
                // --- Calculate the widest cell width in each table column ---
                // ------------------------------------------------------------
                for (uint16_t r=node->first_child_index; r<node->first_child_index + node->child_count; r++)
                {
                    struct Node* row = NU_Layer_Get(row_layer, r);
                    if (row->child_count == 0) break;

                    // Iterate over cells in row
                    NU_Layer* cell_layer = &__nu_global_gui.tree.layers[l+2];
                    int cell_index = 0;
                    for (uint16_t c=row->first_child_index; c<row->first_child_index + row->child_count; c++)
                    {
                        if (cell_index == widest_in_each_column.size) {
                            float val = 0;
                            Vector_Push(&widest_in_each_column, &val);
                        }
                        float* val = Vector_Get(&widest_in_each_column, cell_index);
                        struct Node* cell = NU_Layer_Get(cell_layer, c);
                        if (cell->width > *val) {
                            *val = cell->width;
                        }
                        cell_index++;
                    }
                }

                // -----------------------------------------------
                // --- Apply widest column widths to all cells ---
                // -----------------------------------------------
                float row_width = node->width - node->border_left - node->border_right - node->pad_left - node->pad_right - (!!(node->layout_flags & OVERFLOW_VERTICAL_SCROLL)) * 12.0f;
                float remaining_width = row_width;
                for (int k=0; k<widest_in_each_column.size; k++) {
                    remaining_width -= *(float*)Vector_Get(&widest_in_each_column, k);
                }
                float used_width = row_width - remaining_width;
                for (uint16_t r=node->first_child_index; r<node->first_child_index + node->child_count; r++)
                {
                    struct Node* row = NU_Layer_Get(row_layer, r);
                    row->width = row_width;
                    
                    if (row->child_count == 0) break;

                    // Iterate over cells in row
                    NU_Layer* cell_layer = &__nu_global_gui.tree.layers[l+2];
                    int cell_index = 0;
                    for (uint16_t c=row->first_child_index; c<row->first_child_index + row->child_count; c++)
                    {
                        struct Node* cell = NU_Layer_Get(cell_layer, c);
                        float column_width = *(float*)Vector_Get(&widest_in_each_column, cell_index);
                        float proportion = column_width / used_width;
                        cell->width = column_width + remaining_width * proportion;
                        cell_index++;
                    }
                }
                Vector_Free(&widest_in_each_column);
            }
        }
    }
}

static void NU_Calculate_Text_Heights()
{
    // Configure font
    int fontID = *(int*) Vector_Get(&__nu_global_gui.font_registry, 0);
    nvgFontFaceId(__nu_global_gui.vg_ctx, fontID);   
    nvgFontSize(__nu_global_gui.vg_ctx, 14);

    #pragma omp parallel for
    for (uint16_t l=0; l<=__nu_global_gui.deepest_layer; l++)
    {
        NU_Layer* layer = &__nu_global_gui.tree.layers[l];

        // Iterate over layer
        for (int i=0; i<layer->size; i++)
        {       
            struct Node* node = &layer->node_array[i];
            if (!node->node_present) continue;


            if (node->text_content != NULL) 
            {
                // Compute available inner width
                float inner_width = node->width - node->border_left - node->border_right - node->pad_left - node->pad_right;

                // Measure text the same way it's drawn
                float bounds[4];
                nvgTextBoxBounds(__nu_global_gui.vg_ctx, 0, 0, inner_width, node->text_content, NULL, bounds);
                float text_height = bounds[3] - bounds[1];

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
    if (parent->layout_flags & LAYOUT_VERTICAL)
    {   
        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            float remaning_width = (parent->width - parent->pad_left - parent->pad_right - parent->border_left - parent->border_right) - child->width;
            float x_align_offset = remaning_width * 0.5f * (float)parent->horizontal_alignment;
            child->x = parent->x + parent->pad_left + parent->border_left + x_align_offset;
        }
    }
    else
    {
        // Calculate remaining width (optimise this by caching this calue inside parent's content width variable)
        float remaining_width = (parent->width - parent->pad_left - parent->pad_right - parent->border_left - parent->border_right) - (parent->child_count - 1) * parent->gap - (!!(parent->layout_flags & OVERFLOW_VERTICAL_SCROLL)) * 12.0f;
        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->tag == WINDOW) remaining_width += parent->gap;
            else remaining_width -= child->width;
        }

        float cursor_x = 0.0f;

        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            float x_align_offset = remaining_width * 0.5f * (float)parent->horizontal_alignment;
            child->x = parent->x + parent->pad_left + parent->border_left + cursor_x + x_align_offset;
            cursor_x += child->width + parent->gap;
        }
    }
}

static void NU_Vertically_Place_Children(struct Node* parent, NU_Layer* child_layer)
{
    float y_scroll_offset = 0.0f;
    if (parent->layout_flags & OVERFLOW_VERTICAL_SCROLL && parent->child_count > 0) 
    {
        float track_h = parent->height - parent->border_top - parent->border_bottom;
        float inner_height_w_pad = track_h - parent->pad_top - parent->pad_bottom;
        float inner_proportion_of_content_height = inner_height_w_pad / parent->content_height;
        float thumb_h = inner_proportion_of_content_height * track_h;
        float content_scroll_range = parent->content_height - inner_height_w_pad;
        float thumb_scroll_range = track_h - thumb_h;
        float scroll_factor = content_scroll_range / thumb_scroll_range;
        y_scroll_offset = (-parent->scroll_v) * scroll_factor;

        // Force thead to "stick"
        struct Node* first_child = NU_Layer_Get(child_layer, parent->first_child_index);
        if (first_child->tag == THEAD) {
            first_child->y -= y_scroll_offset;
        }
    }

    if (!(parent->layout_flags & LAYOUT_VERTICAL))
    {   
        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            float remaining_height = (parent->height - parent->pad_top - parent->pad_bottom - parent->border_top - parent->border_bottom) - child->height;
            float y_align_offset = remaining_height * 0.5f * (float)parent->vertical_alignment;
            child->y += parent->y + parent->pad_top + parent->border_top + y_align_offset + y_scroll_offset;
        }
    }
    else
    {
        // Calculate remaining height (optimise this by caching this calue inside parent's content height variable)
        float remaining_height = (parent->height - parent->pad_top - parent->pad_bottom - parent->border_top - parent->border_bottom) - (parent->child_count - 1) * parent->gap - (!!(parent->layout_flags & OVERFLOW_HORIZONTAL_SCROLL)) * 12.0f;
        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->tag == WINDOW) remaining_height += parent->gap;
            else remaining_height -= child->height;
        }

        float cursor_y = 0.0f;

        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            float y_align_offset = remaining_height * 0.5f * (float)parent->vertical_alignment;
            child->y += parent->y + parent->pad_top + parent->border_top + cursor_y + y_align_offset + y_scroll_offset;
            cursor_y += child->height + parent->gap;
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
            if (!parent->node_present) continue;

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

// Checks against bounding rect
static bool NU_Mouse_Over_Node_Bounds(struct Node* node, float mouse_x, float mouse_y)
{
    bool within_x_bound = mouse_x >= node->x && mouse_x <= node->x + node->width;
    bool within_y_bound = mouse_y >= node->y && mouse_y <= node->y + node->height;
    return (within_x_bound && within_y_bound);
}

// Checks against rounded corners
static bool NU_Mouse_Over_Node(struct Node* node, float mouse_x, float mouse_y)
{
    bool within_x_bound = mouse_x >= node->x && mouse_x <= node->x + node->width;
    bool within_y_bound = mouse_y >= node->y && mouse_y <= node->y + node->height;
    if (within_x_bound && within_y_bound)
    {
        // Must also consider border radius
        return true;
    }
    return false;
}

// Check against scrollbar v thumb
static bool NU_Mouse_Over_Node_V_Scrollbar(struct Node* node, float mouse_x, float mouse_y) {
    float scroll_thumb_left_wall = node->x + node->width - node->border_right - 12.0f;
    float scroll_thumb_top_wall = node->y + node->border_top + node->scroll_v;
    float inner_height = node->height - node->border_top - node->border_bottom;
    float thumb_height = (inner_height / node->content_height) * inner_height;
    bool within_x_bound = mouse_x >= scroll_thumb_left_wall && mouse_x <= scroll_thumb_left_wall + 12.0f;
    bool within_y_bound = mouse_y >= scroll_thumb_top_wall && mouse_y <= scroll_thumb_top_wall + thumb_height;
    return within_x_bound && within_y_bound;
}

void NU_Mouse_Hover()
{   
    // --------------------------------------
    // --- Clear ui tree hovered nodes vector
    // --------------------------------------
    if (__nu_global_gui.hovered_node != NULL && __nu_global_gui.hovered_node != __nu_global_gui.mouse_down_node) {
        NU_Apply_Stylesheet_To_Node(__nu_global_gui.hovered_node, __nu_global_gui.stylesheet);
    }
    __nu_global_gui.hovered_node = NULL;
    __nu_global_gui.scroll_hovered_node = NULL;

    if (__nu_global_gui.hovered_window == NULL) return;

    // -------------------------------------------------
    // --- Create a traversal stack and append root node
    // -------------------------------------------------
    struct Vector stack;
    Vector_Reserve(&stack, sizeof(struct Node*), 32);
    
    // ----------------------------------------------------
    // --- Find the window node that mouse is hovering over
    // ----------------------------------------------------
    for (int i=0; i<__nu_global_gui.window_nodes.size; i++)
    {
        struct Node* window_node = *(struct Node**) Vector_Get(&__nu_global_gui.window_nodes, i);
        if (window_node->window == __nu_global_gui.hovered_window) {
            Vector_Push(&stack, &window_node);
            break;
        }
    }

    // -----------------------------
    // --- Get global mouse position
    // -----------------------------
    float mouse_x, mouse_y, rel_x, rel_y;
    int window_x, window_y;
    SDL_GetGlobalMouseState(&mouse_x, &mouse_y);
    SDL_GetWindowPosition(__nu_global_gui.hovered_window, &window_x, &window_y);
    rel_x = mouse_x - window_x;
    rel_y = mouse_y - window_y;
    bool break_loop = false;
    while (stack.size > 0 && !break_loop) 
    {   
        // -----------------
        // --- Pop the stack
        // -----------------
        struct Node* current_node = *(struct Node**)Vector_Get(&stack, stack.size - 1);
        if (current_node->window != __nu_global_gui.hovered_window) break;
        stack.size -= 1;

        // ------------------------------
        // --- Iterate over node children
        // ------------------------------
        NU_Layer* child_layer = &__nu_global_gui.tree.layers[current_node->layer+1];
        for (uint16_t i=current_node->first_child_index; i<current_node->first_child_index + current_node->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);

            // ---------------------------------
            // --- If child is not a window node
            // ---------------------------------
            if (child->tag != WINDOW && NU_Mouse_Over_Node_Bounds(child, rel_x, rel_y)) 
            {
                __nu_global_gui.hovered_node = child;

                if (child->layout_flags & OVERFLOW_V_PROPERTY) {
                    bool overflow_v = child->content_height > child->height - child->border_top - child->border_bottom;
                    if (overflow_v && NU_Mouse_Over_Node_V_Scrollbar(child, rel_x, rel_y)) {
                        __nu_global_gui.scroll_hovered_node = child;
                    }
                }
                else if (child->tag == BUTTON) {
                    break_loop = true;
                    break;
                }

                Vector_Push(&stack, &child);
            }
        }
    }

    // ----------------------------------------
    // Apply hover pseudo style to hovered node
    // ----------------------------------------
    if (__nu_global_gui.hovered_node != NULL && __nu_global_gui.hovered_node != __nu_global_gui.mouse_down_node) {
        NU_Apply_Pseudo_Style_To_Node(__nu_global_gui.hovered_node, __nu_global_gui.stylesheet, PSEUDO_HOVER);
    } 
    Vector_Free(&stack);
}




// -----------------------------
// --- Rendering ---------------
// -----------------------------
void NU_Draw_Node_Text(struct Node* node, NU_Font* font, float screen_width, float screen_height)
{
    nvgFillColor(__nu_global_gui.vg_ctx, nvgRGB(node->text_r, node->text_g, node->text_b));

    // Compute inner dimensions (content area)
    float inner_width  = node->width  - node->border_left - node->border_right - node->pad_left - node->pad_right;
    float inner_height = node->height - node->border_top  - node->border_bottom - node->pad_top - node->pad_bottom;
    float remaining_w = inner_width  - node->content_width;
    float remaining_h = inner_height - node->content_height;
    float x_align_offset = remaining_w * 0.5f * (float)node->horizontal_text_alignment;
    float y_align_offset = remaining_h * 0.5f * (float)node->vertical_text_alignment;

    // Top-left corner of the content area
    float textPosX = node->x + node->border_left + node->pad_left + x_align_offset;
    float textPosY = node->y + node->border_top  + node->pad_top - font->descent * 0.5f + y_align_offset;

    // Draw wrapped text inside inner_width
    NU_Render_Text(font, node->text_content, floorf(textPosX), floorf(textPosY), inner_width, screen_width, screen_height);
    nvgTextBox(__nu_global_gui.vg_ctx, floorf(textPosX), floorf(textPosY), inner_width, node->text_content, NULL);
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

struct NU_Clipped_Node_Info {
    struct Node* node;
    float clip_top;
    float clip_bottom;
    float clip_left;
    float clip_right;
};

void NU_Draw()
{
    struct Vector window_nodes_list[__nu_global_gui.windows.size];
    struct Vector window_clipped_nodes_list[__nu_global_gui.windows.size];
    for (uint32_t i=0; i<__nu_global_gui.windows.size; i++) {
        Vector_Reserve(&window_nodes_list[i], sizeof(struct Node*), 512);
        Vector_Reserve(&window_clipped_nodes_list[i], sizeof(struct NU_Clipped_Node_Info), 8);
    }

    NU_Font* free_font = Vector_Get(&__nu_global_gui.fonts, 0);

    // ----------------------------------------------
    // --- Add visible nodes to window draw lists ---
    // ----------------------------------------------
    // Don't forget about the root
    struct Node* root = &__nu_global_gui.tree.layers[0].node_array[0];
    int root_w, root_h;
    SDL_Window* root_window = *(SDL_Window**) Vector_Get(&__nu_global_gui.windows, 0);
    SDL_GetWindowSize(root_window, &root_w, &root_h);
    if (Is_Node_Visible_In_Window(root, (float)root_w, (float)root_h)) {
        Vector_Push(&window_nodes_list[0], &root);
    }
    for (int l=0; l<=__nu_global_gui.deepest_layer; l++)
    {
        NU_Layer* parent_layer = &__nu_global_gui.tree.layers[l];
        NU_Layer* child_layer = &__nu_global_gui.tree.layers[l+1];

        // Iterate over parent layer
        for (int p=0; p<parent_layer->size; p++)
        {       
            struct Node* parent = NU_Layer_Get(parent_layer, p);
            if (parent->node_present != 1) continue;

            // Precompute once (optimisation)
            float parent_inner_x, parent_inner_y, parent_inner_width, parent_inner_height = 0;
            if (parent->layout_flags & OVERFLOW_VERTICAL_SCROLL) {
                parent_inner_y = parent->y + parent->border_top + parent->pad_top;
                parent_inner_height = parent->height - parent->border_top - parent->border_bottom - parent->pad_top - parent->pad_bottom;
            }
            if (parent->layout_flags & OVERFLOW_HORIZONTAL_SCROLL) {
                parent_inner_x = parent->x + parent->border_left + parent->pad_left;
                parent_inner_width = parent->width - parent->border_left - parent->border_right - parent->pad_left - parent->pad_right;
            }

            // Iterate over children
            for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
            {
                struct Node* child = NU_Layer_Get(child_layer, i);

                // Skip node if not visible in parent (due to overflow)
                if (parent->layout_flags & OVERFLOW_VERTICAL_SCROLL) {
                    int intersect_state = Node_Vertical_Overlap_State(child, parent_inner_y, parent_inner_height);

                    // Node not inside parent
                    if (intersect_state == 0) 
                    {
                        child->node_present = 2; // temporarily hidden
                        continue;
                    }

                    // Overlaps boundary
                    else if (intersect_state == 1) 
                    {
                        // Append node to correct window clipped node list
                        int window_index, w, h;
                        window_index = NU_Draw_Find_Window(child);
                        SDL_Window* window = *(SDL_Window**) Vector_Get(&__nu_global_gui.windows, window_index);
                        SDL_GetWindowSize(window, &w, &h);
                        if (Is_Node_Visible_In_Window(child, (float)w, (float)h)) 
                        {
                            float node_top = child->y;
                            float node_bottom = child->y + child->height;
                            float bounds_top    = parent_inner_y;
                            float bounds_bottom = parent_inner_y + parent_inner_height;
                            struct NU_Clipped_Node_Info info;
                            info.node = child;
                            info.clip_top = fmaxf(node_top - 1, bounds_top);
                            info.clip_bottom = fminf(node_bottom + 1, bounds_bottom);
                            info.clip_left = -1.0f;
                            info.clip_right = 1000000.0f;
                            Vector_Push(&window_clipped_nodes_list[window_index], &info);
                        }
                        child->node_present = 3;
                        continue;
                    }
                }

                // Append node to correct window node list
                int window_index, w, h;
                window_index = NU_Draw_Find_Window(child);
                SDL_Window* window = *(SDL_Window**) Vector_Get(&__nu_global_gui.windows, window_index);
                SDL_GetWindowSize(window, &w, &h);
                if (Is_Node_Visible_In_Window(child, (float)w, (float)h)) {
                    Vector_Push(&window_nodes_list[window_index], &child);
                }
            }
        }
    }


    // For each window
    for (uint32_t i=0; i<__nu_global_gui.windows.size; i++)
    {
        SDL_Window* window = *(SDL_Window**) Vector_Get(&__nu_global_gui.windows, i);
        SDL_GL_MakeCurrent(window, __nu_global_gui.gl_ctx);

        // ----------------------------------------
        // --- Clear the window and start new frame
        // ----------------------------------------
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        float w_fl = (float)w;
        float h_fl = (float)h;
        glViewport(0, 0, w, h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        nvgBeginFrame(__nu_global_gui.vg_ctx, w, h, 1.0f);

        // ------------------------------------------------------------------------------------------------------------------------------------
        // Create single border rect vertex and index buffer for non-overlapped visible nodes (optimisation to draw all border rects in one go)
        // ------------------------------------------------------------------------------------------------------------------------------------
        Vertex_RGB_List border_rect_vertices;
        Index_List border_rect_indices;
        Vertex_RGB_List_Init(&border_rect_vertices, 5000);
        Index_List_Init(&border_rect_indices, 15000);

        // Construct border rect vertices and indices for each node
        for (int n=0; n<window_nodes_list[i].size; n++) {
            struct Node* node = *(struct Node**) Vector_Get(&window_nodes_list[i], n);
            Construct_Border_Rect(node, w_fl, h_fl, &border_rect_vertices, &border_rect_indices);
            if (node->layout_flags & OVERFLOW_VERTICAL_SCROLL && node->content_height > (node->height + node->pad_top + node->pad_bottom + node->border_top + node->border_bottom)) {
                Construct_Scroll_Thumb(node, w_fl, h_fl, &border_rect_vertices, &border_rect_indices);
            }
        }

        // Draw all border rects in one pass
        Draw_Vertex_RGB_List(&border_rect_vertices, &border_rect_indices, w_fl, h_fl);
        Vertex_RGB_List_Free(&border_rect_vertices);
        Index_List_Free(&border_rect_indices);
        // ------------------------------------------------------------------------------------------------------------------------------------
        // Create single border rect vertex and index buffer for non-overlapped visible nodes (optimisation to draw all border rects in one go)
        // ------------------------------------------------------------------------------------------------------------------------------------
    



        // ---------------------------------------------------------
        // --- Draw all images, text and partially visibly nodes ---
        // ---------------------------------------------------------
        int fontID = *(int*) Vector_Get(&__nu_global_gui.font_registry, 0); // Select font
        nvgFontFaceId(__nu_global_gui.vg_ctx, fontID);   
        nvgFontSize(__nu_global_gui.vg_ctx, 14);
        nvgTextAlign(__nu_global_gui.vg_ctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        float asc, desc, lh;
        nvgTextMetrics(__nu_global_gui.vg_ctx, &asc, &desc, &lh);

        // Draw all clipped nodes (partially occluded)
        for (int n=0; n<window_clipped_nodes_list[i].size; n++) {
            struct NU_Clipped_Node_Info* info = (struct NU_Clipped_Node_Info*) Vector_Get(&window_clipped_nodes_list[i], n);
            
            Vertex_RGB_List vertices;
            Index_List indices;
            Vertex_RGB_List_Init(&vertices, 100);
            Index_List_Init(&indices, 100);
            Construct_Border_Rect(info->node, w_fl, h_fl, &vertices, &indices);
            Draw_Clipped_Vertex_RGB_List(
                &vertices, 
                &indices, 
                w_fl, h_fl, 
                info->clip_top, 
                info->clip_bottom, 
                info->clip_left, 
                info->clip_right
            );
            Vertex_RGB_List_Free(&vertices);
            Index_List_Free(&indices);
        }

        // Draw images and text for each node
        for (int n=0; n<window_nodes_list[i].size; n++) {
            struct Node* node = *(struct Node**) Vector_Get(&window_nodes_list[i], n);

            if (node->node_present == 3) continue;

            // Draw image
            if (node->gl_image_handle) {
                float inner_width  = node->width - node->border_left - node->border_right - node->pad_left - node->pad_right;
                float inner_height = node->height - node->border_top - node->border_bottom - node->pad_top - node->pad_bottom;
                Draw_Image(node->x + node->border_left + node->pad_left, node->y + node->border_top + node->pad_top, inner_width, inner_height, (float)w, (float)h, node->gl_image_handle);
            }

            // Draw text
            if (node->text_content != NULL) NU_Draw_Node_Text(node, free_font, w_fl, h_fl);
        }
        // --------------------------------
        // --- Draw all images and text ---
        // --------------------------------



        // ------------------------------

        // --- End frame and swap buffers
        // ------------------------------
        nvgEndFrame(__nu_global_gui.vg_ctx);
        SDL_GL_SwapWindow(window); 
    }

    for (uint32_t i=0; i<__nu_global_gui.windows.size; i++) {
        Vector_Free(&window_nodes_list[i]);
    }

    __nu_global_gui.awaiting_draw = false;
}

void NU_Reflow()
{
    timer_start();
    NU_Clear_Node_Sizes();            // Reset dimensions and positions
    NU_Calculate_Text_Fit_Widths();   // Width and height of unwrapped text nodes
    NU_Calculate_Fit_Size_Widths();   
    NU_Grow_Shrink_Widths();
    NU_Calculate_Table_Column_Widths();
    NU_Calculate_Text_Heights();
    NU_Calculate_Fit_Size_Heights();
    NU_Grow_Shrink_Heights();
    NU_Calculate_Positions();
    timer_stop();
}