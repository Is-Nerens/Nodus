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

static void NU_Clear_Node_Sizes(struct NU_GUI* ngui)
{
    // Don't skip the root
    struct Node* root = Vector_Get(&ngui->tree_stack[0], 0);
    NU_Reset_Node_size(root);
    SDL_SetWindowMinimumSize(root->window, root->min_width, root->min_height);

    // For each layer
    for (int l=0; l<=ngui->deepest_layer; l++)
    {
        struct Vector* parent_layer = &ngui->tree_stack[l];
        struct Vector* child_layer = &ngui->tree_stack[l+1];
        
        for (int p=0; p<parent_layer->size; p++)
        {       
            // Iterate over layer
            struct Node* parent = Vector_Get(parent_layer, p);

            // If parent is window node and has no SDL window assigned to it -> create a new window and renderer
            if (parent->tag == WINDOW && parent->window == NULL && l != 0) {
                NU_Create_Subwindow(ngui, parent);
                SDL_SetWindowMinimumSize(parent->window, parent->min_width, parent->min_height);
            }

            if (parent->child_count == 0) continue; // Skip acummulating child sizes (no children)

            for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
            {
                struct Node* child = Vector_Get(child_layer, i);

                // Inherit window and renderer from parent
                if (child->tag != WINDOW && child->window == NULL)
                {
                    child->window = parent->window;
                }

                NU_Reset_Node_size(child);
            }
        }
    }
}

static void NU_Calculate_Text_Min_Width(struct NU_GUI* ngui, struct Node* node, struct Text_Ref* text_ref)
{
    char* text = ngui->text_arena.char_buffer.data + text_ref->buffer_index;
    int slice_start = 0;
    float max_word_width = 0.0f;
    for (int i=0; i<=text_ref->char_count; i++) {
        char c = (i < text_ref->char_count) ? text[i] : ' '; 
        if (c == ' ') {
            if (i > slice_start) {
                float bounds[4];
                nvgTextBounds(ngui->vg_ctx, 0, 0, text + slice_start, text + i, bounds); // measure slice
                float width = bounds[2] - bounds[0];
                if (width > max_word_width) max_word_width = width;
            }
            slice_start = i + 1; 
        }
    }
    if (max_word_width == 0.0f && text_ref->char_count > 0) { // If no spaces found, the whole text is one word
        float bounds[4];
        nvgTextBounds(ngui->vg_ctx, 0, 0, text, text + text_ref->char_count, bounds);
        max_word_width = bounds[2] - bounds[0];
    }
    float text_controlled_min_width = max_word_width + node->pad_left + node->pad_right + node->border_left + node->border_right;
    node->min_width = max(node->min_width, text_controlled_min_width);
}

static void NU_Calculate_Text_Fit_Size(struct NU_GUI* ngui, struct Node* node, struct Text_Ref* text_ref)
{
    // Extract pointer to text
    char* text = ngui->text_arena.char_buffer.data + text_ref->buffer_index;

    // Make sure the NanoVG context has the correct font/size set before measuring!
    int fontID = *(int*) Vector_Get(&ngui->font_registry, 0);
    nvgFontFaceId(ngui->vg_ctx, fontID);   
    nvgFontSize(ngui->vg_ctx, 18);

    // Calculate text bounds
    float asc, desc, lh;
    float bounds[4];
    nvgTextMetrics(ngui->vg_ctx, &asc, &desc, &lh);
    nvgTextBounds(ngui->vg_ctx, 0, 0, text, text + text_ref->char_count, bounds);
    float text_width = bounds[2] - bounds[0];
    float text_height = lh;
    
    NU_Calculate_Text_Min_Width(ngui, node, text_ref);
    
    if (node->preferred_width == 0.0f) {
        node->width = text_width + node->pad_left + node->pad_right + node->border_left + node->border_right;
    }
    node->width = min(node->width, node->max_width);
    node->width = max(node->width, node->min_width);
    node->height += text_height;
}

