#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>
#include <freetype/freetype.h>
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <stdbool.h>
#include <math.h>
#include "nu_draw.h"
#include "nu_window.h"


// ---------------------------
// --- UI layout -------------
// ---------------------------
static void NU_Reset_Node_size(struct Node* node)
{
    node->x = 0.0f;
    node->y = 0.0f;
    node->width = max(node->preferred_width, node->border_left + node->border_right + node->pad_left + node->pad_right);
    node->height = max(node->preferred_height, node->border_top + node->border_bottom + node->pad_top + node->pad_bottom);
}

static void NU_Clear_Node_Sizes()
{
    // Don't skip the root
    struct Node* root = NU_Tree_Get(&__nu_global_gui.tree, 0, 0); 
    NU_Reset_Node_size(root);
    SDL_SetWindowMinimumSize(root->window, root->min_width, root->min_height);

    // For each layer
    for (uint16_t l=0; l<=__nu_global_gui.deepest_layer; l++)
    {
        NU_Layer* parent_layer = &__nu_global_gui.tree.layers[l];
        NU_Layer* child_layer = &__nu_global_gui.tree.layers[l+1];

        for (int p=0; p<parent_layer->size; p++)
        {       
            struct Node* parent = NU_Layer_Get(parent_layer, p);
            if (!parent->node_present) continue;

            // If parent is window node and has no SDL window assigned to it -> create a new window and renderer
            if (parent->tag == WINDOW && l != 0) {
                SDL_SetWindowMinimumSize(parent->window, parent->min_width, parent->min_height);
            }

            if (parent->child_count == 0) continue; // Skip acummulating child sizes (no children)

            for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
            {
                struct Node* child = NU_Layer_Get(child_layer, i);

                // Inherit window and renderer from parent
                if (child->tag != WINDOW && child->window == NULL) {
                    child->window = parent->window;
                }

                NU_Reset_Node_size(child);
            }
        }
    }
}

static void NU_Calculate_Text_Min_Width(struct Node* node)
{
    const char* text = node->text_content;
    if (!text || !*text) return; // empty string guard

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
        float bounds[4];
        nvgTextBounds(__nu_global_gui.vg_ctx, 0, 0, text, NULL, bounds);
        max_word_width = bounds[2] - bounds[0];
    }

    float text_controlled_min_width = max_word_width + node->pad_left + node->pad_right + node->border_left + node->border_right;
    node->min_width = max(node->min_width, text_controlled_min_width);
}

static void NU_Calculate_Text_Fit_Size(struct Node* node)
{
    // Make sure the NanoVG context has the correct font/size set before measuring!
    int fontID = *(int*) Vector_Get(&__nu_global_gui.font_registry, 0);
    nvgFontFaceId(__nu_global_gui.vg_ctx, fontID);   
    nvgFontSize(__nu_global_gui.vg_ctx, 18);

    // Calculate text bounds
    float asc, desc, lh;
    float bounds[4];
    nvgTextMetrics(__nu_global_gui.vg_ctx, &asc, &desc, &lh);
    nvgTextBounds(__nu_global_gui.vg_ctx, 0, 0, node->text_content, NULL, bounds);
    float text_width = bounds[2] - bounds[0];
    float text_height = lh;
    
    NU_Calculate_Text_Min_Width(node);
    
    if (node->preferred_width == 0.0f) {
        node->width = text_width + node->pad_left + node->pad_right + node->border_left + node->border_right;
    }
    node->width = min(node->width, node->max_width);
    node->width = max(node->width, node->min_width);
    node->height += text_height;
    node->content_width = text_width;
}

static void NU_Calculate_Text_Fit_Sizes()
{
    // Don't skip the root
    struct Node* root = NU_Tree_Get(&__nu_global_gui.tree, 0, 0); 
    if (root->text_content != NULL) {
        NU_Calculate_Text_Fit_Size(root);
    }


    // For each layer
    for (uint16_t l=0; l<=__nu_global_gui.deepest_layer; l++)
    {
        NU_Layer* layer = &__nu_global_gui.tree.layers[l];
        for (uint32_t n=0; n<layer->size; n++)
        {   
            struct Node* node = NU_Layer_Get(layer, n);
            if (!node->node_present) continue;

            if (node->text_content != NULL) {
                NU_Calculate_Text_Fit_Size(node);
            }
        }
    }
}

