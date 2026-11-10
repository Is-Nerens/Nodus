#pragma once

#define ABSOLUTE_Z_LAYER 4096

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

static inline float ComputeZ(uint32 layer, uint8 sublayer)
{
    uint32 p = layer * 8u + (uint32)(sublayer & 7u);   // ∈ [0, 2048)
    uint32 bits = (126u << 23) | p;                 // exponent 126 => 2^-1, mantissa = p
    float z;
    memcpy(&z, &bits, sizeof z);
    return z;
}

inline int NodeOutsideWindow(Node* node, int winW, int winH)
{
    float right  = node->public.generic.x + node->public.generic.width;
    float bottom = node->public.generic.y + node->public.generic.height;
    return (right < 0 || bottom < 0 || node->public.generic.x > winW || node->public.generic.y > winH);
}

inline int NodeOutsideBounds(Node* node, float left, float right, float top, float bottom)
{
    return (node->public.generic.y + node->public.generic.height < top) |
           (node->public.generic.y > bottom) |
           (node->public.generic.x + node->public.generic.width < left) |
           (node->public.generic.x > right);
}

void NU_AddNodeTextToRenderDataArray(Node* node, NU_ClipBounds* clip, float z, Array* textRenderData)
{
    // Compute node inner dimensions
    float inner_width  = NODE_INNER_WIDTH(node);
    float inner_height = NODE_INNER_HEIGHT(node);
    float remaining_w = inner_width  - node->public.generic.contentWidth;
    float remaining_h = inner_height - node->public.generic.contentHeight;

    // Compute text position
    float x_align_offset = remaining_w * 0.5f * (float)node->horizontalTextAlignment;
    float y_align_offset = remaining_h * 0.5f * (float)node->verticalTextAlignment;
    float textPosX = floorf(node->public.generic.x + node->public.generic.borderLeft + node->public.generic.padLeft + x_align_offset);
    float textPosY = floorf(node->public.generic.y + node->public.generic.borderTop  + node->public.generic.padTop + y_align_offset);

    // Add glyph render data
    Font* node_font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontIdx);
    NuColor color = {
        node->public.generic.textR,
        node->public.generic.textG,
        node->public.generic.textB,
        255
    };
    NU_Generate_Text_Renderdata(
        textRenderData,
        node_font,
        clip,
        node->public.generic.textContent,
        textPosX,
        textPosY,
        z,
        color,
        inner_width
    );
}

void NU_DrawInputNodeContent(Node* node, float z, float winWidth, float winHeight, NU_ClipBounds* clip)
{
    // Get font and inout text datastructure
    Font* node_font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontIdx);
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
        //NU_ConstructInputHighlightMesh(node, z + 0.25f, inputText, &highlightVertices, &highlightIndices);
        // Draw_Clipped_Vertex_RGB_List(
        //     &highlightVertices, &highlightIndices,
        //     winWidth, winHeight, 0, 0,
        //     clip->top, clip->bottom,
        //     clip->left, clip->right + 1
        // );
        // Vertex_RGB_List_Free(&highlightVertices);
        // Index_List_Free(&highlightIndices);
    }

    // Draw text
    float textPosX = floorf(node->public.generic.x + node->public.generic.borderLeft + node->public.generic.padLeft + inputText->textOffset);
    float textPosY = floorf(node->public.generic.y + node->public.generic.borderTop  + node->public.generic.padTop);
    Array textRenderData;
    Array_Init(&textRenderData, sizeof(GlyphRenderData), 256);
    NuColor color = {
        node->public.generic.textR,
        node->public.generic.textG,
        node->public.generic.textB,
        255
    };
    NU_Generate_Text_Renderdata(
        &textRenderData,
        node_font,
        NULL,
        inputText->buffer,
        textPosX,
        textPosY,
        z + 0.5f,
        color,
        -1.0f
    );
    //NU_Draw_Clipped_Text(textRenderData, node_font, winWidth, winHeight, 0, 0, clip->top, clip->bottom, clip->left, clip->right);
    Array_Free(&textRenderData);

    // Draw cursor (if input is focused)
    if (node == GUI.focused_node && !InputText_IsHighlighting(inputText))
    {
        Vertex_RGB_List cursorVertices; Vertex_RGB_List_Init(&cursorVertices, 4);
        Index_List cursorIndices; Index_List_Init(&cursorIndices, 6);
        //NU_ConstructInputCursorMesh(node, z + 0.5f, inputText, &cursorVertices, &cursorIndices);
        // Draw_Clipped_Vertex_RGB_List(
        //     &cursorVertices, &cursorIndices,
        //     winWidth, winHeight, 0, 0,
        //     clip->top, clip->bottom,
        //     clip->left, clip->right + 1
        // );
        // Vertex_RGB_List_Free(&cursorVertices);
        // Index_List_Free(&cursorIndices);
    }
}

