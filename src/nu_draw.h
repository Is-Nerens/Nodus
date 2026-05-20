#pragma once
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <math.h>

typedef enum NodeOverlap
{
    NODE_OVERLAP_NONE,
    NODE_OVERLAP_PARTIAL,
    NODE_OVERLAP_INSIDE
} NodeOverlap;

void NU_AddTextMesh(NodeP* node, float z, char* textBuffer, Vertex_RGB_UV_List* vertices, Index_List* indices)
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
    NU_Font* node_font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontId);
    NU_Generate_Text_Mesh(vertices, indices, node_font, textBuffer, floorf(textPosX), floorf(textPosY), z, r, g, b, inner_width);
}

static NodeOverlap NodeVerticalOverlapState(NodeP* node, float y, float h)
{
    float top = node->node.y;
    float bottom = top + node->node.height;
    float bounds_top = y;
    float bounds_bottom = y + h;
    int overlap = (bottom > bounds_top) & (top < bounds_bottom);
    int inside = (top >= bounds_top) & (bottom <= bounds_bottom);
    return (NodeOverlap)(overlap + inside);
}

void NU_DrawNodeImage(NodeP* node, float z, float winWidth, float winHeight)
{
    float inner_width  = node->node.width - node->node.borderLeft - node->node.borderRight - node->node.padLeft - node->node.padRight;
    float inner_height = node->node.height - node->node.borderTop - node->node.borderBottom - node->node.padTop - node->node.padBottom;
    float x = node->node.x + node->node.borderLeft + (float)node->node.padLeft;
    float y = node->node.y + node->node.borderTop + (float)node->node.padTop;
    NU_Draw_Image(
        x, y, 
        inner_width, inner_height, z,
        winWidth, winHeight, 
        -1.0f, 100000.0f, -1.0f, 100000.0f,
        node->typeData.image.glImageHandle
    );
}

void NU_DrawClippedNodeImage(NodeP* node, float z, float winWidth, float winHeight, NU_ClipBounds* clip)
{
    float inner_width  = node->node.width - node->node.borderLeft - node->node.borderRight - node->node.padLeft - node->node.padRight;
    float inner_height = node->node.height - node->node.borderTop - node->node.borderBottom - node->node.padTop - node->node.padBottom;
    float x = node->node.x + node->node.borderLeft + (float)node->node.padLeft;
    float y = node->node.y + node->node.borderTop + (float)node->node.padTop;
    NU_Draw_Image(
        x, y, 
        inner_width, inner_height, z,
        winWidth, winHeight, 
        clip->top, clip->bottom, clip->left, clip->right,
        node->typeData.image.glImageHandle
    );
}

void NU_DrawClippedNodeTextContent(NodeP* node, float z, float winWidth, float winHeight, NU_ClipBounds* clip)
{
    Vertex_RGB_UV_List clipped_text_vertices;
    Index_List clipped_text_indices;
    Vertex_RGB_UV_List_Init(&clipped_text_vertices, 1000);
    Index_List_Init(&clipped_text_indices, 600);
    NU_Font* node_font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontId);
    NU_AddTextMesh(node, z, node->node.textContent, &clipped_text_vertices, &clipped_text_indices);
    NU_Render_Text(&clipped_text_vertices, &clipped_text_indices, node_font, winWidth, winHeight, 0, 0, clip->top, clip->bottom, clip->left, clip->right);
    Vertex_RGB_UV_List_Free(&clipped_text_vertices);
    Index_List_Free(&clipped_text_indices);
}