static void NU_Calculate_Text_Fit_Sizes(struct NU_GUI* ngui)
{
    for (uint32_t i=0; i<ngui->text_arena.text_refs.size; i++)
    {
        struct Text_Ref* text_ref = (struct Text_Ref*) Vector_Get(&ngui->text_arena.text_refs, i);
        
        // Get the corresponding node
        uint8_t node_depth = (uint8_t)(text_ref->node_ID >> 24);
        uint32_t node_index = text_ref->node_ID & 0x00FFFFFF;
        struct Vector* layer = &ngui->tree_stack[node_depth];
        struct Node* node = Vector_Get(layer, node_index);

        // Calculate text size
        NU_Calculate_Text_Fit_Size(ngui, node, text_ref);
    }
}

static void NU_Calculate_Fit_Size_Widths(struct NU_GUI* ngui)
{
    if (ngui->deepest_layer == 0) return;

    // For each layer
    for (int l=ngui->deepest_layer; l>=0; l--)
    {
        struct Vector* parent_layer = &ngui->tree_stack[l];
        struct Vector* child_layer = &ngui->tree_stack[l+1];
        
        // Iterate over layer
        for (int p=0; p<parent_layer->size; p++)
        {   
            struct Node* parent = Vector_Get(parent_layer, p);
            int is_layout_horizontal = (parent->layout_flags & 0x01) == LAYOUT_HORIZONTAL;

            if (parent->tag == WINDOW) {
                int window_width, window_height;
                SDL_GetWindowSize(parent->window, &window_width, &window_height);
                parent->width = (float) window_width;
                parent->height = (float) window_height;
            }

            if (parent->child_count == 0) {
                continue; // Skip acummulating child sizes (no children)
            }
            
            // Track the total width for the parent's content
            float content_width = 0;

            // Iterate over children
            for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
            {
                struct Node* child = Vector_Get(child_layer, i);

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
            if (parent->tag != WINDOW) {
                parent->width = content_width + parent->border_left + parent->border_right + parent->pad_left + parent->pad_right;
            }
        }
    }
}

static void NU_Calculate_Fit_Size_Heights(struct NU_GUI* ngui)
{
    if (ngui->deepest_layer == 0) return;

    // For each layer
    for (int l=ngui->deepest_layer; l>=0; l--)
    {
        struct Vector* parent_layer = &ngui->tree_stack[l];
        struct Vector* child_layer = &ngui->tree_stack[l+1];
        
        // Iterate over layer
        for (int p=0; p<parent_layer->size; p++)
        {   
            struct Node* parent = Vector_Get(parent_layer, p);
            int is_layout_horizontal = !(parent->layout_flags & LAYOUT_VERTICAL);

            if (parent->child_count == 0) {
                continue; // Skip acummulating child sizes (no children)
            }
            
            // Track the total height for the parent's content
            float content_height = 0;

            // Iterate over children
            for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
            {
                struct Node* child = Vector_Get(child_layer, i);

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
            if (parent->tag != WINDOW) {
                parent->height += content_height;
            }
        }
    }
}

static void NU_Grow_Shrink_Child_Node_Widths(struct Node* parent, struct Vector* child_layer)
{
    float remaining_width = parent->width - parent->pad_left - parent->pad_right - parent->border_left - parent->border_right - ((parent->layout_flags & OVERFLOW_VERTICAL_SCROLL) != 0) * 12.0f;

    if (parent->layout_flags & LAYOUT_VERTICAL)
    {   
        for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = Vector_Get(child_layer, i);
            if (child->layout_flags & GROW_HORIZONTAL)
            {
                float pad_and_border = child->pad_left + child->pad_right + child->border_left + child->border_right;
                child->width = remaining_width; 
                child->width = min(child->width, child->max_width);
                child->width = max(child->width, max(child->min_width, pad_and_border));
            }
        }
    }
    else
    {
        uint32_t growable_count = 0;
        for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = Vector_Get(child_layer, i);
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
            for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++) {
                struct Node* child = Vector_Get(child_layer, i);
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
            for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++) {
                struct Node* child = Vector_Get(child_layer, i);            
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
            
            for (int i = parent->first_child_index; i < parent->first_child_index + parent->child_count; i++) {
                struct Node* child = Vector_Get(child_layer, i);
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
            for (int i = parent->first_child_index; i < parent->first_child_index + parent->child_count; i++) {
                struct Node* child = Vector_Get(child_layer, i);
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

        // Enforce min parent width based on content
        parent->width = max(parent->width, parent->content_width + parent->pad_left + parent->pad_right + parent->border_left + parent->border_right);
    }
}

static void NU_Grow_Shrink_Child_Node_Heights(struct Node* parent, struct Vector* child_layer)
{
    float remaining_height = parent->height - parent->pad_top - parent->pad_bottom - parent->border_top - parent->border_bottom - ((parent->layout_flags & OVERFLOW_HORIZONTAL_SCROLL) != 0) * 12.0f;

    if (!(parent->layout_flags & LAYOUT_VERTICAL))
    {
        for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = Vector_Get(child_layer, i);
            if (child->layout_flags & GROW_VERTICAL)
            {   
                float pad_and_border = child->pad_top + child->pad_bottom + child->border_top + child->border_bottom;
                child->height = remaining_height; 
                child->height = min(child->height, child->max_height);
                child->height = max(child->height, max(child->min_height, pad_and_border));
            }
        }
    }
    else
    {
        uint32_t growable_count = 0;
        for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = Vector_Get(child_layer, i);
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
            for (int i=parent->first_child_index; i< parent->first_child_index + parent->child_count; i++) {
                struct Node* child = Vector_Get(child_layer, i);
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
            for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++) {
                struct Node* child = Vector_Get(child_layer, i);
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

        // Enforce min parent height based on content
        parent->height = max(parent->height, parent->content_height + parent->pad_top + parent->pad_bottom + parent->border_top + parent->border_bottom);
    }
}

static void NU_Grow_Shrink_Widths(struct NU_GUI* ngui)
{
    for (int l=0; l<=ngui->deepest_layer; l++) // For each layer
    {
        struct Vector* parent_layer = &ngui->tree_stack[l];
        struct Vector* child_layer = &ngui->tree_stack[l+1];
        
        for (int p=0; p<parent_layer->size; p++) // For node in layer
        {   
            struct Node* parent = Vector_Get(parent_layer, p);
            NU_Grow_Shrink_Child_Node_Widths(parent, child_layer);
        }
    }
}

static void NU_Grow_Shrink_Heights(struct NU_GUI* ngui)
{
    for (int l=0; l<=ngui->deepest_layer; l++) // For each layer
    {
        struct Vector* parent_layer = &ngui->tree_stack[l];
        struct Vector* child_layer = &ngui->tree_stack[l+1];
        
        for (int p=0; p<parent_layer->size; p++) // For node in layer
        {   
            struct Node* parent = Vector_Get(parent_layer, p);
            NU_Grow_Shrink_Child_Node_Heights(parent, child_layer);
        }
    }
}

static void NU_Calculate_Text_Wrap_Heights(struct NU_GUI* ngui)
{
    for (uint32_t i = 0; i < ngui->text_arena.text_refs.size; i++) {
        struct Text_Ref* text_ref = (struct Text_Ref*) Vector_Get(&ngui->text_arena.text_refs, i);
        
        // Get the corresponding node
        uint8_t node_depth = (uint8_t)(text_ref->node_ID >> 24);
        uint32_t node_index = text_ref->node_ID & 0x00FFFFFF;
        struct Vector* layer = &ngui->tree_stack[node_depth];
        struct Node* node = Vector_Get(layer, node_index);

        char* text = ngui->text_arena.char_buffer.data + text_ref->buffer_index;

        // Configure font
        int fontID = *(int*) Vector_Get(&ngui->font_registry, 0);
        nvgFontFaceId(ngui->vg_ctx, fontID);   
        nvgFontSize(ngui->vg_ctx, 18);

        // Compute available inner width
        float inner_width = node->width -
                            node->border_left - node->border_right -
                            node->pad_left - node->pad_right;

        // Measure text the same way it's drawn
        float bounds[4];
        nvgTextBoxBounds(ngui->vg_ctx, 0, 0, inner_width, text, NULL, bounds);
        float text_height = bounds[3] - bounds[1];

        node->height = node->border_top + node->border_bottom +
                       node->pad_top + node->pad_bottom +
                       text_height;
    }
}

static void NU_Horizontally_Place_Children(struct Node* parent, struct Vector* child_layer)
{
    if (parent->layout_flags & LAYOUT_VERTICAL)
    {   
        for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = Vector_Get(child_layer, i);
            float remaning_width = (parent->width - parent->pad_left - parent->pad_right - parent->border_left - parent->border_right) - child->width;
            float x_align_offset = remaning_width * 0.5f * (float)parent->horizontal_alignment;
            child->x = parent->x + parent->pad_left + parent->border_left + x_align_offset;
        }
    }
    else
    {
        // Calculate remaining width (optimise this by caching this calue inside parent's content width variable)
        float remaining_width = (parent->width - parent->pad_left - parent->pad_right - parent->border_left - parent->border_right) - (parent->child_count - 1) * parent->gap - ((parent->layout_flags & OVERFLOW_VERTICAL_SCROLL) != 0) * 12.0f;
        for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = Vector_Get(child_layer, i);
            if (child->tag == WINDOW) remaining_width += parent->gap;
            else remaining_width -= child->width;
        }

        float cursor_x = 0.0f;

        for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = Vector_Get(child_layer, i);
            float x_align_offset = remaining_width * 0.5f * (float)parent->horizontal_alignment;
            child->x = parent->x + parent->pad_left + parent->border_left + cursor_x + x_align_offset;
            cursor_x += child->width + parent->gap;
        }
    }
}

