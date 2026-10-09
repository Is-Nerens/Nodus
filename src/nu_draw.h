#pragma once
#include "rendering/nu_renderer_structures.h"
#include "tree/nu_node_alloc.h"
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <math.h>

static NodeOverlap NodeVerticalOverlapState(Node* node, float y, float h)
{
    float top = node->public.generic.y;
    float bottom = top + node->public.generic.height;
    float bounds_top = y;
    float bounds_bottom = y + h;
    int overlap = (bottom > bounds_top) & (top < bounds_bottom);
    int inside = (top >= bounds_top) & (bottom <= bounds_bottom);
    return (NodeOverlap)(overlap + inside);
}

inline int NodeOutsideWindow(Node* node, int winW, int winH)
{
    float right  = node->public.generic.x + node->public.generic.width;
    float bottom = node->public.generic.y + node->public.generic.height;
    return (right < 0 || bottom < 0 || node->public.generic.x > winW || node->public.generic.y > winH);
}

void NU_AddTextMesh(Node* node, float z, char* textBuffer, Array* textRenderData)
{
    // Compute node inner dimensions
    float inner_width  = node->public.generic.width  - node->public.generic.borderLeft - node->public.generic.borderRight - node->public.generic.padLeft - node->public.generic.padRight;
    float inner_height = node->public.generic.height - node->public.generic.borderTop  - node->public.generic.borderBottom - node->public.generic.padTop - node->public.generic.padBottom;
    float remaining_w = inner_width  - node->public.generic.contentWidth;
    float remaining_h = inner_height - node->public.generic.contentHeight;

    // Compute text position
    float x_align_offset = remaining_w * 0.5f * (float)node->horizontalTextAlignment;
    float y_align_offset = remaining_h * 0.5f * (float)node->verticalTextAlignment;
    float textPosX = floorf(node->public.generic.x + node->public.generic.borderLeft + node->public.generic.padLeft + x_align_offset);
    float textPosY = floorf(node->public.generic.y + node->public.generic.borderTop  + node->public.generic.padTop + y_align_offset);

    // Add glyph render data
    NU_Font* node_font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontIdx);
    NU_Generate_Text_Renderdata(
        textRenderData,
        node_font,
        textBuffer,
        textPosX,
        textPosY,
        z,
        node->public.generic.textR,
        node->public.generic.textG,
        node->public.generic.textB,
        inner_width
    );
}

void NU_DrawInputNodeContent(Node* node, float z, float winWidth, float winHeight, NU_ClipBounds* clip)
{
    // Get font and inout text datastructure
    NU_Font* node_font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontIdx);
    InputText* inputText = Container_Get(&GUI.textInputs, node->special.input.textInputHandle);

    // Recompute text offset
    if (inputText->updateOffsetsPostLayout) {
        inputText->updateOffsetsPostLayout = false;
        InputText_ComputeCursorTextOffset_PlaceEnd(inputText, node, node_font);
    }

    // Construct and draw text highlight
    if (GUI.focused_node != NULL && node == GUI.focused_node && InputText_IsHighlighting(inputText))
    {
        Vertex_RGB_List highlightVertices; Vertex_RGB_List_Init(&highlightVertices, 4);
        Index_List highlightIndices; Index_List_Init(&highlightIndices, 6);
        NU_ConstructInputHighlightMesh(node, z + 0.25f, inputText, &highlightVertices, &highlightIndices);
        Draw_Clipped_Vertex_RGB_List(
            &highlightVertices, &highlightIndices,
            winWidth, winHeight, 0, 0,
            clip->top, clip->bottom,
            clip->left, clip->right + 1
        );
        Vertex_RGB_List_Free(&highlightVertices);
        Index_List_Free(&highlightIndices);
    }

    // Draw text
    float textPosX = floorf(node->public.generic.x + node->public.generic.borderLeft + node->public.generic.padLeft + inputText->textOffset);
    float textPosY = floorf(node->public.generic.y + node->public.generic.borderTop  + node->public.generic.padTop);
    Array textRenderData;
    Array_Init(&textRenderData, sizeof(GlyphRenderData), 256);
    NU_Generate_Text_Renderdata(
        &textRenderData,
        node_font,
        inputText->buffer,
        textPosX,
        textPosY,
        z + 0.5f,
        node->public.generic.textR,
        node->public.generic.textG,
        node->public.generic.textB,
        -1.0f
    );
    NU_Draw_Clipped_Text(textRenderData, node_font, winWidth, winHeight, 0, 0, clip->top, clip->bottom, clip->left, clip->right);
    Array_Free(&textRenderData);

    // Draw cursor (if input is focused)
    if (node == GUI.focused_node && !InputText_IsHighlighting(inputText))
    {
        Vertex_RGB_List cursorVertices; Vertex_RGB_List_Init(&cursorVertices, 4);
        Index_List cursorIndices; Index_List_Init(&cursorIndices, 6);
        NU_ConstructInputCursorMesh(node, z + 0.5f, inputText, &cursorVertices, &cursorIndices);
        Draw_Clipped_Vertex_RGB_List(
            &cursorVertices, &cursorIndices,
            winWidth, winHeight, 0, 0,
            clip->top, clip->bottom,
            clip->left, clip->right + 1
        );
        Vertex_RGB_List_Free(&cursorVertices);
        Index_List_Free(&cursorIndices);
    }
}

