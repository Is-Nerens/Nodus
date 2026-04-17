#pragma once
#include <SDL3/SDL.h>
#include <GL/glew.h>

void NU_AddTextMesh(Vertex_RGB_UV_List* vertices, Index_List* indices, NodeP* node, char* textBuffer)
{
    // Compute inner dimensions (content area)
    float inner_width  = node->node.width  - node->node.borderLeft - node->node.borderRight - node->node.padLeft - node->node.padRight;
    float inner_height = node->node.height - node->node.borderTop  - node->node.borderBottom - node->node.padTop - node->node.padBottom;
    float remaining_w = inner_width  - node->node.contentWidth;
    float remaining_h = inner_height - node->node.contentHeight;
    float x_align_offset = remaining_w * 0.5f * (float)node->horizontalTextAlignment;
    float y_align_offset = remaining_h * 0.5f * (float)node->verticalTextAlignment;

    // Top-left corner of the content area
    float textPosX = node->node.x + node->node.borderLeft + node->node.padLeft + x_align_offset;
    float textPosY = node->node.y + node->node.borderTop  + node->node.padTop + y_align_offset;

    // Draw wrapped text inside inner_width
    float r = (float)node->node.textR / 255.0f;
    float g = (float)node->node.textG / 255.0f;
    float b = (float)node->node.textB / 255.0f;
    NU_Font* node_font = Stylesheet_Get_Font(GUI.stylesheet, node->fontId);
    NU_Generate_Text_Mesh(vertices, indices, node_font, textBuffer, floorf(textPosX), floorf(textPosY), r, g, b, inner_width);
}

static int NodeVerticalOverlapState(NodeP* node, float y, float h)
{
    float top    = node->node.y;
    float bottom = top + node->node.height;
    float bounds_top    = y;
    float bounds_bottom = y + h;
    if (bottom <= bounds_top || top >= bounds_bottom) return 0; // no overlap
    if (top >= bounds_top && bottom <= bounds_bottom) return 2; // fully inside
    return 1; // partial overlap
}

void NU_DrawNodeImage(NodeP* node, float winWidth, float winHeight)
{
    float inner_width  = node->node.width - node->node.borderLeft - node->node.borderRight - node->node.padLeft - node->node.padRight;
    float inner_height = node->node.height - node->node.borderTop - node->node.borderBottom - node->node.padTop - node->node.padBottom;
    float x = node->node.x + node->node.borderLeft + (float)node->node.padLeft;
    float y = node->node.y + node->node.borderTop + (float)node->node.padTop;
    NU_Draw_Image(
        x, y, 
        inner_width, inner_height, 
        winWidth, winHeight, 
        -1.0f, 100000.0f, -1.0f, 100000.0f,
        node->typeData.image.glImageHandle);
}

void NU_DrawClippedNodeImage(NodeP* node, float winWidth, float winHeight, NU_ClipBounds* clip)
{
    float inner_width  = node->node.width - node->node.borderLeft - node->node.borderRight - node->node.padLeft - node->node.padRight;
    float inner_height = node->node.height - node->node.borderTop - node->node.borderBottom - node->node.padTop - node->node.padBottom;
    float x = node->node.x + node->node.borderLeft + (float)node->node.padLeft;
    float y = node->node.y + node->node.borderTop + (float)node->node.padTop;
    NU_Draw_Image(
        x, y, 
        inner_width, inner_height, 
        winWidth, winHeight, 
        clip->clip_top, clip->clip_bottom, clip->clip_left, clip->clip_right,
        node->typeData.image.glImageHandle);
}

void NU_DrawClippedNodeTextContent(NodeP* node, float winWidth, float winHeight, NU_ClipBounds* clip)
{
    Vertex_RGB_UV_List clipped_text_vertices;
    Index_List clipped_text_indices;
    Vertex_RGB_UV_List_Init(&clipped_text_vertices, 1000);
    Index_List_Init(&clipped_text_indices, 600);
    NU_Font* node_font = Stylesheet_Get_Font(GUI.stylesheet, node->fontId);
    NU_AddTextMesh(&clipped_text_vertices, &clipped_text_indices, node, node->node.textContent);
    NU_Render_Text(&clipped_text_vertices, &clipped_text_indices, node_font, winWidth, winHeight, 0, 0, clip->clip_top, clip->clip_bottom, clip->clip_left, clip->clip_right);
    Vertex_RGB_UV_List_Free(&clipped_text_vertices);
    Index_List_Free(&clipped_text_indices);
}