static void NU_Vertically_Place_Children(struct Node* parent, struct Vector* child_layer)
{
    if (!(parent->layout_flags & LAYOUT_VERTICAL))
    {   
        for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = Vector_Get(child_layer, i);
            float remaining_height = (parent->height - parent->pad_top - parent->pad_bottom - parent->border_top - parent->border_bottom) - child->height;
            float y_align_offset = remaining_height * 0.5f * (float)parent->vertical_alignment;
            child->y = parent->y + parent->pad_top + parent->border_top + y_align_offset;
        }
    }
    else
    {
        // Calculate remaining height (optimise this by caching this calue inside parent's content height variable)
        float remaining_height = (parent->height - parent->pad_top - parent->pad_bottom - parent->border_top - parent->border_bottom) - (parent->child_count - 1) * parent->gap - ((parent->layout_flags & OVERFLOW_HORIZONTAL_SCROLL) != 0) * 12.0f;
        for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = Vector_Get(child_layer, i);
            if (child->tag == WINDOW) remaining_height += parent->gap;
            else remaining_height -= child->height;
        }

        float cursor_y = 0.0f;

        for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = Vector_Get(child_layer, i);
            float y_align_offset = remaining_height * 0.5f * (float)parent->vertical_alignment;
            child->y = parent->y + parent->pad_top + parent->border_top + cursor_y + y_align_offset;
            cursor_y += child->height + parent->gap;
        }
    }
}