void NU_DrawCanvasContent(Node* canvas_node, float winW, float winH, NU_ClipBounds* clip)
{
    // Get canvas ctx
    NU_Canvas_Context* ctx = Container_Get(&GUI.canvasContexts, canvas_node->special.canvas.ctxHandle);
    if (ctx == NULL) return;

    // Compute offsets and clipping bounds
    float offsetX = roundf(canvas_node->public.generic.x + canvas_node->public.generic.borderLeft + canvas_node->public.generic.padLeft);
    float offsetY = roundf(canvas_node->public.generic.y + canvas_node->public.generic.borderTop + canvas_node->public.generic.padTop);
    float top    = canvas_node->public.generic.y + canvas_node->public.generic.borderTop + canvas_node->public.generic.padTop;
    float bottom = canvas_node->public.generic.y + canvas_node->public.generic.height - canvas_node->public.generic.borderBottom - canvas_node->public.generic.padBottom;
    float left   = canvas_node->public.generic.x + canvas_node->public.generic.borderLeft + canvas_node->public.generic.padLeft;
    float right  = canvas_node->public.generic.x + canvas_node->public.generic.width - canvas_node->public.generic.borderRight - canvas_node->public.generic.padRight;
    if (clip)
    {
        top    = fmax_fast(top, clip->top);
        bottom = fmax_fast(bottom, clip->bottom);
        left   = fmax_fast(left, clip->left);
        right  = fmax_fast(right, clip->right);
    }
    ctx->canvasWidth = canvas_node->public.generic.width;
    ctx->canvasHeight = canvas_node->public.generic.height;

    // Draw shape layer
    Draw_Clipped_Vertex_RGB_List(
        &ctx->shapeLayer.vertices, &ctx->shapeLayer.indices,
        winW, winH,
        offsetX, offsetY,
        top, bottom, left, right
    );

    // Draw text layers
    for (int l=0; l<ctx->textLayerIndex+1; l++) {
        CanvasTextLayer* layer = Array_Get(&ctx->textLayers, l);
        NU_Font* font = Stylesheet_Get_Font(&GUI.stylesheet, layer->fontID);
        NU_Draw_Clipped_Text(
            layer->textRenderData, font,
            winW, winH,
            offsetX, offsetY,
            top, bottom, left, right
        );
    }
}