void NU_DrawInputNodeContent(NodeP* node, float winWidth, float winHeight, NU_ClipBounds* clip)
{   
    NU_Font* node_font = Stylesheet_Get_Font(GUI.stylesheet, node->fontId);
    InputText* inputText = &node->typeData.input.inputText;

    if (inputText->updateOffsetsPostLayout) {
        inputText->updateOffsetsPostLayout = false;
        InputText_ComputeCursorTextOffset_PlaceEnd(inputText, node, node_font);
    }

    // construct and draw highlight mesh
    if (GUI.focused_node != NULL && node == GUI.focused_node && InputText_IsHighlighting(inputText)) 
    {
        Vertex_RGB_List highlightVertices; Vertex_RGB_List_Init(&highlightVertices, 4);
        Index_List highlightIndices; Index_List_Init(&highlightIndices, 6);
        NU_ConstructInputHighlightMesh(node, &highlightVertices, &highlightIndices);
        Draw_Clipped_Vertex_RGB_List(
            &highlightVertices, &highlightIndices,
            winWidth, winHeight, 
            0, 0,
            clip->clip_top, clip->clip_bottom, 
            clip->clip_left, clip->clip_right + 1
        );
        Vertex_RGB_List_Free(&highlightVertices);
        Index_List_Free(&highlightIndices);
    }

    // generate and draw text
    Vertex_RGB_UV_List clipped_text_vertices; Vertex_RGB_UV_List_Init(&clipped_text_vertices, 1000);
    Index_List clipped_text_indices; Index_List_Init(&clipped_text_indices, 600);
    float textPosX = node->node.x + node->node.borderLeft + node->node.padLeft + node->typeData.input.inputText.textOffset;
    float textPosY = node->node.y + node->node.borderTop  + node->node.padTop;
    float r = (float)node->node.textR / 255.0f;
    float g = (float)node->node.textG / 255.0f;
    float b = (float)node->node.textB / 255.0f;
    NU_Generate_Text_Mesh(&clipped_text_vertices, &clipped_text_indices, node_font, inputText->buffer, floorf(textPosX), floorf(textPosY), r, g, b, 10000000.0f);
    NU_Render_Text(&clipped_text_vertices, &clipped_text_indices, node_font, winWidth, winHeight, 0, 0, clip->clip_top, clip->clip_bottom, clip->clip_left, clip->clip_right);
    Vertex_RGB_UV_List_Free(&clipped_text_vertices);
    Index_List_Free(&clipped_text_indices);

    // draw cursor afterwards (if input is focused)
    if (GUI.focused_node != NULL && node == GUI.focused_node
        && !InputText_IsHighlighting(inputText)) 
    {

        Vertex_RGB_List cursorVertices; Vertex_RGB_List_Init(&cursorVertices, 4);
        Index_List cursorIndices; Index_List_Init(&cursorIndices, 6);
        NU_ConstructInputCursorMesh(node, &cursorVertices, &cursorIndices);
        Draw_Clipped_Vertex_RGB_List(
            &cursorVertices, &cursorIndices,
            winWidth, winHeight, 
            0, 0,
            clip->clip_top, clip->clip_bottom, 
            clip->clip_left, clip->clip_right + 1
        );
        Vertex_RGB_List_Free(&cursorVertices);
        Index_List_Free(&cursorIndices);
    }
}

