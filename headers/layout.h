#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <stdbool.h>
#include <math.h>

#include <nu_draw.h>

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>
#include <freetype/freetype.h>

// UI layout ------------------------------------------------------------
static void NU_Create_New_Window(struct UI_Tree* ui_tree, struct Node* window_node, struct Vector* windows, struct Vector* gl_contexts, struct Vector* nano_vg_contexts)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);  
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_Window* new_window = SDL_CreateWindow("window", 500, 400, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext new_gl_context = SDL_GL_CreateContext(new_window);
    SDL_GL_MakeCurrent(new_window, new_gl_context);
    glEnable(GL_MULTISAMPLE);
    glewInit();
    NVGcontext* new_nano_vg_context = nvgCreateGL3(NVG_STENCIL_STROKES);
    struct Vector font_registry;
    Vector_Reserve(&font_registry, sizeof(int), 8);
    for (int i=0; i<ui_tree->font_resources.size; i++) {
        struct Font_Resource* font = Vector_Get(&ui_tree->font_resources, i);
        int fontID = nvgCreateFontMem(new_nano_vg_context, font->name, font->data, font->size, 0);
        Vector_Push(&font_registry, &fontID);
    }
    Vector_Push(windows, &new_window);
    Vector_Push(gl_contexts, &new_gl_context);
    Vector_Push(nano_vg_contexts, &new_nano_vg_context);
    Vector_Push(&ui_tree->font_registries, &font_registry);
    window_node->window = new_window;
    window_node->vg = new_nano_vg_context;
    NU_Draw_Init();
}

static void NU_Reset_Node_size(struct Node* node)
{
    node->x = 0.0f;
    node->y = 0.0f;
    if (node->preferred_width == 0.0f) node->width = node->border_left + node->border_right + node->pad_left + node->pad_right;
    else node->width = node->preferred_width;
    node->height = node->border_top + node->border_bottom + node->pad_top + node->pad_bottom;
}

static void NU_Clear_Node_Sizes(struct UI_Tree* ui_tree, struct Vector* windows, struct Vector* gl_contexts, struct Vector* nano_vg_contexts)
{
    // For each layer
    for (int l=0; l<=ui_tree->deepest_layer; l++)
    {
        struct Vector* parent_layer = &ui_tree->tree_stack[l];
        struct Vector* child_layer = &ui_tree->tree_stack[l+1];

        for (int p=0; p<parent_layer->size; p++)
        {       
            // Iterate over layer
            struct Node* parent = Vector_Get(parent_layer, p);

            // If parent is window node and has no SDL window assigned to it -> create a new window and renderer
            if (parent->tag == WINDOW && parent->window == NULL) {
                NU_Create_New_Window(ui_tree, parent, windows, gl_contexts, nano_vg_contexts);
            }

            if (parent->child_count == 0) continue; // Skip acummulating child sizes (no children)

            for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
            {
                struct Node* child = Vector_Get(child_layer, i);

                // Inherit window and renderer from parent
                if (child->tag != WINDOW && child->window == NULL)
                {
                    child->window = parent->window;
                    child->vg = parent->vg;
                }

                NU_Reset_Node_size(child);
            }
        }
    }
}

static void NU_Calculate_Text_Min_Width(struct UI_Tree* ui_tree, struct Node* node, struct Text_Ref* text_ref)
{
    char* text = ui_tree->text_arena.char_buffer.data + text_ref->buffer_index;
    int slice_start = 0;
    float max_word_width = 0.0f;
    for (int i=0; i<=text_ref->char_count; i++) {
        char c = (i < text_ref->char_count) ? text[i] : ' '; 
        if (c == ' ') {
            if (i > slice_start) {
                float bounds[4];
                nvgTextBounds(node->vg, 0, 0, text + slice_start, text + i, bounds); // measure slice
                float width = bounds[2] - bounds[0];
                if (width > max_word_width) max_word_width = width;
            }
            slice_start = i + 1; 
        }
    }
    if (max_word_width == 0.0f && text_ref->char_count > 0) { // If no spaces found, the whole text is one word
        float bounds[4];
        nvgTextBounds(node->vg, 0, 0, text, text + text_ref->char_count, bounds);
        max_word_width = bounds[2] - bounds[0];
    }
    float text_controlled_min_width = max_word_width + node->pad_left + node->pad_right + node->border_left + node->border_right;
    node->min_width = max(node->min_width, text_controlled_min_width);
}