void NU_GenerateDrawlists()
{
    // Clear drawlists
    for (int i=0; i<GUI.winManager.windows.size; i++) {
        NU_Window* win = Container_GetAt(&GUI.winManager.windows, i);
        Array_Clear(&win->drawlist.drawNodes);
        Array_Clear(&win->drawlist.clippedDrawNodes);
        Array_Clear(&win->drawlist.canvasNodes);
    }

    // Clear hashmaps
    Hashmap_Clear(&GUI.winManager.clipMap);
    Array_Clear(&GUI.winManager.absoluteRootNodes);

    // Add root to drawlist
    Node* root = GUI.tree.root;
    SetNodeDrawlist_Draw(&GUI.winManager, root);

    // Traverse the tree
    BreadthFirstSearch* bfs = &GUI.bfs;
    BreadthFirstSearch_Reset(bfs, root);
    Node* node;
    while(BreadthFirstSearch_Next(bfs, &node)) {

        // Precompute node inner rect
        float nodeInnerX, nodeInnerY, nodeInnerWidth, nodeInnerHeight = 0;
        if (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL) {
            nodeInnerY = node->public.generic.y + node->public.generic.borderTop + node->public.generic.padTop;
            nodeInnerHeight = node->public.generic.height - node->public.generic.borderTop - node->public.generic.borderBottom - node->public.generic.padTop - node->public.generic.padBottom;

            // Handle case where node is a table with a thead
            if (node->type == NU_TYPE_TABLE && node->childCount > 0) {
                if (node->firstChild->type == NU_TYPE_THEAD) {
                    nodeInnerY += node->firstChild->public.generic.height;
                    nodeInnerHeight -= node->firstChild->public.generic.height;
                }
            }
        }
        if (node->layoutFlags & OVERFLOW_HORIZONTAL_SCROLL) {
            nodeInnerX = node->public.generic.x + node->public.generic.borderLeft + node->public.generic.padLeft;
            nodeInnerWidth = node->public.generic.width - node->public.generic.borderLeft - node->public.generic.borderRight - node->public.generic.padLeft - node->public.generic.padRight;
        }

        // cache window dimensions
        int winW, winH;
        SDL_Window* window = GetSDL_Window(&GUI.winManager, node->windowID);
        SDL_GetWindowSizeInPixels(window, &winW, &winH);

        // iterate over children
        Node* child = node->firstChild;
        while(child)
        {
            // if parent is not visible (ad child inherets parent's visibility) OR child is not visible OR child not visible in it's window -> mark as hidded -> skip
            if ((NodeStateHidden(node) && child->type != NU_TYPE_WINDOW) || NodeStateHidden(child) || NodeOutsideWindow(child, winW, winH)) {
                child->stateFlags |= STATE_FLAG_HIDDEN;
                child = child->nextSibling; continue;
            }

            // add child to list of root absolute nodes
            if (child->layoutFlags & POSITION_ABSOLUTE) {
                Array_Push(&GUI.winManager.absoluteRootNodes, &child);
            }

            // if overflowed parent's bounds
            if (child->type != NU_TYPE_WINDOW && child->type != NU_TYPE_THEAD && (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL)) {
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
                    clip.top = fmaxf(child->public.generic.y - 1, nodeInnerY);
                    clip.bottom = fminf(child->public.generic.y + child->public.generic.height, nodeInnerY + nodeInnerHeight);
                    clip.left = -1.0f;
                    clip.right = 1000000.0f;

                    // if parent is also clipped -> merge clips (stack clipping behaviour)
                    if (node->clippedAncestor) {
                        NU_ClipBounds* parent_clip = Hashmap_Get(&GUI.winManager.clipMap, node->clippedAncestor);
                        clip.top = fmaxf(clip.top, parent_clip->top);
                        clip.bottom = fminf(clip.bottom, parent_clip->bottom);
                    }

                    // add clipping to hashmap
                    Hashmap_Set(&GUI.winManager.clipMap, &child, &clip);
                    child->clippedAncestor = child; // Set clip root to self

                    // append node to correct window clipped node list
                    SetNodeDrawlist_Clipped(&GUI.winManager, child);
                    if (child->type == NU_TYPE_CANVAS) SetNodeDrawlist_Canvas(&GUI.winManager, child);
                    child = child->nextSibling; continue;
                }
            }

            // if parent is clipped -> child inherits clip from parent
            if (node->clippedAncestor) {
                child->clippedAncestor = node->clippedAncestor;
                SetNodeDrawlist_Clipped(&GUI.winManager, child);
                if (child->type == NU_TYPE_CANVAS) SetNodeDrawlist_Canvas(&GUI.winManager, child);
                child = child->nextSibling; continue;
            }

            // neither child nor parent is clipped -> append node to correct window node list
            SetNodeDrawlist_Draw(&GUI.winManager, child);
            if (child->type == NU_TYPE_CANVAS) SetNodeDrawlist_Canvas(&GUI.winManager, child);

            // move to the next child
            child = child->nextSibling;
        }
    }
}