void NU_DrawInputNodeContent(NodeP* node, float z, float winWidth, float winHeight, NU_ClipBounds* clip)
{   
    NU_Font* node_font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontId);
    InputText* inputText = Container_Get(&GUI.textInputs, node->typeData.input.textInputHandle);

    if (inputText->updateOffsetsPostLayout) {
        inputText->updateOffsetsPostLayout = false;
        InputText_ComputeCursorTextOffset_PlaceEnd(inputText, node, node_font);
    }

    // construct and draw highlight mesh
    if (GUI.focused_node != NULL && node == GUI.focused_node && InputText_IsHighlighting(inputText)) 
    {
        Vertex_RGB_List highlightVertices; Vertex_RGB_List_Init(&highlightVertices, 4);
        Index_List highlightIndices; Index_List_Init(&highlightIndices, 6);
        NU_ConstructInputHighlightMesh(node, z + 0.25f, inputText, &highlightVertices, &highlightIndices);
        Draw_Clipped_Vertex_RGB_List(
            &highlightVertices, &highlightIndices,
            winWidth, winHeight, 
            0, 0,
            clip->top, clip->bottom, 
            clip->left, clip->right + 1
        );
        Vertex_RGB_List_Free(&highlightVertices);
        Index_List_Free(&highlightIndices);
    }

    // generate and draw text
    Vertex_RGB_UV_List clipped_text_vertices; Vertex_RGB_UV_List_Init(&clipped_text_vertices, 1000);
    Index_List clipped_text_indices; Index_List_Init(&clipped_text_indices, 600);
    float textPosX = node->node.x + node->node.borderLeft + node->node.padLeft + inputText->textOffset;
    float textPosY = node->node.y + node->node.borderTop  + node->node.padTop;
    float r = (float)node->node.textR / 255.0f;
    float g = (float)node->node.textG / 255.0f;
    float b = (float)node->node.textB / 255.0f;
    NU_Generate_Text_Mesh(&clipped_text_vertices, &clipped_text_indices, node_font, inputText->buffer, floorf(textPosX), floorf(textPosY), z + 0.5f, r, g, b, 10000000.0f);
    NU_Render_Text(&clipped_text_vertices, &clipped_text_indices, node_font, winWidth, winHeight, 0, 0, clip->top, clip->bottom, clip->left, clip->right);
    Vertex_RGB_UV_List_Free(&clipped_text_vertices);
    Index_List_Free(&clipped_text_indices);

    // draw cursor afterwards (if input is focused)
    if (GUI.focused_node != NULL && node == GUI.focused_node
        && !InputText_IsHighlighting(inputText)) 
    {

        Vertex_RGB_List cursorVertices; Vertex_RGB_List_Init(&cursorVertices, 4);
        Index_List cursorIndices; Index_List_Init(&cursorIndices, 6);
        NU_ConstructInputCursorMesh(node, z + 0.5f, inputText, &cursorVertices, &cursorIndices);
        Draw_Clipped_Vertex_RGB_List(
            &cursorVertices, &cursorIndices,
            winWidth, winHeight, 
            0, 0,
            clip->top, clip->bottom, 
            clip->left, clip->right + 1
        );
        Vertex_RGB_List_Free(&cursorVertices);
        Index_List_Free(&cursorIndices);
    }
}