static void NU_Calculate_Positions(struct NU_GUI* ngui)
{
    for (int l=0; l<=ngui->deepest_layer; l++) // For each layer
    {
        struct Vector* parent_layer = &ngui->tree_stack[l];
        struct Vector* child_layer = &ngui->tree_stack[l+1];
        
        for (int p=0; p<parent_layer->size; p++) // For node in layer
        {   
            struct Node* parent = Vector_Get(parent_layer, p);

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

void NU_Mouse_Hover(struct NU_GUI* ngui)
{   
    // --------------------------------------
    // --- Clear ui tree hovered nodes vector
    // --------------------------------------
    ngui->hovered_nodes.size = 0;
    if (ngui->hovered_node != NULL) {
        NU_Apply_Stylesheet_To_Node(ngui->hovered_node, ngui->stylesheet);
        ngui->hovered_node = NULL;
    }

    if (ngui->hovered_window == NULL) return;

    // -------------------------------------------------
    // --- Create a traversal stack and append root node
    // -------------------------------------------------
    struct Vector stack;
    Vector_Reserve(&stack, sizeof(struct Node*), 32);
    
    // ----------------------------------------------------
    // --- Find the window node that mouse is hovering over
    // ----------------------------------------------------
    for (int i=0; i<ngui->window_nodes.size; i++)
    {
        struct Node* window_node = *(struct Node**) Vector_Get(&ngui->window_nodes, i);
        if (window_node->window == ngui->hovered_window) {
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
    SDL_GetWindowPosition(ngui->hovered_window, &window_x, &window_y);
    rel_x = mouse_x - window_x;
    rel_y = mouse_y - window_y;
    bool break_loop = false;
    while (stack.size > 0 && !break_loop) 
    {
        // Pop the stack
        struct Node* current_node = *(struct Node**)Vector_Get(&stack, stack.size - 1);
        if (current_node->window != ngui->hovered_window) break;
        stack.size -= 1;

        // ------------------------------
        // --- Iterate over node children
        // ------------------------------
        int32_t current_layer = ((current_node->ID >> 24) & 0xFF);
        struct Vector* child_layer = &ngui->tree_stack[current_layer+1];
        for (int i=current_node->first_child_index; i<current_node->first_child_index + current_node->child_count; i++)
        {
            struct Node* child = Vector_Get(child_layer, i);

            // ---------------------------------
            // --- If child is not a window node
            // ---------------------------------
            if (child->tag != WINDOW && NU_Mouse_Over_Node_Bounds(child, rel_x, rel_y)) 
            {
                ngui->hovered_node = child;
                if (child->tag == BUTTON) {
                    break_loop = true;
                    break;
                }

                Vector_Push(&stack, &child);
                Vector_Push(&ngui->hovered_nodes, &child);
            }
        }
    }

    // Set currently hovered node
    if (ngui->hovered_node != NULL) {
        NU_Apply_Pseudo_Style_To_Node(ngui->hovered_node, ngui->stylesheet, PSEUDO_HOVER);
    } 
    Vector_Free(&stack);
}




// -----------------------------
// --- Rendering ---------------
// -----------------------------
void NU_Draw_Node(struct Node* node, NVGcontext* vg, float screen_width, float screen_height)
{
    float inner_width  = node->width - node->border_left - node->border_right - node->pad_left - node->pad_right;
    float inner_height = node->height - node->border_top - node->border_bottom - node->pad_top - node->pad_bottom;

    Draw_Varying_Rounded_Rect(
        node->x, 
        node->y, 
        node->width, 
        node->height, 
        node->border_top, 
        node->border_bottom,
        node->border_left,
        node->border_right,
        node->border_radius_tl,
        node->border_radius_tr,
        node->border_radius_bl,
        node->border_radius_br, 
        node->border_r, node->border_g, node->border_b,
        node->background_r, node->background_g, node->background_b,
        screen_width,
        screen_height
    );

    if (node->gl_image_handle) {
        Draw_Image(node->x + node->border_left + node->pad_left, node->y + node->border_top + node->pad_top, inner_width, inner_height, screen_width, screen_height, node->gl_image_handle);
    }
}

void NU_Draw_Node_Text(struct NU_GUI* ngui, struct Node* node, char* text, NVGcontext* vg)
{
    // Setup font
    int fontID = *(int*) Vector_Get(&ngui->font_registry, 0);
    nvgFontFaceId(vg, fontID);   
    nvgFontSize(vg, 18);

    // Text color and alignment
    nvgFillColor(vg, nvgRGB(node->text_r, node->text_g, node->text_b));
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

    float asc, desc, lh;
    nvgTextMetrics(ngui->vg_ctx, &asc, &desc, &lh);

    // Compute inner dimensions (content area)
    float inner_width  = node->width  - node->border_left - node->border_right - node->pad_left - node->pad_right;
    float inner_height = node->height - node->border_top  - node->border_bottom - node->pad_top - node->pad_bottom;

    // Top-left corner of the content area
    float textPosX = node->x + node->border_left + node->pad_left;
    float textPosY = node->y + node->border_top  + node->pad_top - desc * 0.5f;

    // Draw wrapped text inside inner_width
    nvgTextBox(vg, floorf(textPosX), floorf(textPosY), inner_width, text, NULL);
}

void NU_Draw_Nodes(struct NU_GUI* ngui)
{
    struct Vector window_nodes_list[MAX_TREE_DEPTH];
    for (uint32_t i=0; i<ngui->windows.size; i++) {
        Vector_Reserve(&window_nodes_list[i], sizeof(struct Node*), 1000);
    }

    // For each layer
    for (int l=0; l<=ngui->deepest_layer; l++)
    {
        struct Vector* layer = &ngui->tree_stack[l];
        for (int n=0; n<layer->size; n++)
        {   
            struct Node* node = Vector_Get(layer, n);
            for (uint32_t i=0; i<ngui->windows.size; i++)
            {
                SDL_Window* window = *(SDL_Window**) Vector_Get(&ngui->windows, i);
                if (window == node->window)
                {
                    Vector_Push(&window_nodes_list[i], &node);
                }
            }
        }
    }


    // For each window
    for (uint32_t i=0; i<ngui->windows.size; i++)
    {
        SDL_Window* window = *(SDL_Window**) Vector_Get(&ngui->windows, i);
        SDL_GL_MakeCurrent(window, ngui->gl_ctx);

        // Clear the window
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        nvgBeginFrame(ngui->vg_ctx, w, h, 1.0f);

        // For each node belonging to the window
        for (int n=0; n<window_nodes_list[i].size; n++)
        {
            struct Node* node = *(struct Node**) Vector_Get(&window_nodes_list[i], n);
            NU_Draw_Node(node, ngui->vg_ctx, (float)w, (float)h);

            if (node->text_ref_index != -1)
            {
                struct Text_Ref* text_ref = (struct Text_Ref*) Vector_Get(&ngui->text_arena.text_refs, node->text_ref_index);
                
                // Extract pointer to text
                char* text = ngui->text_arena.char_buffer.data + text_ref->buffer_index;
                if (text_ref->char_count != text_ref->char_capacity) text[text_ref->char_count] = '\0';
                NU_Draw_Node_Text(ngui, node, text, ngui->vg_ctx);
                if (text_ref->char_count != text_ref->char_capacity) text[text_ref->char_count] = ' ';
            }
        }

        // Render to window
        nvgEndFrame(ngui->vg_ctx);
        SDL_GL_SwapWindow(window); 
    }

    for (uint32_t i=0; i<ngui->windows.size; i++) {
        Vector_Free(&window_nodes_list[i]);
    }
}

void NU_Calculate(struct NU_GUI* ngui)
{
    NU_Clear_Node_Sizes(ngui);
    NU_Calculate_Text_Fit_Sizes(ngui);
    NU_Calculate_Fit_Size_Widths(ngui);
    NU_Grow_Shrink_Widths(ngui);
    NU_Calculate_Text_Wrap_Heights(ngui);
    NU_Calculate_Fit_Size_Heights(ngui);
    NU_Grow_Shrink_Heights(ngui);
    NU_Calculate_Positions(ngui);
    NU_Mouse_Hover(ngui);
}




// -----------------------------
// Event watcher ---------------
// -----------------------------
struct NU_Watcher_Data {
    struct NU_GUI* ngui;
};

bool ResizingEventWatcher(void* data, SDL_Event* event) 
{
    struct NU_Watcher_Data* wd = (struct NU_Watcher_Data*)data;

    switch (event->type) {
        case SDL_EVENT_WINDOW_RESIZED:
            timer_start();
            NU_Calculate(wd->ngui);
            NU_Draw_Nodes(wd->ngui);
            timer_stop();
            break;
        case SDL_EVENT_MOUSE_MOTION:
            Uint32 id = event->motion.windowID;
            wd->ngui->hovered_window = SDL_GetWindowFromID(id);
            NU_Calculate(wd->ngui);
            NU_Draw_Nodes(wd->ngui);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            wd->ngui->mouse_down_node = wd->ngui->hovered_node;
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (wd->ngui->mouse_down_node == wd->ngui->hovered_node) {
                printf("Node Clicked!");
            }
            break;
        default:
            break;
    }    
    return true;
}