void Add_NodeRectRenderData(
    Node* node, float z,
    float scissorTop, float scissorBottom, float scissorLeft, float scissorRight,
    Array* borderRects
)
{
    NuNodeGenericAccess* n = &node->public.generic;

    uint8 alpha = 255;
    if (node->layoutFlags & HIDE_BACKGROUND) alpha = 0;

    BorderRectRenderData* renderData = Array_PushEmpty(borderRects);
    renderData->x = floorf(n->x);
    renderData->y = floorf(n->y);
    renderData->z = z;
    renderData->w = floorf(n->x + n->width)  - renderData->x;
    renderData->h = floorf(n->y + n->height) - renderData->y;

    // Constrain border radii against floored dimensions
    float borderRadiusTl = n->borderRadiusTl;
    float borderRadiusTr = n->borderRadiusTr;
    float borderRadiusBl = n->borderRadiusBl;
    float borderRadiusBr = n->borderRadiusBr;
    float topRadiiSum    = borderRadiusTl + borderRadiusTr;
    float bottomRadiiSum = borderRadiusBl + borderRadiusBr;
    float leftRadiiSum   = borderRadiusTl + borderRadiusBl;
    float rightRadiiSum  = borderRadiusTr + borderRadiusBr;
    if (topRadiiSum    > renderData->w) { float scale = renderData->w / topRadiiSum;    borderRadiusTl *= scale; borderRadiusTr *= scale; }
    if (bottomRadiiSum > renderData->w) { float scale = renderData->w / bottomRadiiSum; borderRadiusBl *= scale; borderRadiusBr *= scale; }
    if (leftRadiiSum   > renderData->h) { float scale = renderData->h / leftRadiiSum;   borderRadiusTl *= scale; borderRadiusBl *= scale; }
    if (rightRadiiSum  > renderData->h) { float scale = renderData->h / rightRadiiSum;  borderRadiusTr *= scale; borderRadiusBr *= scale; }

    NuColor backgroundColor = {
        n->backgroundR,
        n->backgroundG,
        n->backgroundB,
        alpha
    };
    NuColor borderColor = {
        n->borderR,
        n->borderG,
        n->borderB,
        alpha
    };
    renderData->backgroundRGBA = PackColorIntoUint32(backgroundColor);
    renderData->borderRGBA = PackColorIntoUint32(borderColor);
    renderData->radiusTl = borderRadiusTl;
    renderData->radiusTr = borderRadiusTr;
    renderData->radiusBl = borderRadiusBl;
    renderData->radiusBr = borderRadiusBr;
    renderData->borderTop = n->borderTop;
    renderData->borderBottom = n->borderBottom;
    renderData->borderLeft = n->borderLeft;
    renderData->borderRight = n->borderRight;
    renderData->scissorTop = scissorTop;
    renderData->scissorBottom = scissorBottom;
    renderData->scissorLeft = scissorLeft;
    renderData->scissorRight = scissorRight;
}

void Draw_CanvasContent(Node* node)
{

}