static void NU_Calculate_Text_Fit_Size(struct UI_Tree* ui_tree, struct Node* node, struct Text_Ref* text_ref)
{
    // Extract pointer to text
    char* text = ui_tree->text_arena.char_buffer.data + text_ref->buffer_index;

    // Make sure the NanoVG context has the correct font/size set before measuring!
    struct Vector* font_registry = Vector_Get(&ui_tree->font_registries, 0);
    int fontID = *(int*) Vector_Get(font_registry, 0);
    nvgFontFaceId(node->vg, fontID);   
    nvgFontSize(node->vg, 18);

    // Calculate text bounds
    float asc, desc, lh;
    float bounds[4];
    nvgTextMetrics(node->vg, &asc, &desc, &lh);
    nvgTextBounds(node->vg, 0, 0, text, text + text_ref->char_count, bounds);
    float text_width = bounds[2] - bounds[0];
    float text_height = lh;
    
    NU_Calculate_Text_Min_Width(ui_tree, node, text_ref);
    
    if (node->preferred_width == 0.0f) {
        node->width = text_width + node->pad_left + node->pad_right + node->border_left + node->border_right;
    }
    node->width = min(node->width, node->max_width);
    node->width = max(node->width, node->min_width);
    node->height += text_height;
}

static void NU_Calculate_Text_Fit_Sizes(struct UI_Tree* ui_tree)
{
    for (uint32_t i=0; i<ui_tree->text_arena.text_refs.size; i++)
    {
        struct Text_Ref* text_ref = (struct Text_Ref*) Vector_Get(&ui_tree->text_arena.text_refs, i);
        
        // Get the corresponding node
        uint8_t node_depth = (uint8_t)(text_ref->node_ID >> 24);
        uint32_t node_index = text_ref->node_ID & 0x00FFFFFF;
        struct Vector* layer = &ui_tree->tree_stack[node_depth];
        struct Node* node = Vector_Get(layer, node_index);

        // Calculate text size
        NU_Calculate_Text_Fit_Size(ui_tree, node, text_ref);
    }
}