void NU_DrawCanvasContent(NodeP* canvas_node, float winW, float winH)
{
    float offsetX = canvas_node->node.x + canvas_node->node.borderLeft + canvas_node->node.padLeft;
    float offsetY = canvas_node->node.y + canvas_node->node.borderTop + canvas_node->node.padTop;
    float top    = canvas_node->node.y + canvas_node->node.borderTop + canvas_node->node.padTop;
    float bottom = canvas_node->node.y + canvas_node->node.height - canvas_node->node.borderBottom - canvas_node->node.padBottom;
    float left   = canvas_node->node.x + canvas_node->node.borderLeft + canvas_node->node.padLeft;
    float right  = canvas_node->node.x + canvas_node->node.width - canvas_node->node.borderRight - canvas_node->node.padRight;
    NU_Canvas_Context* ctx = Container_Get(&GUI.canvasContexts, canvas_node->typeData.canvas.ctxHandle); 
    ctx->canvasWidth = canvas_node->node.width;
    ctx->canvasHeight = canvas_node->node.height;

    // Draw canvas shape layer
    Draw_Clipped_Vertex_RGB_List(
        &ctx->shapeLayer.vertices, &ctx->shapeLayer.indices,
        winW, winH, 
        offsetX, offsetY,
        top, bottom, left, right 
    );

    // Draw each canvas text layer
    for (int l=0; l<ctx->textLayerIndex+1; l++) {
        CanvasTextLayer* layer = ArrayGet(&ctx->textLayers, l);
        NU_Font* font = Stylesheet_Get_Font(&GUI.stylesheet, layer->fontID);
        NU_Render_Text(
            &layer->vertices, &layer->indices, 
            font, 
            winW, winH, 
            offsetX, offsetY,
            top, bottom, left, right
        );
    }
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
        ArrayClear(&win->drawlist.relativeNodes);
        ArrayClear(&win->drawlist.absoluteNodes);
        ArrayClear(&win->drawlist.clippedRelativeNodes);
        ArrayClear(&win->drawlist.clippedAbsoluteNodes);
    }

    // Clear hashmaps
    HashmapClear(&GUI.winManager.clipMap);
    ArrayClear(&GUI.winManager.absoluteRootNodes);

    // Add root to drawlist 
    NodeP* root = GUI.tree.root;
    SetNodeDrawlist_Relative(&GUI.winManager, root);

    // Traverse the tree
    BreadthFirstSearch* bfs = &GUI.bfs;
    BreadthFirstSearch_Reset(bfs, root);
    NodeP* node;
    while(BreadthFirstSearch_Next(bfs, &node)) {

        // Node not visible? children must inherit this
        if (NodeStateHidden(node)) {
            NodeP* child = node->firstChild;
            while(child != NULL) {
                child->stateFlags |= STATE_FLAG_HIDDEN;
                child = child->nextSibling;
            }
            continue;
        }

        // Precompute node inner rect
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
            // if child is not visible (or not visible in window -> mark as hidded) -> skip
            if (NodeStateHidden(child) | NodeNotVisibleInWindow(child, winW, winH)) {
                child->stateFlags |= STATE_FLAG_HIDDEN; 
                child = child->nextSibling; continue;
            }

            // add child to list of root absolute nodes
            if (child->layoutFlags & POSITION_ABSOLUTE) {
                ArrayPush(&GUI.winManager.absoluteRootNodes, &child);
            }

            // skip child if overflowed parent's bounds
            if (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL && child->type != NU_THEAD) {
                NodeOverlap verticalOverlap = NodeVerticalOverlapState(child, nodeInnerY, nodeInnerHeight);

                // child not inside parent -> hide in this draw pass
                if (verticalOverlap == NODE_OVERLAP_NONE) { 
                    child->stateFlags |= STATE_FLAG_HIDDEN; 
                    child = child->nextSibling; 
                    continue; 
                }

                // child overlaps parent boundary
                else if (verticalOverlap == NODE_OVERLAP_PARTIAL) {
                    // determine clipping
                    NU_ClipBounds clip;
                    clip.top = fmaxf(child->node.y - 1, nodeInnerY);
                    clip.bottom = fminf(child->node.y + child->node.height, nodeInnerY + nodeInnerHeight);
                    clip.left = -1.0f;
                    clip.right = 1000000.0f;

                    // position absolute -> secondary drawing list
                    if (NodeStatePosAbsolute(child)) {
                        
                        // if node is also clipped -> merge clips (stack clipping behaviour)
                        if (node->clippedAncestor != NULL) {
                            NU_ClipBounds* node_clip = HashmapGet(&GUI.winManager.clipMap, &node->clippedAncestor);
                            clip.top = fmaxf(clip.top, node_clip->top);
                            clip.bottom = fminf(clip.bottom, node_clip->bottom);
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
                            clip.top = fmaxf(clip.top, parent_clip->top);
                            clip.bottom = fminf(clip.bottom, parent_clip->bottom);
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
                if (NodeStatePosAbsolute(child)) {
                    SetNodeDrawlist_ClippedAbsolute(&GUI.winManager, child);
                }
                // position relative -> main drawing list
                else {
                    SetNodeDrawlist_ClippedRelative(&GUI.winManager, child);
                }
                child = child->nextSibling; continue;
            }

            // neither child nor parent is clipped -> append node to correct window node list
            if (NodeStatePosAbsolute(child)) { // Position Absolute -> secondary drawing list
                SetNodeDrawlist_Absolute(&GUI.winManager, child);
            }
            else { // position Relative -> main drawing list
                SetNodeDrawlist_Relative(&GUI.winManager, child);
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
    Vertex_RGB_UV_List text_vertex_buffers[GUI.stylesheet.fonts.size];
    Index_List text_index_buffers[GUI.stylesheet.fonts.size];
    for (u32 i=0; i<GUI.stylesheet.fonts.size; i++) {
        Vertex_RGB_UV_List_Init(&text_vertex_buffers[i], 512);
        Index_List_Init(&text_index_buffers[i], 512);
    }

    ArrayClear(&GUI.borderRects);

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
        
        // 1. Generate border rect data for relative nodes
        for (u32 n=0; n<drawList->relativeNodes.size; n++) 
        {
            NodeP* node = *(NodeP**)ArrayGet(&drawList->relativeNodes, n);
            float z = (float)(node->layer);

            // Construct border rect data
            Add_NodeRectRenderData(node, z, 0.0f, winH, 0.0f, winW, &GUI.borderRects);
            if (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL 
                && node->node.contentHeight > (node->node.height - node->node.padTop - node->node.padBottom - node->node.borderTop - node->node.borderBottom)) {
                Add_ScrollbarRenderData(node, z + 0.5f, &GUI.stylesheet.scrollbarStyle, &GUI.borderRects);
            }
            // Construct text mesh for node's textContent
            if (node->node.textContent != NULL) {
                Vertex_RGB_UV_List* text_vertices = &text_vertex_buffers[node->fontId];
                Index_List* text_indices = &text_index_buffers[node->fontId];
                NU_AddTextMesh(node, z, node->node.textContent, text_vertices, text_indices);
            }
            // Draw text input content (1 draw call)
            else if (node->type == NU_INPUT) {
                NU_ClipBounds clip = {0};
                clip.top = node->node.y;
                clip.left = node->node.x + node->node.borderLeft + node->node.padLeft;
                clip.right = node->node.x + node->node.width - node->node.borderRight - node->node.padRight;
                clip.bottom = node->node.y + node->node.height + 1000;
                NU_DrawInputNodeContent(node, z, winW, winH, &clip);
            }   

            // Draw canvas content
            if (node->type == NU_CANVAS) NU_DrawCanvasContent(node, winW, winH);
        }

        // 2. Generate border rect data for absolute nodes
        for (u32 n=0; n<drawList->absoluteNodes.size; n++) 
        {
            NodeP* node = *(NodeP**)ArrayGet(&drawList->absoluteNodes, n);
            float z = (float)(node->layer) + 32.0f;

            // Construct border rect data
            Add_NodeRectRenderData(node, z, 0.0f, winH, 0.0f, winW, &GUI.borderRects);
            if (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL 
                && node->node.contentHeight > (node->node.height - node->node.padTop - node->node.padBottom - node->node.borderTop - node->node.borderBottom)) {
                Add_ScrollbarRenderData(node, z + 0.5f, &GUI.stylesheet.scrollbarStyle, &GUI.borderRects);
            }
            // Construct text mesh for node's textContent
            if (node->node.textContent != NULL) {
                Vertex_RGB_UV_List* text_vertices = &text_vertex_buffers[node->fontId];
                Index_List* text_indices = &text_index_buffers[node->fontId];
                NU_AddTextMesh(node, z, node->node.textContent, text_vertices, text_indices);
            }
            // Draw text input content (1 draw call)
            else if (node->type == NU_INPUT) {
                NU_ClipBounds clip = {0};
                clip.top = node->node.y;
                clip.left = node->node.x + node->node.borderLeft + node->node.padLeft;
                clip.right = node->node.x + node->node.width - node->node.borderRight - node->node.padRight;
                clip.bottom = node->node.y + node->node.height + 1000;
                NU_DrawInputNodeContent(node, z, winW, winH, &clip);
            }  

            // Draw canvas content
            if (node->type == NU_CANVAS) NU_DrawCanvasContent(node, winW, winH);
        }

        // 3. Draw all unclipped border rects (1 draw call)
        Draw_SDF_Border_Rects(GUI.borderRects, winW, winH); ArrayClear(&GUI.borderRects);

        // 4. Draw all unclipped text (1 draw call per font)
        for (u32 t=0; t<GUI.stylesheet.fonts.size; t++) {
            Vertex_RGB_UV_List* text_vertices = &text_vertex_buffers[t];
            Index_List* text_indices = &text_index_buffers[t];
            NU_Font* node_font = Stylesheet_Get_Font(&GUI.stylesheet, t);
            NU_Render_Text(text_vertices, text_indices, node_font, winW, winH, 0, 0, -1.0f, 100000.0f, -1.0f, 100000.0f);
            text_vertices->size = 0;
            text_indices->size = 0;
        }

        // 5. Draw unclipped relative node images
        for (u32 n=0; n<drawList->relativeNodes.size; n++) 
        {
            NodeP* node = *(NodeP**)ArrayGet(&drawList->relativeNodes, n);
            float z = (float)(node->layer);
            if (node->typeData.image.glImageHandle && node->type != NU_CANVAS && node->type != NU_INPUT) {
                NU_DrawNodeImage(node, z + 0.75f, winW, winH);
            }
        }

        // 6. Draw unclipped absolute node images
        for (u32 n=0; n<drawList->absoluteNodes.size; n++) 
        {
            NodeP* node = *(NodeP**)ArrayGet(&drawList->absoluteNodes, n);
            float z = (float)(node->layer);
            if (node->typeData.image.glImageHandle && node->type != NU_CANVAS && node->type != NU_INPUT) {
                NU_DrawNodeImage(node, z + 0.75f, winW, winH);
            }
        }

        // 7. Draw clipped relative node border rects + images + text + text input
        for (u32 n=0; n<drawList->clippedRelativeNodes.size; n++) {
            NodeP* node = *(NodeP**)ArrayGet(&drawList->clippedRelativeNodes, n);
            float z = (float)(node->layer);

            // Draw border rect (1 draw call)
            NU_ClipBounds* clip = (NU_ClipBounds*)HashmapGet(&GUI.winManager.clipMap, &node->clippedAncestor);
            Add_NodeRectRenderData(node, z, clip->top, clip->bottom, clip->left, clip->right, &GUI.borderRects);
            Draw_SDF_Border_Rects(GUI.borderRects, winW, winH); ArrayClear(&GUI.borderRects);

            // Draw text content (1 draw call)
            if (node->node.textContent != NULL) {
                NU_DrawClippedNodeTextContent(node, z + 0.5f, winW, winH, clip);
            }
            // Draw text input (1 draw call)
            else if (node->type == NU_INPUT) {
                InputText* inputText = Container_Get(&GUI.textInputs, node->typeData.input.textInputHandle);
                if (inputText->numBytes > 0) {
                    NU_ClipBounds innerClip = *clip;
                    innerClip.left += node->node.borderLeft + node->node.padLeft;
                    innerClip.right -= node->node.borderRight + node->node.padRight; 
                    NU_DrawInputNodeContent(node, z, winW, winH, &innerClip);
                }
            }
            // Draw image (1 draw call)
            if (node->typeData.image.glImageHandle && node->type != NU_CANVAS && node->type != NU_INPUT) {
                NU_DrawClippedNodeImage(node, z + 0.75f, winW, winH, clip);
            }
        }

        // 8. Draw clipped absolute node border rects + images + text + text input
        for (u32 n=0; n<drawList->clippedAbsoluteNodes.size; n++) {
            NodeP* node = *(NodeP**) ArrayGet(&drawList->clippedAbsoluteNodes, n);
            float z = (float)(node->layer) + 32.0f;

            // Draw border rect (1 draw call)
            NU_ClipBounds* clip = (NU_ClipBounds*)HashmapGet(&GUI.winManager.clipMap, &node->clippedAncestor);
            Add_NodeRectRenderData(node, z, clip->top, clip->bottom, clip->left, clip->right, &GUI.borderRects);
            Draw_SDF_Border_Rects(GUI.borderRects, winW, winH); ArrayClear(&GUI.borderRects);

            // Draw text content (1 draw call)
            if (node->node.textContent != NULL) {
                NU_DrawClippedNodeTextContent(node, z + 0.5f, winW, winH, clip);
            }
            // Draw text input (1 draw call)
            else if (node->type == NU_INPUT) {
                InputText* inputText = Container_Get(&GUI.textInputs, node->typeData.input.textInputHandle);
                if (inputText->numBytes > 0) {
                    NU_ClipBounds innerClip = *clip;
                    innerClip.left += node->node.borderLeft + node->node.padLeft;
                    innerClip.right -= node->node.borderRight + node->node.padRight; 
                    NU_DrawInputNodeContent(node, z, winW, winH, &innerClip);
                }
            }
            // Draw image (1 draw call)
            if (node->typeData.image.glImageHandle && node->type != NU_CANVAS && node->type != NU_INPUT) {
                NU_DrawClippedNodeImage(node, z + 0.75f, winW, winH, clip);
            }
        }

        SDL_GL_SwapWindow(window); 
    }

    // -----------------------
    // --- Free memory -------
    // -----------------------
    for (u32 i=0; i<GUI.stylesheet.fonts.size; i++) {
        Vertex_RGB_UV_List_Free(&text_vertex_buffers[i]);
        Index_List_Free(&text_index_buffers[i]);
    }
    GUI.awaiting_redraw = false;
}