void Add_ScrollbarRenderData(
    Node* node, float z,
    Stylesheet_Scrollbar_Style* scrollbarStyle,
    Array* borderRects
)
{
    NuNodeGenericAccess* n = &node->public.generic;

    // --------------------------------------
    // --- Compute constrained dimensions ---
    // --------------------------------------
    float thumbWidth = (float)scrollbarStyle->width - (float)scrollbarStyle->trackPadLeft - (float)scrollbarStyle->trackPadRight;

    // Constrain thumb width by thumb border
    if (thumbWidth < scrollbarStyle->thumbBorderLeft + scrollbarStyle->thumbBorderRight) {
        thumbWidth = scrollbarStyle->thumbBorderLeft + scrollbarStyle->thumbBorderRight;
    }

    // Ensure absolute minimum thumb width of 2px
    if (thumbWidth < 2) thumbWidth = 2;

    // Compute thumb-constrained track width
    float trackWidth = thumbWidth + (float)scrollbarStyle->trackPadLeft + (float)scrollbarStyle->trackPadRight;

    // Compute track height
    float trackHeight = n->height - n->borderTop - n->borderBottom;
    float usableTrackHeight = trackHeight - scrollbarStyle->trackPadTop - scrollbarStyle->trackPadBottom;

    // Compute track pos
    float trackX = n->x + n->width - n->borderRight - trackWidth;
    float trackY = n->y + n->borderTop;

    // Compute scroll transform values
    float scrollContentHeight = n->contentHeight;
    float scrollViewHeight = usableTrackHeight - n->padTop - n->padBottom;
    float scrollScaleFactor = scrollViewHeight / scrollContentHeight;

    // Compute thumb pos and constrained height
    float thumbHeight = fmaxf(scrollViewHeight / n->contentHeight * usableTrackHeight, scrollbarStyle->thumbMinSize);
    float scrollTravel = usableTrackHeight - thumbHeight;
    float thumbY = trackY + scrollbarStyle->trackPadTop + n->scrollV * scrollTravel;
    float thumbX = trackX + scrollbarStyle->trackPadLeft;

    // Constrain thumb border radii based on thumb width and height
    float thumbBorderTop = scrollbarStyle->thumbBorderTop;
    float thumbBorderBottom = scrollbarStyle->thumbBorderBottom;
    float thumbBorderLeft = scrollbarStyle->thumbBorderLeft;
    float thumbBorderRight = scrollbarStyle->thumbBorderRight;
    float thumbRadiusTl = scrollbarStyle->thumbBorderRadiusTl;
    float thumbRadiusTr = scrollbarStyle->thumbBorderRadiusTr;
    float thumbRadiusBl = scrollbarStyle->thumbBorderRadiusBl;
    float thumbRadiusBr = scrollbarStyle->thumbBorderRadiusBr;
    float thumbLeftRadiiSum   = thumbRadiusTl + thumbRadiusBl;
    float thumbRightRadiiSum  = thumbRadiusTr + thumbRadiusBr;
    float thumbTopRadiiSum    = thumbRadiusTl + thumbRadiusTr;
    float thumbBottomRadiiSum = thumbRadiusBl + thumbRadiusBr;
    if (thumbLeftRadiiSum   > thumbHeight) { float scale = thumbHeight / thumbLeftRadiiSum;   thumbRadiusTl *= scale; thumbRadiusBl *= scale; }
    if (thumbRightRadiiSum  > thumbHeight) { float scale = thumbHeight / thumbRightRadiiSum;  thumbRadiusTr *= scale; thumbRadiusBr *= scale; }
    if (thumbTopRadiiSum    > thumbWidth ) { float scale = thumbWidth  / thumbTopRadiiSum;    thumbRadiusTl *= scale; thumbRadiusTr *= scale; }
    if (thumbBottomRadiiSum > thumbWidth ) { float scale = thumbWidth  / thumbBottomRadiiSum; thumbRadiusBl *= scale; thumbRadiusBr *= scale; }

    // Constrain track border radii based on track width and height
    float trackBorderTop    = scrollbarStyle->trackBorderTop;
    float trackBorderBottom = scrollbarStyle->trackBorderBottom;
    float trackBorderLeft   = scrollbarStyle->trackBorderLeft;
    float trackBorderRight  = scrollbarStyle->trackBorderRight;
    float trackRadiusTl = scrollbarStyle->trackBorderRadiusTl;
    float trackRadiusTr = scrollbarStyle->trackBorderRadiusTr;
    float trackRadiusBl = scrollbarStyle->trackBorderRadiusBl;
    float trackRadiusBr = scrollbarStyle->trackBorderRadiusBr;
    float trackLeftRadiiSum   = trackRadiusTl + trackRadiusBl;
    float trackRightRadiiSum  = trackRadiusTr + trackRadiusBr;
    float trackTopRadiiSum    = trackRadiusTl + trackRadiusTr;
    float trackBottomRadiiSum = trackRadiusBl + trackRadiusBr;
    if (trackLeftRadiiSum   > trackHeight) { float scale = trackHeight / trackLeftRadiiSum;   trackRadiusTl *= scale; trackRadiusBl *= scale; }
    if (trackRightRadiiSum  > trackHeight) { float scale = trackHeight / trackRightRadiiSum;  trackRadiusTr *= scale; trackRadiusBr *= scale; }
    if (trackTopRadiiSum    > trackWidth ) { float scale = trackWidth  / trackTopRadiiSum;    trackRadiusTl *= scale; trackRadiusTr *= scale; }
    if (trackBottomRadiiSum > trackWidth ) { float scale = trackWidth  / trackBottomRadiiSum; trackRadiusBl *= scale; trackRadiusBr *= scale; }


    // --------------------------
    // --- Construct geometry ---
    // --------------------------

    // Track
    BorderRectRenderData* trackRData = Array_PushEmpty(borderRects);
    trackRData->x = floorf(trackX);
    trackRData->y = floorf(trackY);
    trackRData->z = z;
    trackRData->w = floorf(trackX + trackWidth) - trackRData->x;
    trackRData->h = floorf(trackY + trackHeight) - trackRData->y;
    trackRData->radiusTl = trackRadiusTl;
    trackRData->radiusTr = trackRadiusTr;
    trackRData->radiusBl = trackRadiusBl;
    trackRData->radiusBr = trackRadiusBr;
    NuColor trackCol = {
        scrollbarStyle->trackBackgroundR,
        scrollbarStyle->trackBackgroundG,
        scrollbarStyle->trackBackgroundB,
        255
    };
    NuColor borderCol = {
        scrollbarStyle->trackBorderR,
        scrollbarStyle->trackBorderG,
        scrollbarStyle->trackBorderB,
        255
    };
    trackRData->backgroundRGBA = PackColorIntoUint32(trackCol);
    trackRData->borderRGBA = PackColorIntoUint32(borderCol);
    trackRData->borderTop = scrollbarStyle->trackBorderTop;
    trackRData->borderBottom = scrollbarStyle->trackBorderBottom;
    trackRData->borderLeft = scrollbarStyle->trackBorderLeft;
    trackRData->borderRight = scrollbarStyle->trackBorderRight;
    trackRData->scissorTop = 0.0f;
    trackRData->scissorBottom = 10000000.0f;
    trackRData->scissorLeft = 0.0f;
    trackRData->scissorRight = 10000000.0f;

    // Thumb
    BorderRectRenderData* thumbRData = Array_PushEmpty(borderRects);
    thumbRData->x = floorf(thumbX);
    thumbRData->y = floorf(thumbY);
    thumbRData->z = z;
    thumbRData->w = floorf(thumbX + thumbWidth) - thumbRData->x;
    thumbRData->h = floorf(thumbY + thumbHeight) - thumbRData->y;
    thumbRData->radiusTl = thumbRadiusTl;
    thumbRData->radiusTr = thumbRadiusTr;
    thumbRData->radiusBl = thumbRadiusBl;
    thumbRData->radiusBr = thumbRadiusBr;
    NuColor thumbBgCol = {
        scrollbarStyle->thumbBackgroundR,
        scrollbarStyle->thumbBackgroundG,
        scrollbarStyle->thumbBackgroundB,
        255
    };
    NuColor thumbBorderCol = {
        scrollbarStyle->thumbBorderR,
        scrollbarStyle->thumbBorderG,
        scrollbarStyle->thumbBorderB,
        255
    };
    thumbRData->backgroundRGBA = PackColorIntoUint32(thumbBgCol);
    thumbRData->borderRGBA = PackColorIntoUint32(thumbBorderCol);
    thumbRData->borderTop = scrollbarStyle->thumbBorderTop;
    thumbRData->borderBottom = scrollbarStyle->thumbBorderBottom;
    thumbRData->borderLeft = scrollbarStyle->thumbBorderLeft;
    thumbRData->borderRight = scrollbarStyle->thumbBorderRight;
    thumbRData->scissorTop = 0.0f;
    thumbRData->scissorBottom = 10000000.0f;
    thumbRData->scissorLeft = 0.0f;
    thumbRData->scissorRight = 10000000.0f;
}