static void NU_Calculate_Fit_Size_Widths(struct UI_Tree* ui_tree)
{
    if (ui_tree->deepest_layer == 0) return;

    // For each layer
    for (int l=ui_tree->deepest_layer; l>=0; l--)
    {
        struct Vector* parent_layer = &ui_tree->tree_stack[l];
        struct Vector* child_layer = &ui_tree->tree_stack[l+1];
        
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

                // Dont' accumulate if parent is a window
                if (is_layout_horizontal) { // Horizontal Layout
                    content_width += child->width;
                }

                if (!is_layout_horizontal) { // Vertical Layout
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

static void NU_Calculate_Fit_Size_Heights(struct UI_Tree* ui_tree)
{
    if (ui_tree->deepest_layer == 0) return;

    // For each layer
    for (int l=ui_tree->deepest_layer; l>=0; l--)
    {
        struct Vector* parent_layer = &ui_tree->tree_stack[l];
        struct Vector* child_layer = &ui_tree->tree_stack[l+1];
        
        // Iterate over layer
        for (int p=0; p<parent_layer->size; p++)
        {   
            struct Node* parent = Vector_Get(parent_layer, p);
            int is_layout_horizontal = (parent->layout_flags & 0x01) == LAYOUT_HORIZONTAL;

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

                // Dont' accumulate if parent is a window
                if (is_layout_horizontal) { // Horizontal Layout
                    content_height = MAX(content_height, child->height);
                }

                if (!is_layout_horizontal) { // Vertical Layout
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
                child->width = remaining_width; 
                child->width = min(child->width, child->max_width);
                child->width = max(child->width, child->min_width);
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
                child->height = remaining_height; 
                child->height = min(child->height, child->max_height);
                child->height = max(child->height, child->min_height);
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

        while (remaining_height > 0.001f)
        {
            // Find smallest and second smallest
            float smallest = 1e20f;
            for (int i = parent->first_child_index; i < parent->first_child_index + parent->child_count; i++) {
                struct Node* child = Vector_Get(child_layer, i);
                if ((child->layout_flags & GROW_VERTICAL) && child->tag != WINDOW) {
                    smallest = child->height;
                    break;
                }
            }
            float second_smallest = 1e200;
            float height_to_add = remaining_height;
            for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++) {
                struct Node* child = Vector_Get(child_layer, i);
                if (child->layout_flags & GROW_VERTICAL && child->tag != WINDOW) {
                    if (child->height < smallest) {
                        second_smallest = smallest;
                        smallest = child->height;
                    }
                    else if (child->height > smallest) {
                        second_smallest = min(child->height, second_smallest);
                        height_to_add = second_smallest - smallest;
                    }
                }
            }
            height_to_add = min(height_to_add, remaining_height / growable_count);

            // for each child
            bool grew_any = false;
            for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++) {
                struct Node* child = Vector_Get(child_layer, i);
                if (child->layout_flags & GROW_VERTICAL && child->tag != WINDOW) { // if child is growable
                    if (child->height == smallest) {
                        child->height += height_to_add;
                        remaining_height -= height_to_add;
                        grew_any = true;
                    }
                }
            }
            if (!grew_any) break;
        }
    }
}

static void NU_Grow_Shrink_Widths(struct UI_Tree* ui_tree)
{
    for (int l=0; l<=ui_tree->deepest_layer; l++) // For each layer
    {
        struct Vector* parent_layer = &ui_tree->tree_stack[l];
        struct Vector* child_layer = &ui_tree->tree_stack[l+1];
        
        for (int p=0; p<parent_layer->size; p++) // For node in layer
        {   
            struct Node* parent = Vector_Get(parent_layer, p);
            NU_Grow_Shrink_Child_Node_Widths(parent, child_layer);
        }
    }
}

static void NU_Grow_Shrink_Heights(struct UI_Tree* ui_tree)
{
    for (int l=0; l<=ui_tree->deepest_layer; l++) // For each layer
    {
        struct Vector* parent_layer = &ui_tree->tree_stack[l];
        struct Vector* child_layer = &ui_tree->tree_stack[l+1];
        
        for (int p=0; p<parent_layer->size; p++) // For node in layer
        {   
            struct Node* parent = Vector_Get(parent_layer, p);
            NU_Grow_Shrink_Child_Node_Heights(parent, child_layer);
        }
    }
}

static void NU_Calculate_Text_Wrap_Heights(struct UI_Tree* ui_tree, struct Vector* windows, struct Vector* gl_contexts, struct Vector* nano_vg_contexts)
{
    for (uint32_t i = 0; i < ui_tree->text_arena.text_refs.size; i++) {
        struct Text_Ref* text_ref = (struct Text_Ref*) Vector_Get(&ui_tree->text_arena.text_refs, i);
        
        // Get the corresponding node
        uint8_t node_depth = (uint8_t)(text_ref->node_ID >> 24);
        uint32_t node_index = text_ref->node_ID & 0x00FFFFFF;
        struct Vector* layer = &ui_tree->tree_stack[node_depth];
        struct Node* node = Vector_Get(layer, node_index);

        char* text = ui_tree->text_arena.char_buffer.data + text_ref->buffer_index;

        // Configure font
        struct Vector* font_registry = Vector_Get(&ui_tree->font_registries, 0);
        int fontID = *(int*) Vector_Get(font_registry, 0);
        nvgFontFaceId(node->vg, fontID);   
        nvgFontSize(node->vg, 18);

        // Compute available inner width
        float inner_width = node->width -
                            node->border_left - node->border_right -
                            node->pad_left - node->pad_right;

        // Measure text the same way it's drawn
        float bounds[4];
        nvgTextBoxBounds(node->vg, 0, 0, inner_width, text, NULL, bounds);
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
            child->x = child->border_left + parent->x + parent->pad_left + x_align_offset;
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
            child->x = child->border_left + parent->x + parent->pad_left + cursor_x + x_align_offset;
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
            child->y = child->border_top + parent->y + parent->pad_top + y_align_offset;
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
            else remaining_height -= child->width;
        }

        float cursor_y = 0.0f;

        for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
        {
            struct Node* child = Vector_Get(child_layer, i);
            float y_align_offset = remaining_height * 0.5f * (float)parent->vertical_alignment;
            child->y = child->border_top + parent->y + parent->pad_top + cursor_y + y_align_offset;
            cursor_y += child->height + parent->gap;
        }
    }
}

static void NU_Calculate_Positions(struct UI_Tree* ui_tree)
{
    for (int l=0; l<=ui_tree->deepest_layer; l++) // For each layer
    {
        struct Vector* parent_layer = &ui_tree->tree_stack[l];
        struct Vector* child_layer = &ui_tree->tree_stack[l+1];
        
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
// UI layout ------------------------------------------------------------



// UI rendering ---------------------------------------------------------
void NU_Draw_Node(struct Node* node, NVGcontext* vg, float screen_width, float screen_height)
{
    float inner_width  = node->width - node->border_left - node->border_right - node->pad_left - node->pad_right;
    float inner_height = node->height - node->border_top - node->border_bottom - node->pad_top - node->pad_bottom;

    // Fill color based on tag
    NVGcolor fillColor;
    switch(node->tag)
    {
        case RECT:   fillColor = nvgRGB(120, 100, 100); break;
        case BUTTON: fillColor = nvgRGB(50, 50, 50); break;
        case WINDOW: fillColor = nvgRGB(60, 30, 255); break;
        default:     fillColor = nvgRGB(100, 150, 120); break;
    }

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
}

void NU_Draw_Node_Text(struct UI_Tree* ui_tree, struct Node* node, char* text, NVGcontext* vg)
{
    // Setup font
    struct Vector* font_registry = Vector_Get(&ui_tree->font_registries, 0);
    int fontID = *(int*) Vector_Get(font_registry, 0);
    nvgFontFaceId(vg, fontID);   
    nvgFontSize(vg, 18);

    // Text color and alignment
    nvgFillColor(vg, nvgRGB(255, 255, 255));
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

    float asc, desc, lh;
    nvgTextMetrics(node->vg, &asc, &desc, &lh);

    // Compute inner dimensions (content area)
    float inner_width  = node->width  - node->border_left - node->border_right - node->pad_left - node->pad_right;
    float inner_height = node->height - node->border_top  - node->border_bottom - node->pad_top - node->pad_bottom;

    // Top-left corner of the content area
    float textPosX = node->x + node->border_left + node->pad_left;
    float textPosY = node->y + node->border_top  + node->pad_top - desc * 0.5f;

    // Draw wrapped text inside inner_width
    nvgTextBox(vg, floorf(textPosX), floorf(textPosY), inner_width, text, NULL);
}

void NU_Draw_Nodes(struct UI_Tree* ui_tree, struct Vector* windows, struct Vector* gl_contexts, struct Vector* nano_vg_contexts)
{
    struct Vector window_nodes_list[MAX_TREE_DEPTH];
    for (int i=0; i<windows->size; i++) {
        Vector_Reserve(&window_nodes_list[i], sizeof(struct Node*), 1000);
    }

    // For each layer
    for (int l=0; l<=ui_tree->deepest_layer; l++)
    {
        struct Vector* layer = &ui_tree->tree_stack[l];
        for (int n=0; n<layer->size; n++)
        {   
            struct Node* node = Vector_Get(layer, n);
            for (int i=0; i<windows->size; i++)
            {
                SDL_Window* window = *(SDL_Window**) Vector_Get(windows, i);
                if (window == node->window)
                {
                    Vector_Push(&window_nodes_list[i], &node);
                }
            }
        }
    }


    // For each window
    for (int i=0; i<windows->size; i++)
    {
        SDL_Window* window = *(SDL_Window**) Vector_Get(windows, i);
        SDL_GLContext gl_context = *(SDL_GLContext*) Vector_Get(gl_contexts, i);
        NVGcontext* nano_vg_context = *(NVGcontext**) Vector_Get(nano_vg_contexts, i);
        SDL_GL_MakeCurrent(window, gl_context);

        // Clear the window
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        nvgBeginFrame(nano_vg_context, w, h, 1.0f);

        // For each node belonging to the window
        for (int n=0; n<window_nodes_list[i].size; n++)
        {
            struct Node* node = *(struct Node**) Vector_Get(&window_nodes_list[i], n);

            NU_Draw_Node(node, nano_vg_context, (float)w, (float)h);

            if (node->text_ref_index != -1)
            {
                struct Text_Ref* text_ref = (struct Text_Ref*) Vector_Get(&ui_tree->text_arena.text_refs, node->text_ref_index);
                
                // Extract pointer to text
                char* text = ui_tree->text_arena.char_buffer.data + text_ref->buffer_index;
                if (text_ref->char_count != text_ref->char_capacity) text[text_ref->char_count] = '\0';
                NU_Draw_Node_Text(ui_tree, node, text, nano_vg_context);
                if (text_ref->char_count != text_ref->char_capacity) text[text_ref->char_count] = ' ';
            }
        }

        // timer_start();
        // for (int i=0; i<100000; i++)
        // {
        //     Draw_Varying_Rounded_Rect(100, 100, 500, 200, 10, 10, 10, 10, 20, 20, 20, 20, (char)255, (char)100, (char)100, (float)w, (float)h);
        // }
        // timer_stop();

        // Render to window
        nvgEndFrame(nano_vg_context);
        SDL_GL_SwapWindow(window); 
    }

    for (int i=0; i<windows->size; i++) {
        Vector_Free(&window_nodes_list[i]);
    }
}

void NU_Render(struct UI_Tree* ui_tree, struct Vector* windows, struct Vector* gl_contexts, struct Vector* nano_vg_contexts)
{
    NU_Clear_Node_Sizes(ui_tree, windows, gl_contexts, nano_vg_contexts);
    NU_Calculate_Text_Fit_Sizes(ui_tree);
    NU_Calculate_Fit_Size_Widths(ui_tree);
    NU_Grow_Shrink_Widths(ui_tree);
    NU_Calculate_Text_Wrap_Heights(ui_tree, windows, gl_contexts, nano_vg_contexts);
    NU_Calculate_Fit_Size_Heights(ui_tree);
    NU_Grow_Shrink_Heights(ui_tree);
    NU_Calculate_Positions(ui_tree);
    NU_Draw_Nodes(ui_tree, windows, gl_contexts, nano_vg_contexts);
}
// UI rendering ---------------------------------------------------------



// Window resize event handling -----------------------------------------
struct NU_Watcher_Data {
    struct UI_Tree* ui_tree;
    struct Vector* windows;
    struct Vector* gl_contexts;
    struct Vector* nano_vg_contexts;
};

bool ResizingEventWatcher(void* data, SDL_Event* event) 
{
    struct NU_Watcher_Data* wd = (struct NU_Watcher_Data*)data;

    if (event->type == SDL_EVENT_WINDOW_RESIZED) 
    {
        NU_Render(wd->ui_tree, wd->windows, wd->gl_contexts, wd->nano_vg_contexts);
    }
    return true;
}
// Window resize event handling -----------------------------------------