void NU_DrawClippedNodeBorderRect(NodeP* node, float winWidth, float winHeight, NU_ClipBounds* clip)
{
    Vertex_RGB_List vertices;
    Index_List indices;
    Vertex_RGB_List_Init(&vertices, 100);
    Index_List_Init(&indices, 100);
    Construct_Border_Rect(node, winWidth, winHeight, &vertices, &indices);
    Draw_Clipped_Vertex_RGB_List(&vertices, &indices, winWidth, winHeight, 0.0f, 0.0f, clip->clip_top, clip->clip_bottom, clip->clip_left, clip->clip_right);
    Vertex_RGB_List_Free(&vertices);
    Index_List_Free(&indices);
}

inline int NodeNotVisibleInWindow(NodeP* node, int winW, int winH) 
{
    float right  = node->node.x + node->node.width;
    float bottom = node->node.y + node->node.height;
    return (right < 0 || bottom < 0 || node->node.x > winW || node->node.y > winH);
}

void NU_GenerateDrawlists()
{
    // Clear drawlists
    for (int i=0; i<GUI.winManager.windows.size; i++) 
    {
        NU_Window* win = Container_GetAt(&GUI.winManager.windows, i);
        Vector_Clear(&win->drawlist.relativeNodes);
        Vector_Clear(&win->drawlist.absoluteNodes);
        Vector_Clear(&win->drawlist.canvasNodes);
        Vector_Clear(&win->drawlist.clippedRelativeNodes);
        Vector_Clear(&win->drawlist.clippedAbsoluteNodes);
        Vector_Clear(&win->drawlist.clippedCanvasNodes);
    }

    // Clear hashmaps
    HashmapClear(&GUI.winManager.clipMap);
    Vector_Clear(&GUI.winManager.absoluteRootNodes);

    // add root to drawlist 
    NodeP* root = GUI.tree.root;
    SetNodeDrawlist_Relative(&GUI.winManager, root);

    // traverse the tree
    BreadthFirstSearch* bfs = &GUI.bfs;
    BreadthFirstSearch_Reset(bfs, root);
    NodeP* node;
    while(BreadthFirstSearch_Next(bfs, &node)) {

        // node not visible? children must inherit this
        if (node->state == 2) {
            NodeP* child = node->firstChild;
            while(child != NULL) {
                child->state = 2;
                child = child->nextSibling;
            }
            continue;
        }

        // precompute node inner rect
        float nodeInnerX, nodeInnerY, nodeInnerWidth, nodeInnerHeight = 0;
        if (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL) {
            nodeInnerY = node->node.y + node->node.borderTop + node->node.padTop;
            nodeInnerHeight = node->node.height - node->node.borderTop - node->node.borderBottom - node->node.padTop - node->node.padBottom;

            // Handle case where node is a table with a thead
            if (node->type == NU_TABLE && node->childCount > 0) {
                if (node->firstChild->type == NU_THEAD) {
                    nodeInnerY += node->firstChild->node.height;
                    nodeInnerHeight -= node->firstChild->node.height;
                }
            }
        }
        if (node->layoutFlags & OVERFLOW_HORIZONTAL_SCROLL) {
            nodeInnerX = node->node.x + node->node.borderLeft + node->node.padLeft;
            nodeInnerWidth = node->node.width - node->node.borderLeft - node->node.borderRight - node->node.padLeft - node->node.padRight;
        }

        // cache window dimensions
        int winW, winH;
        SDL_Window* window = GetSDL_Window(&GUI.winManager, node->windowID);
        SDL_GetWindowSize(window, &winW, &winH);

        // iterate over children
        NodeP* child = node->firstChild;
        while(child != NULL) 
        {
            if (child->state == 2) {
                child = child->nextSibling; continue;
            }

            // if child not visible in window bounds -> mark as hidden
            if (NodeNotVisibleInWindow(child, winW, winH)) {
                child->state = 2; 
                child = child->nextSibling; continue;
            }

            // add child to list of root absolute nodes
            if (child->layoutFlags & POSITION_ABSOLUTE) {
                Vector_Push(&GUI.winManager.absoluteRootNodes, &child);
            }

            // skip child if overflowed parent's bounds
            if (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL && child->type != NU_THEAD) {
                int intersect_state = NodeVerticalOverlapState(child, nodeInnerY, nodeInnerHeight);

                // child not inside parent -> hide in this draw pass
                if (intersect_state == 0) { 
                    child->state = 2; 
                    child = child->nextSibling; 
                    continue; 
                }

                // child overlaps parent boundary
                else if (intersect_state == 1) {
                    // determine clipping
                    NU_ClipBounds clip;
                    clip.clip_top = fmaxf(child->node.y - 1, nodeInnerY);
                    clip.clip_bottom = fminf(child->node.y + child->node.height, nodeInnerY + nodeInnerHeight);
                    clip.clip_left = -1.0f;
                    clip.clip_right = 1000000.0f;

                    // position absolute -> secondary drawing list
                    if (child->positionAbsolute) {
                        // if node is also clipped -> merge clips (stack clipping behaviour)
                        if (node->clippedAncestor != NULL) {
                            NU_ClipBounds* node_clip = HashmapGet(&GUI.winManager.clipMap, &node->clippedAncestor);
                            clip.clip_top = fmaxf(clip.clip_top, node_clip->clip_top);
                            clip.clip_bottom = fminf(clip.clip_bottom, node_clip->clip_bottom);
                        }

                        // add clipping to hashmap
                        HashmapSet(&GUI.winManager.clipMap, &child, &clip);
                        child->clippedAncestor = child; // Set clip root to self

                        // append node to correct window clipped node list
                        SetNodeDrawlist_ClippedAbsolute(&GUI.winManager, child);
                        child = child->nextSibling; continue;
                    }
                    // position relative -> main drawing list
                    else {
                        // if parent is also clipped -> merge clips (stack clipping behaviour)
                        if (node->clippedAncestor != NULL) {
                            NU_ClipBounds* parent_clip = HashmapGet(&GUI.winManager.clipMap, &node->clippedAncestor);
                            clip.clip_top = fmaxf(clip.clip_top, parent_clip->clip_top);
                            clip.clip_bottom = fminf(clip.clip_bottom, parent_clip->clip_bottom);
                        }
                        
                        // add clipping to hashmap
                        HashmapSet(&GUI.winManager.clipMap, &child, &clip);
                        child->clippedAncestor = child; // Set clip root to self

                        // append node to correct window clipped node list
                        SetNodeDrawlist_ClippedRelative(&GUI.winManager, child);
                        child = child->nextSibling; continue;
                    }
                }
            }

            // if parent is clipped -> child inherits clip from parent
            if (node->clippedAncestor != NULL) {
                child->clippedAncestor = node->clippedAncestor;

                // position absolute -> secondary drawing list
                if (child->positionAbsolute) {
                    SetNodeDrawlist_ClippedAbsolute(&GUI.winManager, child);
                }
                // position relative -> main drawing list
                else {
                    SetNodeDrawlist_ClippedRelative(&GUI.winManager, child);
                }
                child = child->nextSibling; continue;
            }

            // neither child nor parent is clipped -> append node to correct window node list
            if (child->positionAbsolute) { // Position Absolute -> secondary drawing list
                SetNodeDrawlist_Absolute(&GUI.winManager, child);
            }
            else { // position Relative -> main drawing list
                SetNodeDrawlist_Relative(&GUI.winManager, child);
            }

            // append node to canvas API drawing list
            if (child->type == NU_CANVAS && child->typeData.canvas.ctxHandle != -1) {
                SetNodeDrawlist_Canvas(&GUI.winManager, child);
            }

            // move to the next child
            child = child->nextSibling;
        }
    }
}