void NU_ConstructInputCursorMesh(
    Node* node, float z,
    InputText* inputText,
    Vertex_RGB_List* vertices,
    Index_List* indices)
{
    NuNodeGenericAccess* n = &node->public.generic;
    float x = n->x + n->borderLeft + n->padLeft + inputText->cursorOffset;
    float y = n->y + n->borderTop + n->padTop;
    float innerHeight = n->height - n->borderTop  - n->borderBottom - n->padTop - n->padBottom;

    NuColor color = { 255, 255, 255, 255 };
    float32 packedColor = PackColorIntoFloat32(color);
    vertices->array[0] = (vertex_rgb){ x, y, z, packedColor }; // top left
    vertices->array[1] = (vertex_rgb){ x + 1, y, z, packedColor }; // top right
    vertices->array[2] = (vertex_rgb){ x, y + innerHeight - 1, z, packedColor }; // bottom left
    vertices->array[3] = (vertex_rgb){ x + 1, y + innerHeight - 1, z, packedColor }; // bottom right

    // indices
    uint32* indices_write = indices->array;
    *indices_write++ = 0;
    *indices_write++ = 1;
    *indices_write++ = 2;
    *indices_write++ = 1;
    *indices_write++ = 2;
    *indices_write++ = 3;

    vertices->size = 4;
    indices->size = 6;
}

void NU_ConstructInputHighlightMesh(
    Node* node, float z,
    InputText* inputText,
    Vertex_RGB_List* vertices,
    Index_List* indices)
{
    NuNodeGenericAccess* n = &node->public.generic;

    float cursorX = n->x + n->borderLeft + n->padLeft + inputText->cursorOffset;
    float highlightX = n->x + n->borderLeft + n->padLeft + inputText->highlightOffset;
    float x = highlightX; if (cursorX < x) x = cursorX;
    float y = n->y + n->borderTop + n->padTop;
    float width = fabs(cursorX - highlightX);
    float height = n->height - n->borderTop - n->borderBottom - n->padTop - n->padBottom;

    NuColor highlightColor = { 66, 137, 244, 255 };
    float32 packedHighlightColor = PackColorIntoFloat32(highlightColor);

    vertices->array[0] = (vertex_rgb){ x, y, z, packedHighlightColor }; // top left
    vertices->array[1] = (vertex_rgb){ x + width, y, z, packedHighlightColor }; // top right
    vertices->array[2] = (vertex_rgb){ x, y + height, z, packedHighlightColor }; // bottom left
    vertices->array[3] = (vertex_rgb){ x + width, y + height, z, packedHighlightColor }; // bottom right

    // indices
    uint32* indices_write = indices->array;
    *indices_write++ = 0;
    *indices_write++ = 1;
    *indices_write++ = 2;
    *indices_write++ = 1;
    *indices_write++ = 2;
    *indices_write++ = 3;

    vertices->size = 4;
    indices->size = 6;
}





