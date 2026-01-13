#pragma once
#include <SDL3/SDL.h>
#include <GL/glew.h>

void NU_Add_Text_Mesh(Vertex_RGB_UV_List* vertices, Index_List* indices, Node* node)
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

static bool Is_Node_Visible_In_Bounds(Node* node, float x, float y, float w, float h)
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

static int NodeVerticalOverlapState(Node* node, float y, float h)
{
    float top    = node->y;
    float bottom = top + node->height;
    float bounds_top    = y;
    float bounds_bottom = y + h;
    if (bottom <= bounds_top || top >= bounds_bottom) return 0; // no overlap
    if (top >= bounds_top && bottom <= bounds_bottom) return 2; // fully inside
    return 1; // partial overlap
}

void NU_GenerateDrawlists()
{
    // prepare draw datastructures
    PrepareDrawlists(&__NGUI.winManager);

    // add root to drawlist 
    Node* root = &__NGUI.tree.layers[0].node_array[0];
    SetNodeDrawlist_Relative(&__NGUI.winManager, root);

    // traverse the tree
    for (int l=0; l<=__NGUI.deepest_layer; l++)
    {
        NU_Layer* parentlayer = &__NGUI.tree.layers[l];
        NU_Layer* childlayer = &__NGUI.tree.layers[l+1];

        // iterate over parent layer
        for (int p=0; p<parentlayer->size; p++) 
        {       
            Node* parent = NU_Layer_Get(parentlayer, p);
            if (parent->nodeState == 0) continue;
            if (parent->nodeState == 2) { // Parent not visible -> children must inherit this
                for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
                    Node* child = NU_Layer_Get(childlayer, i); child->nodeState = 2;
                }
                continue;
            }

            // precompute parent inner rect
            float parent_inner_x, parent_inner_y, parent_inner_width, parent_inner_height = 0;
            if (parent->layoutFlags & OVERFLOW_VERTICAL_SCROLL) {
                parent_inner_y = parent->y + parent->borderTop + parent->padTop;
                parent_inner_height = parent->height - parent->borderTop - parent->borderBottom - parent->padTop - parent->padBottom;

                // Handle case where parent is a table with a thead
                if (parent->tag == TABLE && parent->childCount > 0) {
                    Node* first_child = NU_Layer_Get(childlayer, parent->firstChildIndex);
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

            // iterate over children
            for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
            {
                Node* child = NU_Layer_Get(childlayer, i); if (child->nodeState == 2) continue;

                // if child not visible in window bounds -> mark as hidden
                if (!NodeVisibleInWindow(&__NGUI.winManager, child)) {
                    child->nodeState = 2; 
                    continue;
                }

                // add child to list of root absolute nodes
                if (child->layoutFlags & POSITION_ABSOLUTE) {
                    Vector_Push(&__NGUI.winManager.absoluteRootNodes, &child);
                }

                // skip child if overflowed parent's bounds
                if (parent->layoutFlags & OVERFLOW_VERTICAL_SCROLL && child->tag != THEAD) 
                {   
                    int intersect_state = NodeVerticalOverlapState(child, parent_inner_y, parent_inner_height);

                    // child not inside parent -> hide in this draw pass
                    if (intersect_state == 0) { child->nodeState = 2; continue; }

                    // child overlaps parent boundary
                    else if (intersect_state == 1) 
                    {
                        // determine clipping
                        NU_ClipBounds clip;
                        clip.clip_top = fmaxf(child->y - 1, parent_inner_y);
                        clip.clip_bottom = fminf(child->y + child->height, parent_inner_y + parent_inner_height);
                        clip.clip_left = -1.0f;
                        clip.clip_right = 1000000.0f;

                        // position absolute -> secondary drawing list
                        if (child->positionAbsolute)
                        {
                            // Iif parent is also clipped -> merge clips (stack clipping behaviour)
                            if (parent->clippingRootHandle != UINT32_MAX) {
                                NU_ClipBounds* parent_clip = HashmapGet(&__NGUI.winManager.clipMap, &parent->clippingRootHandle);
                                clip.clip_top = fmaxf(clip.clip_top, parent_clip->clip_top);
                                clip.clip_bottom = fminf(clip.clip_bottom, parent_clip->clip_bottom);
                            }
                            
                            // add clipping to hashmap
                            HashmapSet(&__NGUI.winManager.clipMap, &child->handle, &clip);
                            child->clippingRootHandle = child->handle; // Set clip root to self

                            // append node to correct window clipped node list
                            SetNodeDrawlist_ClippedAbsolute(&__NGUI.winManager, child);
                            continue;
                        }
                        // position relative -> main drawing list
                        else
                        {
                            // if parent is also clipped -> merge clips (stack clipping behaviour)
                            if (parent->clippingRootHandle != UINT32_MAX) {
                                NU_ClipBounds* parent_clip = HashmapGet(&__NGUI.winManager.clipMap, &parent->clippingRootHandle);
                                clip.clip_top = fmaxf(clip.clip_top, parent_clip->clip_top);
                                clip.clip_bottom = fminf(clip.clip_bottom, parent_clip->clip_bottom);
                            }
                            
                            // add clipping to hashmap
                            HashmapSet(&__NGUI.winManager.clipMap, &child->handle, &clip);
                            child->clippingRootHandle = child->handle; // Set clip root to self

                            // append node to correct window clipped node list
                            SetNodeDrawlist_ClippedRelative(&__NGUI.winManager, child);
                            continue;
                        }
                    }
                }

                // if parent is clipped -> child inherits clip from parent
                if (parent->clippingRootHandle != UINT32_MAX) {
                    child->clippingRootHandle = parent->clippingRootHandle;

                    // position absolute -> secondary drawing list
                    if (child->positionAbsolute) {
                        SetNodeDrawlist_ClippedAbsolute(&__NGUI.winManager, child);
                    }
                    // position relative -> main drawing list
                    else {
                        SetNodeDrawlist_ClippedRelative(&__NGUI.winManager, child);
                    }
                    continue;
                }

                // neither child nor parent is clipped -> append node to correct window node list
                if (child->positionAbsolute) { // Position Absolute -> secondary drawing list
                    SetNodeDrawlist_Absolute(&__NGUI.winManager, child);
                }
                else { // position Relative -> main drawing list
                    SetNodeDrawlist_Relative(&__NGUI.winManager, child);
                }

                // append node to canvas API drawing list
                if (child->tag == CANVAS) {
                    SetNodeDrawlist_Canvas(&__NGUI.winManager, child);
                }
            }
        }
    }
}

void NU_Draw()
{
    NU_GenerateDrawlists();

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
    for (uint32_t i=0; i<__NGUI.winManager.windows.size; i++)
    {
        // ---------------------------------------------
        // --- Get the window and dimensions, clear and start new frame
        // ---------------------------------------------
        SDL_Window* window = *(SDL_Window**) Vector_Get(&__NGUI.winManager.windows, i);
        SDL_GL_MakeCurrent(window, __NGUI.gl_ctx);
        WindowStartNewFrame(window);
        NU_WindowDrawlist* drawList = Vector_Get(&__NGUI.winManager.windowDrawLists, i);
        float winW, winH; GetWindowSize(window, &winW, &winH);
        

        // --------------------------------------------------------------------------------------------------------
        // --- Draw Relative Positioned Nodes [First Draw Pass] ===================================================
        // --------------------------------------------------------------------------------------------------------

        // --- Draw Border Rects ---
        Vertex_RGB_List border_rect_vertices;              Index_List border_rect_indices;
        Vertex_RGB_List_Init(&border_rect_vertices, 5000); Index_List_Init(&border_rect_indices, 15000);
        for (uint32_t n=0; n<drawList->relativeNodes.size; n++) { // Construct border rect vertices and indices for each node
            Node* node = *(Node**)Vector_Get(&drawList->relativeNodes, n);
            Construct_Border_Rect(node, winW, winH, &border_rect_vertices, &border_rect_indices);
            if (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL 
                && node->contentHeight > (node->height - node->padTop - node->padBottom - node->borderTop - node->borderBottom)) {
                Construct_Scroll_Thumb(node, winW, winH, &border_rect_vertices, &border_rect_indices);
            }
        }
        Draw_Vertex_RGB_List(&border_rect_vertices, &border_rect_indices, winW, winH, 0.0f, 0.0f); // Draw border rects in one call
        Vertex_RGB_List_Free(&border_rect_vertices);
        Index_List_Free(&border_rect_indices);


        // --- Draw Canvas API Content ---
        for (uint32_t n=0; n<drawList->canvasNodes.size; n++)
        {
            Node* canvas_node = *(Node**)Vector_Get(&drawList->canvasNodes, n);
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
                winW, winH, 
                offset_x, offset_y,
                clip_top, clip_bottom, clip_left, clip_right 
            );
        }


        // --- Construct Text Meshes & Draw Images ---
        for (uint32_t n=0; n<drawList->relativeNodes.size; n++) 
        {

            // Construct text mesh for node
            Node* node = *(Node**)Vector_Get(&drawList->relativeNodes, n);
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
                    winW, winH, 
                    -1.0f, 100000.0f, -1.0f, 100000.0f,
                    node->glImageHandle);
            }
        }
        for (uint32_t t=0; t<__NGUI.stylesheet->fonts.size; t++) // Draw text
        {
            Vertex_RGB_UV_List* text_vertices = &text_relative_vertex_buffers[t];
            Index_List* text_indices = &text_relative_index_buffers[t];
            NU_Font* node_font = Vector_Get(&__NGUI.stylesheet->fonts, t);
            NU_Render_Text(text_vertices, text_indices, node_font, winW, winH, -1.0f, 100000.0f, -1.0f, 100000.0f);
            text_vertices->size = 0;
            text_indices->size = 0;
        }


        // --- Draw Partially Visible Nodes ---
        for (uint32_t n=0; n<drawList->clippedRelativeNodes.size; n++) {
            Node* node = *(Node**)Vector_Get(&drawList->clippedRelativeNodes, n);
            NU_ClipBounds* clip = (NU_ClipBounds*)HashmapGet(&__NGUI.winManager.clipMap, &node->clippingRootHandle);

            // Draw border rects
            Vertex_RGB_List vertices;
            Index_List indices;
            Vertex_RGB_List_Init(&vertices, 100);
            Index_List_Init(&indices, 100);
            Construct_Border_Rect(node, winW, winH, &vertices, &indices);
            Draw_Clipped_Vertex_RGB_List(&vertices, &indices, winW, winH, 0.0f, 0.0f, clip->clip_top, clip->clip_bottom, clip->clip_left, clip->clip_right);
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
                NU_Render_Text(&clipped_text_vertices, &clipped_text_indices, node_font, winW, winH, clip->clip_top, clip->clip_bottom, clip->clip_left, clip->clip_right);
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
                    winW, winH, 
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
        for (uint32_t n=0; n<drawList->absoluteNodes.size; n++) { // Construct border rect vertices and indices for each node
            Node* node = *(Node**)Vector_Get(&drawList->absoluteNodes, n);
            Construct_Border_Rect(node, winW, winH, &border_rect_vertices, &border_rect_indices);
            if (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL 
                && node->contentHeight > (node->height - node->padTop - node->padBottom - node->borderTop - node->borderBottom)) {
                Construct_Scroll_Thumb(node, winW, winH, &border_rect_vertices, &border_rect_indices);
            }
        }
        Draw_Vertex_RGB_List(&border_rect_vertices, &border_rect_indices, winW, winH, 0.0f, 0.0f); // Draw border rects in one call
        Vertex_RGB_List_Free(&border_rect_vertices);
        Index_List_Free(&border_rect_indices);


        // --- Construct Text Meshes & Draw Images ---
        for (uint32_t n=0; n<drawList->absoluteNodes.size; n++) 
        {

            // Construct text mesh for node
            Node* node = *(Node**)Vector_Get(&drawList->absoluteNodes, n);
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
                    winW, winH, 
                    -1.0f, 100000.0f, -1.0f, 100000.0f,
                    node->glImageHandle);
            }
        }
        for (uint32_t t=0; t<__NGUI.stylesheet->fonts.size; t++) // Draw text
        {
            Vertex_RGB_UV_List* text_vertices = &text_absolute_vertex_buffers[t];
            Index_List* text_indices = &text_absolute_index_buffers[t];
            NU_Font* node_font = Vector_Get(&__NGUI.stylesheet->fonts, t);
            NU_Render_Text(text_vertices, text_indices, node_font, winW, winH, -1.0f, 100000.0f, -1.0f, 100000.0f);
            text_vertices->size = 0;
            text_indices->size = 0;
        }

        // --- Draw Partially Visible Nodes ---
        for (uint32_t n=0; n<drawList->clippedAbsoluteNodes.size; n++) {
            Node* node = *(Node**) Vector_Get(&drawList->clippedAbsoluteNodes, n);
            NU_ClipBounds* clip = (NU_ClipBounds*)HashmapGet(&__NGUI.winManager.clipMap, &node->clippingRootHandle);

            // Draw border rects
            Vertex_RGB_List vertices;
            Index_List indices;
            Vertex_RGB_List_Init(&vertices, 100);
            Index_List_Init(&indices, 100);
            Construct_Border_Rect(node, winW, winH, &vertices, &indices);
            Draw_Clipped_Vertex_RGB_List(&vertices, &indices, winW, winH, 0.0f, 0.0f, clip->clip_top, clip->clip_bottom, clip->clip_left, clip->clip_right);
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
                NU_Render_Text(&clipped_text_vertices, &clipped_text_indices, node_font, winW, winH, clip->clip_top, clip->clip_bottom, clip->clip_left, clip->clip_right);
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
                    winW, winH, 
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
    for (uint32_t i=0; i<__NGUI.stylesheet->fonts.size; i++) {
        Vertex_RGB_UV_List_Free(&text_relative_vertex_buffers[i]);
        Index_List_Free(&text_relative_index_buffers[i]);
        Vertex_RGB_UV_List_Free(&text_absolute_vertex_buffers[i]);
        Index_List_Free(&text_absolute_index_buffers[i]);
    }

    __NGUI.awaiting_redraw = false;
}