void NU_Draw()
{
    NU_GenerateDrawlists();

    Array_Clear(&GUI.borderRectRenderDataArray);

    // Re-upload font atlases as needed
    for (u32 t=0; t<GUI.stylesheet.fonts.size; t++) {
        NU_Font* font = Stylesheet_Get_Font(&GUI.stylesheet, t);
        NU_Font_Atlas_Upload_Or_Modify_GPU(&font->atlas);
    }

    // Clear per-font render data arrays
    for (int i=0; i<GUI.stylesheet.fonts.size; i++) {
        Array* glyphRenderDataArray = Array_Get(&GUI.perFontTextRenderData, i);
        Array_Clear(glyphRenderDataArray);
    }

    Node* focusedInputNode = NULL;

    // For each window
    for (u32 i=0; i<GUI.winManager.windows.size; i++)
    {
        NU_Window* win = Container_GetAt(&GUI.winManager.windows, i);
        ImageResourceManager_ClearAllImageRenderData(&GUI.imageResourceManager);

        // get the window and dimensions, clear and start new frame
        SDL_Window* window = win->window;
        SDL_GL_MakeCurrent(window, GUI.gl_ctx);
        WindowBeginFrame(window);
        NU_WindowDrawlist* drawList = &win->drawlist;
        int winW_int, winH_int;
        SDL_GetWindowSizeInPixels(window, &winW_int, &winH_int);
        float winW = (float)winW_int;
        float winH = (float)winH_int;

        // 1. Generate border rect data for unclipped nodes
        for (u32 n=0; n<drawList->drawNodes.size; n++)
        {
            Node* node = *(Node**)Array_Get(&drawList->drawNodes, n);
            float z = (float)(node->layer) + 32.0f * NodeStatePosAbsolute(node);

            // Construct border rect data
            Add_NodeRectRenderData(node, z, 0.0f, winH, 0.0f, winW, &GUI.borderRectRenderDataArray);
            if (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL
                && node->public.generic.contentHeight > (node->public.generic.height - node->public.generic.padTop - node->public.generic.padBottom - node->public.generic.borderTop - node->public.generic.borderBottom)) {
                Add_ScrollbarRenderData(node, z + 0.5f, &GUI.stylesheet.scrollbarStyle, &GUI.borderRectRenderDataArray);
            }
            // Construct text mesh for node's textContent
            if (node->public.generic.textContent != NULL) {
                Array* textRenderData = Array_Get(&GUI.perFontTextRenderData, node->fontIdx);
                NU_AddTextMesh(node, z, node->public.generic.textContent, textRenderData);
            }
            // Draw text input content (1 draw call)
            else if (node->type == NU_TYPE_INPUT) {
                NU_ClipBounds clip = {0};
                clip.top = node->public.generic.y;
                clip.left = node->public.generic.x + node->public.generic.borderLeft + node->public.generic.padLeft;
                clip.right = node->public.generic.x + node->public.generic.width - node->public.generic.borderRight - node->public.generic.padRight;
                clip.bottom = node->public.generic.y + node->public.generic.height + 1000;
                NU_DrawInputNodeContent(node, z, winW, winH, &clip);
            }
            // Construct image render data
            if (node->special.image.imageHandle != 0 && node->type != NU_TYPE_CANVAS && node->type != NU_TYPE_INPUT) {
                ImageRenderData renderData;
                renderData.x = node->public.generic.x + node->public.generic.borderLeft + node->public.generic.padLeft;
                renderData.y = node->public.generic.y + node->public.generic.borderTop + node->public.generic.padTop;
                renderData.z = z + 0.75f;
                renderData.w = node->public.generic.width - node->public.generic.borderLeft - node->public.generic.borderRight - node->public.generic.padLeft - node->public.generic.padRight;
                renderData.h = node->public.generic.height - node->public.generic.borderTop - node->public.generic.borderBottom - node->public.generic.padTop - node->public.generic.padBottom;
                renderData.scissorTop = 0.0f;
                renderData.scissorBottom = 1000000.0f;
                renderData.scissorLeft = 0.0f;
                renderData.scissorRight = 1000000.0f;
                renderData.filter = node->special.image.filter;
                renderData.repeat = node->special.image.repeat;
                ImageResourceManager_AddImageRenderData(
                    &GUI.imageResourceManager,
                    node->special.image.imageHandle,
                    &renderData
                );
            }
        }

        // 2. Draw all unclipped border rects (1 draw call)
        Draw_SDF_Border_Rects(GUI.borderRectRenderDataArray, winW, winH); Array_Clear(&GUI.borderRectRenderDataArray);

        // 3. Draw all unclipped text (1 draw call per font)
        for (u32 t=0; t<GUI.stylesheet.fonts.size; t++) {
            NU_Font* font = Stylesheet_Get_Font(&GUI.stylesheet, t);
            Array* textRenderData = Array_Get(&GUI.perFontTextRenderData, t);
            NU_Draw_Text(*textRenderData, font, winW, winH, 0, 0);
        }

        // 4. Draw clipped node border rects + images + text + text input
        for (u32 n=0; n<drawList->clippedDrawNodes.size; n++) {
            Node* node = *(Node**)Array_Get(&drawList->clippedDrawNodes, n);
            float z = (float)(node->layer) + 32.0f * NodeStatePosAbsolute(node);

            // Draw border rect (1 draw call)
            NU_ClipBounds* clip = (NU_ClipBounds*)Hashmap_Get(&GUI.winManager.clipMap, node->clippedAncestor);
            Add_NodeRectRenderData(node, z, clip->top, clip->bottom, clip->left, clip->right, &GUI.borderRectRenderDataArray);
            Draw_SDF_Border_Rects(GUI.borderRectRenderDataArray, winW, winH); Array_Clear(&GUI.borderRectRenderDataArray);

            // Draw text content (1 draw call)
            if (node->public.generic.textContent != NULL) {
                NU_Font* font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontIdx);
                Array* textRenderData = Array_Get(&GUI.perFontTextRenderData, node->fontIdx);
                Array_Clear(textRenderData);
                NU_AddTextMesh(node, z + 0.5f, node->public.generic.textContent, textRenderData);
                NU_Draw_Text(*textRenderData, font, winW, winH, 0, 0);
            }
            // Draw text input (1 draw call)
            else if (node->type == NU_TYPE_INPUT) {
                InputText* inputText = Container_Get(&GUI.textInputs, node->special.input.textInputHandle);
                if (inputText->numBytes > 0) {
                    NU_ClipBounds innerClip = *clip;
                    innerClip.left += node->public.generic.borderLeft + node->public.generic.padLeft;
                    innerClip.right -= node->public.generic.borderRight + node->public.generic.padRight;
                    NU_DrawInputNodeContent(node, z, winW, winH, &innerClip);
                }
            }
            // Construct image render data
            if (node->special.image.imageHandle != 0 && node->type != NU_TYPE_CANVAS && node->type != NU_TYPE_INPUT) {
                ImageRenderData renderData;
                renderData.x = node->public.generic.x + node->public.generic.borderLeft + node->public.generic.padLeft;
                renderData.y = node->public.generic.y + node->public.generic.borderTop + node->public.generic.padTop;
                renderData.z = z + 0.75f;
                renderData.w = node->public.generic.width - node->public.generic.borderLeft - node->public.generic.borderRight - node->public.generic.padLeft - node->public.generic.padRight;
                renderData.h = node->public.generic.height - node->public.generic.borderTop - node->public.generic.borderBottom - node->public.generic.padTop - node->public.generic.padBottom;
                renderData.scissorTop = clip->top;
                renderData.scissorBottom = clip->bottom;
                renderData.scissorLeft = clip->left;
                renderData.scissorRight = clip->right;
                renderData.filter = node->special.image.filter;
                renderData.repeat = node->special.image.repeat;
                ImageResourceManager_AddImageRenderData(
                    &GUI.imageResourceManager,
                    node->special.image.imageHandle,
                    &renderData
                );
            }
        }

        // 5. Draw all canvas content
        for (u32 n=0; n<drawList->canvasNodes.size; n++) {
            Node* node = *(Node**)Array_Get(&drawList->canvasNodes, n);
            float z = (float)(node->layer) + 32.0f * NodeStatePosAbsolute(node);
            NU_DrawCanvasContent(node, winW, winH, NULL);
        }

        // 6. Draw all images (1 draw call per atlas | 1 draw call per standalone image)
        for (int i=0; i<GUI.imageResourceManager.atlases.size; i++) {
            Atlas* atlas = Array_Get(&GUI.imageResourceManager.atlases, i);
            NU_Draw_Images(atlas->renderDataArray, atlas->glImageHandle, winW, winH);
        }
        for (int i=0; i<GUI.imageResourceManager.standaloneImageRenderDatas.size; i++) {
            StandaloneImageRenderData* sRenderData = Array_Get(&GUI.imageResourceManager.standaloneImageRenderDatas, i);
            NU_Draw_Image(&sRenderData->renderData, winW, winH, sRenderData->glImageHandle);
        }

        SDL_GL_SwapWindow(window);
    }

    GUI.awaiting_redraw = false;
}