void NU_GenerateDrawlists()
{
    // Clear drawlists
    for (int i=0; i<GUI.displayManager.windows.size; i++) {
        Window* win = Container_GetAt(&GUI.displayManager.windows, i);
        Array_Clear(&win->drawlist.drawNodes);
        Array_Clear(&win->drawlist.clippedDrawNodes);
        Array_Clear(&win->drawlist.canvasNodes);
    }

    // Clear hashmaps
    Hashmap_Clear(&GUI.displayManager.clipMap);
    Array_Clear(&GUI.displayManager.absoluteRootNodes);

    // Add root to drawlist
    Node* root = GUI.tree.root;
    SetNodeDrawlist_Draw(&GUI.displayManager, root);

    // Traverse the tree
    BreadthFirstSearch* bfs = &GUI.bfs;
    BreadthFirstSearch_Reset(bfs, root);
    Node* node;
    while(BreadthFirstSearch_Next(bfs, &node)) {

        // Precompute node inner rect
        float nodeInnerX, nodeInnerY, nodeInnerWidth, nodeInnerHeight = 0;
        if (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL) {
            nodeInnerY = node->public.generic.y + node->public.generic.borderTop + node->public.generic.padTop;
            nodeInnerHeight = NODE_INNER_HEIGHT(node);

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
            nodeInnerWidth = NODE_INNER_WIDTH(node);
        }

        // cache window dimensions
        int winW, winH;
        SDL_Window* window = DisplayManager_GetNodeSDLWindow(&GUI.displayManager, node);
        SDL_GetWindowSizeInPixels(window, &winW, &winH);

        // iterate over children
        Node* child = node->firstChild;
        while(child)
        {
            // Hide child if inherits parent's visibility OR should hide
            bool inheritHiddenness = child->type != NU_TYPE_WINDOW && NodeStateHidden(node);
            bool makeHidden = NodeStateHidden(child) || NodeOutsideWindow(child, winW, winH);
            if (inheritHiddenness || makeHidden) {
                child->stateFlags |= STATE_FLAG_HIDDEN;
                child = child->nextSibling; continue;
            }

            // add child to list of root absolute nodes
            if (child->layoutFlags & POSITION_ABSOLUTE) {
                Array_Push(&GUI.displayManager.absoluteRootNodes, &child);
            }

            // if overflowed parent's bounds
            if (child->type != NU_TYPE_WINDOW && child->type != NU_TYPE_THEAD && (node->layoutFlags & OVERFLOW_VERTICAL_SCROLL)) {
                NodeOverlap verticalOverlap = NodeVerticalOverlapState(child, nodeInnerY, nodeInnerHeight);

                // child not inside parent -> hide in this draw pass
                if (verticalOverlap == NODE_OVERLAP_NONE) {
                    child->stateFlags |= STATE_FLAG_HIDDEN;
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
                        NU_ClipBounds* parent_clip = Hashmap_Get(&GUI.displayManager.clipMap, &node->clippedAncestor);
                        clip.top = fmaxf(clip.top, parent_clip->top);
                        clip.bottom = fminf(clip.bottom, parent_clip->bottom);
                    }

                    // add clipping to hashmap
                    child->clippedAncestor = child; // Set clip root to self
                    Hashmap_Set(&GUI.displayManager.clipMap, &child->clippedAncestor, &clip);

                    // append node to correct window clipped node list
                    SetNodeDrawlist_Clipped(&GUI.displayManager, child);
                    if (child->type == NU_TYPE_CANVAS) SetNodeDrawlist_Canvas(&GUI.displayManager, child);
                }

                // child does not overlap parent -> child contained squarely within scroll parent
                else {
                    SetNodeDrawlist_Draw(&GUI.displayManager, child);
                    if (child->type == NU_TYPE_CANVAS) SetNodeDrawlist_Canvas(&GUI.displayManager, child);
                }
            }

            // Otherwise if only the parent is clipped -> child inherits clip from parent
            else if (node->clippedAncestor) {
                NU_ClipBounds* clip = Hashmap_Get(&GUI.displayManager.clipMap, &node->clippedAncestor);
                // Totally ignore children outside of inherited clip bounds
                if (NodeOutsideBounds(child, clip->left, clip->right, clip->top, clip->bottom)) {
                    child->stateFlags |= STATE_FLAG_HIDDEN;
                }
                else {
                    child->clippedAncestor = node->clippedAncestor;
                    SetNodeDrawlist_Clipped(&GUI.displayManager, child);
                    if (child->type == NU_TYPE_CANVAS) SetNodeDrawlist_Canvas(&GUI.displayManager, child);
                }
            }
            // Else neither parent nor child are clipped
            else {
                SetNodeDrawlist_Draw(&GUI.displayManager, child);
                if (child->type == NU_TYPE_CANVAS) SetNodeDrawlist_Canvas(&GUI.displayManager, child);
            }

            // move to the next child
            child = child->nextSibling;
        }
    }
}