static void NU_Calculate_Fit_Size_Widths()
{
    if (__nu_global_gui.deepest_layer == 0) return;

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

            // Track the total width for the parent's content
            float content_width = 0;

            for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
            {
                struct Node* child = NU_Layer_Get(child_layer, i);

                if (child->tag == WINDOW) {
                    if (is_layout_horizontal) content_width -= parent->gap + child->width;
                }   
                if (is_layout_horizontal) { // Horizontal Layout
                    content_width += child->width;
                } else { // Vertical Layout
                    content_width = MAX(content_width, child->width);
                }
            }

            // Grow parent node
            if (is_layout_horizontal) content_width += (parent->child_count - 1) * parent->gap;
            parent->content_width = content_width;
            if (parent->tag != WINDOW && content_width > parent->width) {
                parent->width = content_width + parent->border_left + parent->border_right + parent->pad_left + parent->pad_right;
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
            
            // Track the total height for the parent's content
            float content_height = 0;

            // Iterate over children
            for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
            {
                struct Node* child = NU_Layer_Get(child_layer, i);

                if (child->tag == WINDOW) {
                    if (!is_layout_horizontal) content_height -= parent->gap + child->height;
                }   
                if (is_layout_horizontal) { // Horizontal Layout
                    content_height = MAX(content_height, child->height);
                } else { // Vertical Layout
                    content_height += child->height;
                }
            }

            // Grow parent node
            if (!is_layout_horizontal) content_height += (parent->child_count - 1) * parent->gap;
            parent->content_height = content_height;
            if (parent->tag != WINDOW && content_height > parent->height) {
                parent->height = content_height + parent->border_top + parent->border_bottom + parent->pad_top + parent->pad_bottom;
            }
        }
    }
}

static void NU_Grow_Shrink_Child_Node_Widths(struct Node* parent, NU_Layer* child_layer)
{
    float remaining_width = parent->width - parent->pad_left - parent->pad_right - parent->border_left - parent->border_right - ((parent->layout_flags & OVERFLOW_VERTICAL_SCROLL) == 1) * 12.0f;

    if (parent->layout_flags & LAYOUT_VERTICAL)
    {   
        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->layout_flags & GROW_HORIZONTAL && remaining_width > child->width)
            {
                float pad_and_border = child->pad_left + child->pad_right + child->border_left + child->border_right;
                child->width = remaining_width; 
                child->width = min(child->width, child->max_width);
                child->width = max(child->width, max(child->min_width, pad_and_border));
                parent->content_width = max(parent->content_width, child->width);
            }
        }
    }
    else
    {
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

        // Grow elements
        while (remaining_width > 0.01f)
        {   
            // Determine smallest, second smallest and growable count
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

            // Calculate width to add
            float width_to_add = remaining_width / (float)growable_count;
            if (second_smallest > smallest) {
                width_to_add = min(width_to_add, second_smallest - smallest);
            }

            // for each child
            bool grew_any = false;
            for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++) {
                struct Node* child = NU_Layer_Get(child_layer, i);            
                if (child->layout_flags & GROW_HORIZONTAL && child->tag != WINDOW && child->width < child->max_width) {// if child is growable
                    if (child->width == smallest) {
                        float available = child->max_width - child->width;
                        float grow = min(width_to_add, available);
                        if (grow > 0.0f) {
                            parent->content_height += grow;
                            child->width += grow;
                            remaining_width -= grow;
                            grew_any = true;
                        }
                    }
                }
            }
            if (!grew_any) break;
        }

        // Shrink elements
        while (remaining_width < -0.01f)
        {
            // Determine largest, second largest and shrinkable count
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

            // Calculate width to subtract
            float width_to_subtract = -remaining_width / (float)shrinkable_count;
            if (second_largest < largest && second_largest >= 0) {
                width_to_subtract = min(width_to_subtract, largest - second_largest);
            }

            // For each child
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

    // Enforce min parent width based on content
    parent->width = max(parent->width, parent->content_width + parent->pad_left + parent->pad_right + parent->border_left + parent->border_right);
}

static void NU_Grow_Shrink_Child_Node_Heights(struct Node* parent, NU_Layer* child_layer)
{
    float remaining_height = parent->height - parent->pad_top - parent->pad_bottom - parent->border_top - parent->border_bottom - ((parent->layout_flags & OVERFLOW_HORIZONTAL_SCROLL) == 1) * 12.0f;

    if (!(parent->layout_flags & LAYOUT_VERTICAL))
    {
        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            if (child->layout_flags & GROW_VERTICAL && remaining_height > child->height)
            {  
                float pad_and_border = child->pad_top + child->pad_bottom + child->border_top + child->border_bottom;
                child->height = remaining_height; 
                child->height = min(child->height, child->max_height);
                child->height = max(child->height, max(child->min_height, pad_and_border));
                parent->content_height = max(parent->content_height, child->height);
            }
        }
    }
    else
    {
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

        while (remaining_height > 0.01f)
        {
            // Find smallest and second smallest
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
            
            // Calculate height to add
            float height_to_add = remaining_height / (float)growable_count;
            if (second_smallest > smallest) { 
                height_to_add = min(height_to_add, second_smallest - smallest);
            }

            // for each child
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
    }

    // Enforce min parent height based on content
    parent->height = max(parent->height, parent->content_height + parent->pad_top + parent->pad_bottom + parent->border_top + parent->border_bottom);
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
            if (!parent->node_present) continue;

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
            if (!parent->node_present) continue;

            NU_Grow_Shrink_Child_Node_Heights(parent, child_layer);
        }
    }
}