void NU_Draw()
{
    NU_GenerateDrawlists();

    
    // Initialise text vertex and index buffers (per font)
    Vertex_RGB_UV_List text_relative_vertex_buffers[GUI.stylesheet->fonts.size];
    Vertex_RGB_UV_List text_absolute_vertex_buffers[GUI.stylesheet->fonts.size];
    Index_List text_relative_index_buffers[GUI.stylesheet->fonts.size];
    Index_List text_absolute_index_buffers[GUI.stylesheet->fonts.size];
    for (u32 i=0; i<GUI.stylesheet->fonts.size; i++) {
        Vertex_RGB_UV_List_Init(&text_relative_vertex_buffers[i], 512);
        Index_List_Init(&text_relative_index_buffers[i], 512);
        Vertex_RGB_UV_List_Init(&text_absolute_vertex_buffers[i], 128);
        Index_List_Init(&text_absolute_index_buffers[i], 128);
    }

    Vertex_RGB_List_Clear(&GUI.borderRectVertices); 
    Index_List_Clear(&GUI.borderRectIndices);

    NodeP* focusedInputNode = NULL;

    // For each window
    for (u32 i=0; i<GUI.winManager.windows.size; i++)
    {   
        NU_Window* win = Container_GetAt(&GUI.winManager.windows, i);

        // get the window and dimensions, clear and start new frame
        SDL_Window* window = win->window;
        SDL_GL_MakeCurrent(window, GUI.gl_ctx);
        WindowBeginFrame(window);
        NU_WindowDrawlist* drawList = &win->drawlist;
        int winW_int, winH_int;
        SDL_GetWindowSize(window, &winW_int, &winH_int);
        float winW = (float)winW_int;
        float winH = (float)winH_int;
        

        // --------------------------------------------------------------------------------------------------------
        // --- Draw Relative Positioned Nodes [First Draw Pass] ===================================================
        // --------------------------------------------------------------------------------------------------------

        // cosntruct border rects
        for (u32 n=0; n<drawList->relativeNodes.size; n++) {
            NodeP* node = *(NodeP**)Vector_Get(&drawList->relativeNodes, n);
            Construct_Border_Rect(node, winW, winH, &GUI.borderRectVertices, &GUI.borderRectIndices);
            if (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL 
                && node->node.contentHeight > (node->node.height - node->node.padTop - node->node.padBottom - node->node.borderTop - node->node.borderBottom)) {
                Construct_Scroll_Thumb(node, winW, winH, &GUI.borderRectVertices, &GUI.borderRectIndices);
            }
        }
        
        // draw border rects (one draw call)
        Draw_Vertex_RGB_List(&GUI.borderRectVertices, &GUI.borderRectIndices, winW, winH, 0.0f, 0.0f);
        Vertex_RGB_List_Clear(&GUI.borderRectVertices);
        Index_List_Clear(&GUI.borderRectIndices);

        // draw canvas api content
        for (u32 n=0; n<drawList->canvasNodes.size; n++)
        {
            NodeP* canvas_node = *(NodeP**)Vector_Get(&drawList->canvasNodes, n);
            float offset_x = canvas_node->node.x + canvas_node->node.borderLeft + canvas_node->node.padLeft;
            float offset_y = canvas_node->node.y + canvas_node->node.borderTop + canvas_node->node.padTop;
            float clip_top    = canvas_node->node.y + canvas_node->node.borderTop + canvas_node->node.padTop;
            float clip_bottom = canvas_node->node.y + canvas_node->node.height - canvas_node->node.borderBottom - canvas_node->node.padBottom;
            float clip_left   = canvas_node->node.x + canvas_node->node.borderLeft + canvas_node->node.padLeft;
            float clip_right  = canvas_node->node.x + canvas_node->node.width - canvas_node->node.borderRight - canvas_node->node.padRight;
            NU_Canvas_Context* ctx = Container_Get(&GUI.canvasContexts, canvas_node->typeData.canvas.ctxHandle); 

            // Draw each canvas layer
            for (u32 l=0; l<ctx->canvasLayers.size; l++) {
                CanvasLayer* layer = Vector_Get(&ctx->canvasLayers, l);
                
                // Shape layer
                if (layer->type == NU_CANVAS_SHAPE_LAYER) {
                    Draw_Clipped_Vertex_RGB_List(
                        &layer->vertexData.shapeVertices, &layer->indices,
                        winW, winH, 
                        offset_x, offset_y,
                        clip_top, clip_bottom, clip_left, clip_right 
                    );
                }
                // Text layer
                else 
                {
                    NU_Font* font = Stylesheet_Get_Font(GUI.stylesheet, layer->fontID);
                    NU_Render_Text(
                        &layer->vertexData.textVertices, &layer->indices, 
                        font, 
                        winW, winH, 
                        offset_x, offset_y,
                        clip_top, clip_bottom, clip_left, clip_right
                    );
                }
            }
        }   

        // construct text meshes and draw images
        for (u32 n=0; n<drawList->relativeNodes.size; n++) 
        {
            // construct text mesh for node's textContent
            NodeP* node = *(NodeP**)Vector_Get(&drawList->relativeNodes, n);
            if (node->node.textContent != NULL) {
                Vertex_RGB_UV_List* text_vertices = &text_relative_vertex_buffers[node->fontId];
                Index_List* text_indices = &text_relative_index_buffers[node->fontId];
                NU_AddTextMesh(text_vertices, text_indices, node, node->node.textContent);
            }

            // construct text mesh for input node
            else if (node->type == NU_INPUT) {
                NU_ClipBounds clip = {0};
                clip.clip_top = node->node.y;
                clip.clip_left = node->node.x + node->node.borderLeft + node->node.padLeft;
                clip.clip_right = node->node.x + node->node.width - node->node.borderRight - node->node.padRight;
                clip.clip_bottom = node->node.y + node->node.height + 1000;
                NU_DrawInputNodeContent(node, winW, winH, &clip);
            }    

            // draw node image
            if (node->typeData.image.glImageHandle && node->type != NU_CANVAS && node->type != NU_INPUT) {
                NU_DrawNodeImage(node, winW, winH);
            }
        }
        
        // draw text
        for (u32 t=0; t<GUI.stylesheet->fonts.size; t++) {
            Vertex_RGB_UV_List* text_vertices = &text_relative_vertex_buffers[t];
            Index_List* text_indices = &text_relative_index_buffers[t];
            NU_Font* node_font = Stylesheet_Get_Font(GUI.stylesheet, t);
            NU_Render_Text(text_vertices, text_indices, node_font, winW, winH, 0, 0, -1.0f, 100000.0f, -1.0f, 100000.0f);
            text_vertices->size = 0;
            text_indices->size = 0;
        }
    
        // draw partially visible nodes
        for (u32 n=0; n<drawList->clippedRelativeNodes.size; n++) {
            NodeP* node = *(NodeP**)Vector_Get(&drawList->clippedRelativeNodes, n);

            NU_ClipBounds* clip = (NU_ClipBounds*)HashmapGet(&GUI.winManager.clipMap, &node->clippedAncestor);
            NU_DrawClippedNodeBorderRect(node, winW, winH, clip); // draw border rects

            if (node->node.textContent != NULL) { // draw node's textContent
                NU_DrawClippedNodeTextContent(node, winW, winH, clip);
            }
            else if (node->type == NU_INPUT && node->typeData.input.inputText.numBytes > 0) { // draw input node's text
                NU_ClipBounds innerClip = *clip;
                innerClip.clip_left += node->node.borderLeft + node->node.padLeft;
                innerClip.clip_right -= node->node.borderRight + node->node.padRight; 
                NU_DrawInputNodeContent(node, winW, winH, &innerClip);
            }
            if (node->typeData.image.glImageHandle && node->type != NU_CANVAS && node->type != NU_INPUT) { // draw node image
                NU_DrawClippedNodeImage(node, winW, winH, clip);
            }
        }



        // --------------------------------------------------------------------------------------------------------
        // --- Draw Absolute Positioned Nodes [Second Draw Pass] ==================================================
        // --------------------------------------------------------------------------------------------------------

        // draw border rects
        for (u32 n=0; n<drawList->absoluteNodes.size; n++) { // construct border rect vertices and indices for each node
            NodeP* node = *(NodeP**)Vector_Get(&drawList->absoluteNodes, n);
            Construct_Border_Rect(node, winW, winH, &GUI.borderRectVertices, &GUI.borderRectIndices);
            if (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL 
                && node->node.contentHeight > (node->node.height - node->node.padTop - node->node.padBottom - node->node.borderTop - node->node.borderBottom)) {
                Construct_Scroll_Thumb(node, winW, winH, &GUI.borderRectVertices, &GUI.borderRectIndices);
            }
        }
        Draw_Vertex_RGB_List(&GUI.borderRectVertices, &GUI.borderRectIndices, winW, winH, 0.0f, 0.0f); // draw border rects in one call

        // construct text meshes & draw images
        for (u32 n=0; n<drawList->absoluteNodes.size; n++) 
        {

            // construct text mesh for node's textContent
            NodeP* node = *(NodeP**)Vector_Get(&drawList->absoluteNodes, n);
            if (node->node.textContent != NULL) {
                Vertex_RGB_UV_List* text_vertices = &text_absolute_vertex_buffers[node->fontId];
                Index_List* text_indices = &text_absolute_index_buffers[node->fontId];
                NU_AddTextMesh(text_vertices, text_indices, node, node->node.textContent);
            }

            // construct text mesh for input node's text input
            else if (node->type == NU_INPUT && node->typeData.input.inputText.numBytes > 0) {
                NU_ClipBounds clip = {0};
                clip.clip_top = node->node.y;
                clip.clip_left = node->node.x + node->node.borderLeft + node->node.padLeft;
                clip.clip_right = node->node.x + node->node.width - node->node.borderRight - node->node.padRight;
                clip.clip_bottom = node->node.y + node->node.height + 1000;
                NU_DrawInputNodeContent(node, winW, winH, &clip);
            }

            // draw node image
            if (node->typeData.image.glImageHandle && node->type != NU_CANVAS && node->type != NU_INPUT) {
                NU_DrawNodeImage(node, winW, winH);
            }
        }
        
        // draw text
        for (u32 t=0; t<GUI.stylesheet->fonts.size; t++) {
            Vertex_RGB_UV_List* text_vertices = &text_absolute_vertex_buffers[t];
            Index_List* text_indices = &text_absolute_index_buffers[t];
            NU_Font* node_font = Stylesheet_Get_Font(GUI.stylesheet, t);
            NU_Render_Text(text_vertices, text_indices, node_font, winW, winH, 0, 0, -1.0f, 100000.0f, -1.0f, 100000.0f);
            text_vertices->size = 0;
            text_indices->size = 0;
        }

        // draw partially visible absolute nodes
        for (u32 n=0; n<drawList->clippedAbsoluteNodes.size; n++) {
            NodeP* node = *(NodeP**) Vector_Get(&drawList->clippedAbsoluteNodes, n);
            NU_ClipBounds* clip = (NU_ClipBounds*)HashmapGet(&GUI.winManager.clipMap, &node->clippedAncestor);

            NU_DrawClippedNodeBorderRect(node, winW, winH, clip); // draw border rect

            if (node->node.textContent != NULL) { // draw node's textContent
                NU_DrawClippedNodeTextContent(node, winW, winH, clip);
            }
            else if (node->type == NU_INPUT && node->typeData.input.inputText.numBytes > 0) { // draw input node's text
                NU_ClipBounds innerClip = *clip;
                innerClip.clip_left += node->node.borderLeft + node->node.padLeft;
                innerClip.clip_right -= node->node.borderRight + node->node.padRight; 
                NU_DrawInputNodeContent(node, winW, winH, &innerClip);
            }
            if (node->typeData.image.glImageHandle && node->type != NU_CANVAS && node->type != NU_INPUT) { // draw node image
                NU_DrawClippedNodeImage(node, winW, winH, clip);
            }
        }
        SDL_GL_SwapWindow(window); 
    }
    
    // -----------------------
    // --- Free memory -------
    // -----------------------
    for (u32 i=0; i<GUI.stylesheet->fonts.size; i++) {
        Vertex_RGB_UV_List_Free(&text_relative_vertex_buffers[i]);
        Index_List_Free(&text_relative_index_buffers[i]);
        Vertex_RGB_UV_List_Free(&text_absolute_vertex_buffers[i]);
        Index_List_Free(&text_absolute_index_buffers[i]);
    }
    GUI.awaiting_redraw = false;
}