void NU_Draw()
{
    NU_GenerateDrawlists();

    // Clear GPU arena allocator
    ArenaAllocatorGPU_Clear(&GUI.displayManager.gpuArenaAlloc);

    // Re-upload font atlases as needed
    for (uint32 t=0; t<Array_Length(&GUI.stylesheet.fonts); t++) {
        Font* font = Stylesheet_Get_Font(&GUI.stylesheet, t);
        Font_UpdateGPU(
            font,
            GUI.displayManager.device,
            GUI.displayManager.physicalDevice,
            GUI.displayManager.commandPool,
            GUI.displayManager.graphicsQueue,
            GUI.displayManager.fontPool,
            GUI.displayManager.fontSetLayout
        );
    }

    Node* focusedInputNode = NULL;

    // For each window
    for (uint32 i=0; i<GUI.displayManager.windows.size; i++)
    {
        // Get the window
        Window* win = Container_GetAt(&GUI.displayManager.windows, i);

        // Prepare the frame
        if (!Window_PrepareFrame(win, &GUI.displayManager)) { // Failure -> skip frame (should not happen)
            continue;
        }

        // Clear border rect render data array
        Array_Clear(&GUI.borderRectsRenderData);

        // Clear text render data arrays
        for (int j=0; j<Array_Length(&GUI.perFontTextRenderData); j++) {
            Array* textRenderData = Array_Get(&GUI.perFontTextRenderData, j);
            Array_Clear(textRenderData);
        }

        // Clear clipped text render data arrays
        for (int j=0; j<Array_Length(&GUI.perFontClippedTextRenderData); j++) {
            Array* clippedTextRenderData = Array_Get(&GUI.perFontClippedTextRenderData, j);
            Array_Clear(clippedTextRenderData);
        }

        // Lazily append new image render groups
        int renderGroupImageCapacity = Array_Length(&GUI.imageRenderGroups) * 8;
        int numGroupsToAdd = 0;
        if (renderGroupImageCapacity < Array_Length(&GUI.imageResourceManager.imageResources)) {
            int imageCapacityShortfall = Array_Length(&GUI.imageResourceManager.imageResources) - renderGroupImageCapacity;
            numGroupsToAdd = (imageCapacityShortfall + 7) / 8;
            for (int j=0; j<numGroupsToAdd; j++) {
                ImageRenderGroup* newGroup = Array_PushEmpty(&GUI.imageRenderGroups);
                Array_Init(&newGroup->imagesRenderData, sizeof(ImageRenderData), 16);
            }
        }

        // Clear image group render data arrays <[Performance]> does not clear groups that were just added
        for (int j=0; j<Array_Length(&GUI.imageRenderGroups) - numGroupsToAdd; j++) {
            ImageRenderGroup* imageRenderGroup = Array_Get(&GUI.imageRenderGroups, j);
            Array_Clear(&imageRenderGroup->imagesRenderData);
        }

        // Get window dimensions
        SDL_Window* window = win->sdlWin;
        NuWindowDrawlist* drawList = &win->drawlist;
        int winW_int, winH_int;
        SDL_GetWindowSizeInPixels(window, &winW_int, &winH_int);
        float winW = (float)winW_int;
        float winH = (float)winH_int;

        // Construct render data for non-clipped nodes
        for (uint32 n=0; n<drawList->drawNodes.size; n++)
        {
            Node* node = *(Node**)Array_Get(&drawList->drawNodes, n);

            // Border rect render data
            Add_NodeRectRenderData(node, ComputeZ(node->layer + ABSOLUTE_Z_LAYER * NodeStatePosAbsolute(node), 0), 0.0f, winH, 0.0f, winW, &GUI.borderRectsRenderData);
            if ((node->layoutFlags & OVERFLOW_VERTICAL_SCROLL) && node->public.generic.contentHeight > NODE_INNER_HEIGHT(node)) {
                Add_ScrollbarRenderData(node, ComputeZ(node->layer + ABSOLUTE_Z_LAYER * NodeStatePosAbsolute(node), 1), &GUI.stylesheet.scrollbarStyle, &GUI.borderRectsRenderData);
            }

            // Text content render data
            if (node->public.generic.textContent != NULL) {
                Array* textRenderData = Array_Get(&GUI.perFontTextRenderData, node->fontIdx);
                NU_AddNodeTextToRenderDataArray(node, NULL, ComputeZ(node->layer + ABSOLUTE_Z_LAYER * NodeStatePosAbsolute(node), 1), textRenderData);
            }

            // Text input render data
            else if (node->type == NU_TYPE_INPUT) {
                NU_ClipBounds clip = {0};
                clip.top = node->public.generic.y;
                clip.left = node->public.generic.x + node->public.generic.borderLeft + node->public.generic.padLeft;
                clip.right = node->public.generic.x + node->public.generic.width - node->public.generic.borderRight - node->public.generic.padRight;
                clip.bottom = node->public.generic.y + node->public.generic.height + 1000;
                //NU_DrawInputNodeContent(node, z, winW, winH, &clip);
            }

            // Image render data
            if (node->special.image.imageHandle != 0 && node->type == NU_TYPE_IMAGE)
            {
                // Get image render group
                int imageHandle = node->special.image.imageHandle;
                ImageView* imageView = ImageResourceManager_GetImageView(&GUI.imageResourceManager, imageHandle);
                int groupIndex = imageView->imageResourceIndex / 8;
                uint8 groupImageIndex = imageView->imageResourceIndex % 8;
                ImageRenderGroup* imageRenderGroup = Array_Get(&GUI.imageRenderGroups, groupIndex);

                // Create image render data in group
                ImageRenderData* renderData = Array_PushEmpty(&imageRenderGroup->imagesRenderData);
                renderData->x = node->public.generic.x + node->public.generic.borderLeft + node->public.generic.padLeft;
                renderData->y = node->public.generic.y + node->public.generic.borderTop + node->public.generic.padTop;
                renderData->z = ComputeZ(node->layer + ABSOLUTE_Z_LAYER * NodeStatePosAbsolute(node), 2);
                renderData->w = node->public.generic.width - NODE_BORDER_PAD_WIDTH(node);
                renderData->h = node->public.generic.height - NODE_BORDER_PAD_HEIGHT(node);
                renderData->u0 = imageView->u0;
                renderData->v0 = imageView->v0;
                renderData->u1 = imageView->u1;
                renderData->v1 = imageView->v1;
                renderData->clipTop = 0;
                renderData->clipBottom = 60000;
                renderData->clipLeft = 0;
                renderData->clipRight = 60000;
                renderData->clipRadiusTopLeft = 0;
                renderData->clipRadiusTopRight = 0;
                renderData->clipRadiusBottomLeft = 0;
                renderData->clipRadiusBottomRight = 0;
                renderData->textureIndex = groupImageIndex;
                renderData->filter = node->special.image.filter;
                renderData->repeat = node->special.image.repeat;
            }
        }

        // Construct render data for clipped nodes
        for (uint32 n=0; n<drawList->clippedDrawNodes.size; n++) {
            Node* node = *(Node**)Array_Get(&drawList->clippedDrawNodes, n);

            // Border rect render data
            NU_ClipBounds* clip = (NU_ClipBounds*)Hashmap_Get(&GUI.displayManager.clipMap, &node->clippedAncestor);
            Add_NodeRectRenderData(node, ComputeZ(node->layer + ABSOLUTE_Z_LAYER * NodeStatePosAbsolute(node), 0), clip->top, clip->bottom, clip->left, clip->right, &GUI.borderRectsRenderData);

            // Text content render data
            if (node->public.generic.textContent != NULL) {
                Font* font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontIdx);
                Array* textRenderData = Array_Get(&GUI.perFontClippedTextRenderData, node->fontIdx);
                NU_AddNodeTextToRenderDataArray(node, clip, ComputeZ(node->layer + ABSOLUTE_Z_LAYER * NodeStatePosAbsolute(node), 1), textRenderData);
            }

            // Text input render data
            else if (node->type == NU_TYPE_INPUT) {
                InputText* inputText = Container_Get(&GUI.textInputs, node->special.input.textInputHandle);
                if (inputText->numBytes > 0) {
                    NU_ClipBounds innerClip = *clip;
                    innerClip.left += node->public.generic.borderLeft + node->public.generic.padLeft;
                    innerClip.right -= node->public.generic.borderRight + node->public.generic.padRight;
                    //NU_DrawInputNodeContent(node, z, winW, winH, &innerClip);
                }
            }

            // Image render data
            if (node->special.image.imageHandle != 0 && node->type == NU_TYPE_IMAGE)
            {
                // Get image render group
                int imageHandle = node->special.image.imageHandle;
                ImageView* imageView = ImageResourceManager_GetImageView(&GUI.imageResourceManager, imageHandle);
                int groupIndex = imageView->imageResourceIndex / 8;
                uint8 groupImageIndex = imageView->imageResourceIndex % 8;
                ImageRenderGroup* imageRenderGroup = Array_Get(&GUI.imageRenderGroups, groupIndex);

                // Create image render data in group
                ImageRenderData* renderData = Array_PushEmpty(&imageRenderGroup->imagesRenderData);
                renderData->x = node->public.generic.x + node->public.generic.borderLeft + node->public.generic.padLeft;
                renderData->y = node->public.generic.y + node->public.generic.borderTop + node->public.generic.padTop;
                renderData->z = ComputeZ(node->layer + ABSOLUTE_Z_LAYER * NodeStatePosAbsolute(node), 2);
                renderData->w = node->public.generic.width - NODE_BORDER_PAD_WIDTH(node);
                renderData->h = node->public.generic.height - NODE_BORDER_PAD_HEIGHT(node);
                renderData->u0 = imageView->u0;
                renderData->v0 = imageView->v0;
                renderData->u1 = imageView->u1;
                renderData->v1 = imageView->v1;
                renderData->clipTop = clip->top;
                renderData->clipBottom = clip->bottom;
                renderData->clipLeft = clip->left;
                renderData->clipRight = clip->right;
                renderData->clipRadiusTopLeft = 0;
                renderData->clipRadiusTopRight = 0;
                renderData->clipRadiusBottomLeft = 0;
                renderData->clipRadiusBottomRight = 0;
                renderData->textureIndex = groupImageIndex;
                renderData->filter = node->special.image.filter;
                renderData->repeat = node->special.image.repeat;
            }
        }

        // Draw unclipped border rects (1 draw call)
        NU_DrawBorderRects(win, &GUI.displayManager, &GUI.borderRectsRenderData, winW, winH);

        // Draw unclipped text (1 draw call per font)
        for (uint32 t=0; t<Array_Length(&GUI.stylesheet.fonts); t++) {
            Font* font = Stylesheet_Get_Font(&GUI.stylesheet, t);
            Array* textRenderData = Array_Get(&GUI.perFontTextRenderData, t);
            NU_DrawText(win, &GUI.displayManager, font, textRenderData, 0, 0);
        }

        // Draw clipped text (1 draw call per font)
        for (uint32 t=0; t<Array_Length(&GUI.stylesheet.fonts); t++) {
            Font* font = Stylesheet_Get_Font(&GUI.stylesheet, t);
            Array* clippedTextRenderData = Array_Get(&GUI.perFontClippedTextRenderData, t);
            NU_DrawClippedText(win, &GUI.displayManager, font, clippedTextRenderData, 0, 0);
        }

        // 4. Draw canvas contents
        for (uint32 n=0; n<Array_Length(&drawList->canvasNodes); n++) {
            Node* node = *(Node**)Array_Get(&drawList->canvasNodes, n);

            NU_Canvas_Context* ctx = Container_Get(&GUI.canvasContexts, node->special.canvas.ctxHandle);
            if (ctx == NULL) continue;

            NU_ClipBounds* clip = (NU_ClipBounds*)Hashmap_Get(&GUI.displayManager.clipMap, &node->clippedAncestor);

            // Compute offsets and clipping bounds
            float offsetX = roundf(node->public.generic.x + node->public.generic.borderLeft + node->public.generic.padLeft);
            float offsetY = roundf(node->public.generic.y + node->public.generic.borderTop + node->public.generic.padTop);
            float top    = node->public.generic.y + node->public.generic.borderTop + node->public.generic.padTop;
            float bottom = node->public.generic.y + node->public.generic.height - node->public.generic.borderBottom - node->public.generic.padBottom;
            float left   = node->public.generic.x + node->public.generic.borderLeft + node->public.generic.padLeft;
            float right  = node->public.generic.x + node->public.generic.width - node->public.generic.borderRight - node->public.generic.padRight;
            if (clip)
            {
                top    = max(top, clip->top);
                bottom = max(bottom, clip->bottom);
                left   = max(left, clip->left);
                right  = max(right, clip->right);
            }
            ctx->canvasWidth = node->public.generic.width;
            ctx->canvasHeight = node->public.generic.height;

            uint32 baseDepthLayer = node->layer + ABSOLUTE_Z_LAYER * NodeStatePosAbsolute(node);

            // Retarget z values of unseen vertices
            uint32 retargetedCount = 0;
            for (uint32 v=ctx->meshLayer.zValuesRetargeted;
                v<Vertex_RGB_List_Length(&ctx->meshLayer.vertices); v++)
            {
                vertex_rgb* vert = &ctx->meshLayer.vertices.array[i];
                vert->z = ComputeZ(baseDepthLayer, (uint8)(vert->z));
                retargetedCount++;
            }
            ctx->meshLayer.zValuesRetargeted += retargetedCount;

            // Retarget z values of unseen glyph render instances
            retargetedCount = 0;
            for (uint32 l=0; l<Array_Length(&ctx->textLayers); l++) {
                CanvasTextLayer* layer = Array_Get(&ctx->textLayers, l);
                for (uint32 g=layer->zValuesRetargeted; g<Array_Length(&layer->textRenderData); g++) {
                    GlyphRenderData* renderData = Array_Get(&layer->textRenderData, g);
                    renderData->z = ComputeZ(baseDepthLayer, (uint8)(renderData->z));
                    retargetedCount++;
                }
                layer->zValuesRetargeted += retargetedCount;
            }

            // Draw mesh layer
            NU_DrawVertexRGB(
                win,
                &GUI.displayManager,
                &ctx->meshLayer.vertices,
                &ctx->meshLayer.indices,
                offsetX, offsetY,
                top, bottom, left, right
            );

            // Draw text layers
            for (int l=0; l<Array_Length(&ctx->textLayers); l++) {
                CanvasTextLayer* layer = Array_Get(&ctx->textLayers, l);
                if (Array_Length(&layer->textRenderData) == 0) continue;
                Font* font = Stylesheet_Get_Font(&GUI.stylesheet, layer->fontIdx);
                NU_DrawText(win, &GUI.displayManager, font, &layer->textRenderData, offsetX, offsetY);
            }
        }

        // Draw images (1 draw call per image render group)
        for (int i=0; i<Array_Length(&GUI.imageRenderGroups); i++) {
            ImageRenderGroup* imageRenderGroup = Array_Get(&GUI.imageRenderGroups, i);
            NU_DrawImages(win, &GUI.displayManager, &GUI.imageResourceManager, imageRenderGroup);
        }

        // Display
        Window_Present(win, &GUI.displayManager);
    }

    GUI.awaiting_redraw = false;
}