static void NU_Calculate_Text_Wrap_Height(struct Node* node)
{
    // Configure font
    int fontID = *(int*) Vector_Get(&__nu_global_gui.font_registry, 0);
    nvgFontFaceId(__nu_global_gui.vg_ctx, fontID);   
    nvgFontSize(__nu_global_gui.vg_ctx, 18);

    // Compute available inner width
    float inner_width = node->width - node->border_left - node->border_right - node->pad_left - node->pad_right;

    // Measure text the same way it's drawn
    float bounds[4];
    nvgTextBoxBounds(__nu_global_gui.vg_ctx, 0, 0, inner_width, node->text_content, NULL, bounds);
    float text_height = bounds[3] - bounds[1];
    node->height = node->border_top + node->border_bottom + node->pad_top + node->pad_bottom + text_height;
    node->content_height = text_height;
}

static void NU_Calculate_Text_Wrap_Heights()
{
    // Don't skip the root
    struct Node* root = NU_Tree_Get(&__nu_global_gui.tree, 0, 0); 
    if (root->text_content != NULL) {
        NU_Calculate_Text_Wrap_Height(root);
    }

    for (uint16_t l=0; l<=__nu_global_gui.deepest_layer; l++)
    {
        NU_Layer* layer = &__nu_global_gui.tree.layers[l];

        // Iterate over layer
        for (int i=0; i<layer->size; i++)
        {       
            struct Node* node = NU_Layer_Get(layer, i);
            if (!node->node_present) continue;
            if (node->text_content != NULL) {
                NU_Calculate_Text_Wrap_Height(node);
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
        float remaining_width = (parent->width - parent->pad_left - parent->pad_right - parent->border_left - parent->border_right) - (parent->child_count - 1) * parent->gap - ((parent->layout_flags & OVERFLOW_VERTICAL_SCROLL) != 0) * 12.0f;
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
    if (!(parent->layout_flags & LAYOUT_VERTICAL))
    {   
        for (uint16_t i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = NU_Layer_Get(child_layer, i);
            float remaining_height = (parent->height - parent->pad_top - parent->pad_bottom - parent->border_top - parent->border_bottom) - child->height;
            float y_align_offset = remaining_height * 0.5f * (float)parent->vertical_alignment;
            child->y = parent->y + parent->pad_top + parent->border_top + y_align_offset;
        }
    }
    else
    {
        // Calculate remaining height (optimise this by caching this calue inside parent's content height variable)
        float remaining_height = (parent->height - parent->pad_top - parent->pad_bottom - parent->border_top - parent->border_bottom) - (parent->child_count - 1) * parent->gap - ((parent->layout_flags & OVERFLOW_HORIZONTAL_SCROLL) != 0) * 12.0f;
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
            child->y = parent->y + parent->pad_top + parent->border_top + cursor_y + y_align_offset;
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

void NU_Mouse_Hover()
{   
    // --------------------------------------
    // --- Clear ui tree hovered nodes vector
    // --------------------------------------
    if (__nu_global_gui.hovered_node != NULL) {
        NU_Apply_Stylesheet_To_Node(__nu_global_gui.hovered_node, __nu_global_gui.stylesheet);
        __nu_global_gui.hovered_node = NULL;
    }

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
        // Pop the stack
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
                if (child->tag == BUTTON) {
                    break_loop = true;
                    break;
                }

                Vector_Push(&stack, &child);
            }
        }
    }

    // Set currently hovered node
    if (__nu_global_gui.hovered_node != NULL) {
        NU_Apply_Pseudo_Style_To_Node(__nu_global_gui.hovered_node, __nu_global_gui.stylesheet, PSEUDO_HOVER);
    } 
    Vector_Free(&stack);
}




// -----------------------------
// --- Rendering ---------------
// -----------------------------
void NU_Draw_Node_Text(struct Node* node, NVGcontext* vg)
{
    // Setup font
    int fontID = *(int*) Vector_Get(&__nu_global_gui.font_registry, 0);
    nvgFontFaceId(vg, fontID);   
    nvgFontSize(vg, 18);

    // Text color and alignment
    nvgFillColor(vg, nvgRGB(node->text_r, node->text_g, node->text_b));
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

    float asc, desc, lh;
    nvgTextMetrics(__nu_global_gui.vg_ctx, &asc, &desc, &lh);

    // Compute inner dimensions (content area)
    float inner_width  = node->width  - node->border_left - node->border_right - node->pad_left - node->pad_right;
    float inner_height = node->height - node->border_top  - node->border_bottom - node->pad_top - node->pad_bottom;



    float remaining_w = inner_width  - node->content_width;
    float remaining_h = inner_height - node->content_height;
    float x_align_offset = remaining_w * 0.5f * (float)node->horizontal_text_alignment;
    float y_align_offset = remaining_h * 0.5f * (float)node->vertical_text_alignment;

    // printf("x align offset: %f\n", x_align_offset);
    // printf("horizontal align val: %f\n", (float)node->horizontal_text_alignment);

    // Top-left corner of the content area
    float textPosX = node->x + node->border_left + node->pad_left + x_align_offset;
    float textPosY = node->y + node->border_top  + node->pad_top - desc * 0.5f + y_align_offset;


    // Draw wrapped text inside inner_width
    nvgTextBox(vg, floorf(textPosX), floorf(textPosY), inner_width, node->text_content, NULL);
}

void NU_Draw()
{
    struct Vector window_nodes_list[MAX_TREE_DEPTH];
    for (uint32_t i=0; i<__nu_global_gui.windows.size; i++) {
        Vector_Reserve(&window_nodes_list[i], sizeof(struct Node*), 1000);
    }

    // For each layer
    for (int l=0; l<=__nu_global_gui.deepest_layer; l++)
    {
        NU_Layer* layer = &__nu_global_gui.tree.layers[l];

        // Iterate over layer
        for (int n=0; n<layer->size; n++)
        {   
            struct Node* node = NU_Layer_Get(layer, n);
            if (!node->node_present) continue;

            // Append node to corresponding window_node list
            for (uint32_t i=0; i<__nu_global_gui.windows.size; i++)
            {
                SDL_Window* window = *(SDL_Window**) Vector_Get(&__nu_global_gui.windows, i);
                if (window == node->window)
                {
                    Vector_Push(&window_nodes_list[i], &node);
                    break;
                }
            }
        }
    }


    // For each window
    for (uint32_t i=0; i<__nu_global_gui.windows.size; i++)
    {
        SDL_Window* window = *(SDL_Window**) Vector_Get(&__nu_global_gui.windows, i);
        SDL_GL_MakeCurrent(window, __nu_global_gui.gl_ctx);

        // Clear the window
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        nvgBeginFrame(__nu_global_gui.vg_ctx, w, h, 1.0f);

        // Create border rect vertex and index lists (optimisation to draw all border rects in one go)
        Vertex_RGB_List border_rect_vertices;
        Index_List border_rect_indices;
        Vertex_RGB_List_Init(&border_rect_vertices, 5000);
        Index_List_Init(&border_rect_indices, 15000);


        // Construct border rect vertices and indices for each node
        for (int n=0; n<window_nodes_list[i].size; n++)
        {
            struct Node* node = *(struct Node**) Vector_Get(&window_nodes_list[i], n);
            Construct_Border_Rect(node, (float)w, (float)h, &border_rect_vertices, &border_rect_indices);
        }

        // Draw all border rects in one pass
        Draw_Vertex_RGB_List(&border_rect_vertices, &border_rect_indices, (float)w, (float)h);
        Vertex_RGB_List_Free(&border_rect_vertices);
        Index_List_Free(&border_rect_indices);

        // Draw images and text for each node
        for (int n=0; n<window_nodes_list[i].size; n++)
        {
            struct Node* node = *(struct Node**) Vector_Get(&window_nodes_list[i], n);

            // Draw image
            if (node->gl_image_handle) {
                float inner_width  = node->width - node->border_left - node->border_right - node->pad_left - node->pad_right;
                float inner_height = node->height - node->border_top - node->border_bottom - node->pad_top - node->pad_bottom;
                Draw_Image(node->x + node->border_left + node->pad_left, node->y + node->border_top + node->pad_top, inner_width, inner_height, (float)w, (float)h, node->gl_image_handle);
            }

            // Draw text
            if (node->text_content != NULL)
            {
                NU_Draw_Node_Text(node, __nu_global_gui.vg_ctx);
            }
        }



        // Render to window
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
    NU_Clear_Node_Sizes();
    NU_Calculate_Text_Fit_Sizes();
    NU_Calculate_Fit_Size_Widths();
    NU_Grow_Shrink_Widths();
    NU_Calculate_Text_Wrap_Heights();
    NU_Calculate_Fit_Size_Heights();
    NU_Grow_Shrink_Heights();
    NU_Calculate_Positions();
}