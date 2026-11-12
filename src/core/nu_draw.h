#pragma once

#include "vulkan/vulkan_core.h"
#define ABSOLUTE_Z_LAYER 4096

static NodeOverlap NodeVerticalOverlapState(Node* node, float y, float h)
{
    float top = node->base.y;
    float bottom = top + node->base.height;
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
    float right  = node->base.x + node->base.width;
    float bottom = node->base.y + node->base.height;
    return (right < 0 || bottom < 0 || node->base.x > winW || node->base.y > winH);
}

inline int NodeOutsideBounds(Node* node, float left, float right, float top, float bottom)
{
    return (node->base.y + node->base.height < top) |
           (node->base.y > bottom) |
           (node->base.x + node->base.width < left) |
           (node->base.x > right);
}

void NU_AddNodeTextToRenderDataArray(Node* node, NU_ClipBounds* clip, float z, Array* textRenderData)
{
    // Compute node inner dimensions
    float inner_width  = NODE_INNER_WIDTH(node);
    float inner_height = NODE_INNER_HEIGHT(node);
    float remaining_w = inner_width  - node->base.contentWidth;
    float remaining_h = inner_height - node->base.contentHeight;

    // Compute text position
    float x_align_offset = remaining_w * 0.5f * (float)node->horizontalTextAlignment;
    float y_align_offset = remaining_h * 0.5f * (float)node->verticalTextAlignment;
    float textPosX = floorf(node->base.x + node->base.borderLeft + node->base.padLeft + x_align_offset);
    float textPosY = floorf(node->base.y + node->base.borderTop  + node->base.padTop + y_align_offset);

    // Add glyph render data
    Font* node_font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontIdx);
    NuColor color = {
        node->special.text.textR,
        node->special.text.textG,
        node->special.text.textB,
        255
    };
    NU_Generate_Text_Renderdata(
        textRenderData,
        node_font,
        clip,
        node->special.text.textContent,
        textPosX,
        textPosY,
        z,
        color,
        inner_width
    );
}

void AddTextInputRenderData(Node* node, NU_ClipBounds* clip, Array* textRenderData, Array* borderRectRenderData)
{
    // Get font and inout text datastructure
    Font* font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontIdx);
    InputText* inputText = Container_Get(&GUI.textInputs, node->special.input.textInputHandle);

    // Recompute text offset
    if (inputText->updateOffsetsPostLayout) {
        inputText->updateOffsetsPostLayout = false;
        InputText_ComputeCursorTextOffset_PlaceEnd(inputText, node, font);
    }

    // Add text render data
    float textPosX = floorf(node->base.x + node->base.borderLeft + node->base.padLeft + inputText->textOffset);
    float textPosY = floorf(node->base.y + node->base.borderTop  + node->base.padTop);
    NuColor textColor = {
        node->special.text.textR,
        node->special.text.textG,
        node->special.text.textB,
        255
    };
    NU_Generate_Text_Renderdata(
        textRenderData,
        font,
        NULL,
        inputText->buffer,
        textPosX,
        textPosY,
        ComputeZ(node->layer + ABSOLUTE_Z_LAYER * NodeStatePosAbsolute(node), 1),
        textColor,
        -1.0f
    );

    // Text highlight rect
    if (node == GUI.focused_node && InputText_IsHighlighting(inputText))
    {
        NodePublicBase* n = &node->base;
        float cursorX = n->x + n->borderLeft + n->padLeft + inputText->cursorOffset;
        float highlightX = n->x + n->borderLeft + n->padLeft + inputText->highlightOffset;
        float x = highlightX; if (cursorX < x) x = cursorX;
        float y = n->y + n->borderTop + n->padTop;
        float width = fabs(cursorX - highlightX);
        float height = n->height - n->borderTop - n->borderBottom - n->padTop - n->padBottom;

        NuColor highlightColor = { 66, 137, 244, 255 };
        float32 packedColor = PackColorIntoUint32(highlightColor);

        BorderRectRenderData* renderData = Array_PushEmpty(borderRectRenderData);
        renderData->x = x;
        renderData->y = y;
        renderData->z = ComputeZ(node->layer + ABSOLUTE_Z_LAYER * NodeStatePosAbsolute(node), 2);
        renderData->w = width;
        renderData->h = height;
        renderData->backgroundRGBA = packedColor;
        renderData->borderRGBA = packedColor;
        renderData->radiusTl = 0;
        renderData->radiusTr = 0;
        renderData->radiusBl = 0;
        renderData->radiusBr = 0;
        renderData->borderTop = 0;
        renderData->borderBottom = 0;
        renderData->borderLeft = 0;
        renderData->borderRight = 0;
        renderData->scissorTop = clip->top;
        renderData->scissorBottom = clip->bottom;
        renderData->scissorLeft = clip->left;
        renderData->scissorRight = clip->right;
    }

    // Add cursor rect
    if (node == GUI.focused_node && !InputText_IsHighlighting(inputText))
    {
        NodePublicBase* n = &node->base;
        float x = n->x + n->borderLeft + n->padLeft + inputText->cursorOffset;
        float y = n->y + n->borderTop + n->padTop;
        float innerHeight = n->height - n->borderTop - n->borderBottom - n->padTop - n->padBottom;

        NuColor cursorColor = { 255, 255, 255, 255 };
        float32 packedColor = PackColorIntoUint32(cursorColor);

        BorderRectRenderData* renderData = Array_PushEmpty(borderRectRenderData);
        renderData->x = x;
        renderData->y = y;
        renderData->z = ComputeZ(node->layer + ABSOLUTE_Z_LAYER * NodeStatePosAbsolute(node), 3);
        renderData->w = 2;
        renderData->h = innerHeight;
        renderData->backgroundRGBA = packedColor;
        renderData->borderRGBA = packedColor;
        renderData->radiusTl = 0;
        renderData->radiusTr = 0;
        renderData->radiusBl = 0;
        renderData->radiusBr = 0;
        renderData->borderTop = 0;
        renderData->borderBottom = 0;
        renderData->borderLeft = 0;
        renderData->borderRight = 0;
        renderData->scissorTop = clip->top;
        renderData->scissorBottom = clip->bottom;
        renderData->scissorLeft = clip->left;
        renderData->scissorRight = clip->right;
    }
}

void Add_NodeRectRenderData(
    Node* node, float z,
    float scissorTop, float scissorBottom, float scissorLeft, float scissorRight,
    Array* borderRects
)
{
    NodePublicBase* n = &node->base;

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


void Add_ScrollbarRenderData(
    Node* node, float z,
    Stylesheet_Scrollbar_Style* scrollbarStyle,
    Array* borderRects
)
{
    NodePublicBase* n = &node->base;

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
            nodeInnerY = node->base.y + node->base.borderTop + node->base.padTop;
            nodeInnerHeight = NODE_INNER_HEIGHT(node);

            // Handle case where node is a table with a thead
            if (node->type == NU_TYPE_TABLE && node->childCount > 0) {
                if (node->firstChild->type == NU_TYPE_THEAD) {
                    nodeInnerY += node->firstChild->base.height;
                    nodeInnerHeight -= node->firstChild->base.height;
                }
            }
        }
        if (node->layoutFlags & OVERFLOW_HORIZONTAL_SCROLL) {
            nodeInnerX = node->base.x + node->base.borderLeft + node->base.padLeft;
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
                    clip.top = fmaxf(child->base.y - 1, nodeInnerY);
                    clip.bottom = fminf(child->base.y + child->base.height, nodeInnerY + nodeInnerHeight);
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

void NU_BeginCanvasRenderTargetPass(NU_Canvas_Context* ctx, Window* window)
{
    CanvasRenderTarget* rt = &ctx->renderTarget;
    Node* node = ctx->node;
    bool MSAA_4X = Canvas_IS_MSAA_4X(ctx);

    // Clear values
    VkClearValue clearValues[3] = {0}; uint32 clearValueCount = 0;
    float r = (float)(node->base.backgroundR) / 255.0f;
    float g = (float)(node->base.backgroundG) / 255.0f;
    float b = (float)(node->base.backgroundB) / 255.0f;
    if (MSAA_4X) {
        clearValues[0].color        = (VkClearColorValue){{r, g, b, 1.0f}};
        clearValues[1].depthStencil = (VkClearDepthStencilValue){0.0f, 0};
        clearValues[2].color        = (VkClearColorValue){{r, g, b, 1.0f}};
        clearValueCount = 3;
    }
    else {
        clearValues[0].color = (VkClearColorValue){{r, g, b, 1.0f}};
        clearValues[1].depthStencil = (VkClearDepthStencilValue){0.0f, 0};
        clearValueCount = 2;
    }

    // Begin render pass
    VkRenderPass renderPass = GUI.displayManager.sharedRenderPass;
    if (MSAA_4X) renderPass = GUI.displayManager.sharedRenderPassMSAA4X;
    VkRenderPassBeginInfo renderInfo = {0};
    renderInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderInfo.renderPass      = renderPass;
    renderInfo.framebuffer     = rt->framebuffer;
    renderInfo.renderArea      = (VkRect2D){ {0,0}, { rt->width, rt->height } };
    renderInfo.clearValueCount = clearValueCount;
    renderInfo.pClearValues    = clearValues;
    VkCommandBuffer cmd = window->commandBuffers[window->currentFrame];
    vkCmdBeginRenderPass(cmd, &renderInfo, VK_SUBPASS_CONTENTS_INLINE);
}

inline void NU_EndCanvasRenderTargetPass(NU_Canvas_Context* ctx, Window* window)
{
    VkCommandBuffer cmd = window->commandBuffers[window->currentFrame];

    vkCmdEndRenderPass(cmd);

    CanvasRenderTarget* renderTarget = &ctx->renderTarget;

    // MSAA resolve attachment already finishes in its final layout.
    if (renderTarget->msaaEnabled)
    {
        renderTarget->currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        return;
    }

    // Non-MSAA transition
    VkImageMemoryBarrier barrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout           = renderTarget->currentLayout,
        .newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = renderTarget->image,
        .subresourceRange    = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, NULL, 0, NULL, 1, &barrier
    );

    renderTarget->currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void NU_CanvasDraw(NU_Canvas_Context* ctx, Window* window)
{
    Node* node = ctx->node;
    CanvasRenderTarget* rt = &ctx->renderTarget;
    NU_ClipBounds* clip = (NU_ClipBounds*)Hashmap_Get(&GUI.displayManager.clipMap, &node->clippedAncestor);

    // Compute offsets and clipping bounds
    float offsetX = roundf(node->base.x + node->base.borderLeft + node->base.padLeft);
    float offsetY = roundf(node->base.y + node->base.borderTop + node->base.padTop);
    float top    = node->base.y + node->base.borderTop + node->base.padTop;
    float bottom = node->base.y + node->base.height - node->base.borderBottom - node->base.padBottom;
    float left   = node->base.x + node->base.borderLeft + node->base.padLeft;
    float right  = node->base.x + node->base.width - node->base.borderRight - node->base.padRight;
    if (clip)
    {
        top    = max(top, clip->top);
        bottom = max(bottom, clip->bottom);
        left   = max(left, clip->left);
        right  = max(right, clip->right);
    }
    ctx->canvasWidth  = node->base.width;
    ctx->canvasHeight = node->base.height;

    // Retarget z values of unseen vertices
    uint32 retargetedCount = 0;
    for (uint32 v = ctx->meshLayer.zValuesRetargeted; v < VertexSDFList_VertexCount(&ctx->meshLayer.vertexList); v++) {
        vertexSDF* vert = &ctx->meshLayer.vertexList.vertices[v];
        vert->z = ComputeZ((uint32)(vert->z), 0);
        retargetedCount++;
    }
    ctx->meshLayer.zValuesRetargeted += retargetedCount;

    // Retarget z values of unseen glyph render instances
    for (uint32 l=0; l<Array_Length(&ctx->textLayers); l++) {
        CanvasTextLayer* layer = Array_Get(&ctx->textLayers, l);
        retargetedCount = 0;
        for (uint32 g = layer->zValuesRetargeted; g < Array_Length(&layer->textRenderData); g++) {
            GlyphRenderData* renderData = Array_Get(&layer->textRenderData, g);
            renderData->z = ComputeZ((uint32)(renderData->z), 0);
            retargetedCount++;
        }
        layer->zValuesRetargeted += retargetedCount;
    }

    // Draw mesh layer
    NU_DrawVertexSDFList(
        window,
        &GUI.displayManager,
        &ctx->meshLayer.vertexList,
        0, 0,
        top, bottom, left, right,
        Canvas_IS_MSAA_4X(ctx)
    );

    // Draw text layers
    for (int l=0; l<Array_Length(&ctx->textLayers); l++) {
        CanvasTextLayer* layer = Array_Get(&ctx->textLayers, l);
        if (Array_Length(&layer->textRenderData) == 0) continue;
        Font* font = Stylesheet_Get_Font(&GUI.stylesheet, layer->fontIdx);
        NU_DrawText(
            window,
            &GUI.displayManager,
            font,
            &layer->textRenderData,
            0, 0,
            Canvas_IS_MSAA_4X(ctx)
        );
    }

    ctx->renderTargetUpToDate = true;
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

        // Prepare the frame, Failure -> skip frame (should not happen)
        if (!Window_PrepareFrame(win, &GUI.displayManager)) continue;

        // Get window dimensions and drawlist
        NuWindowDrawlist* drawList = &win->drawlist;
        int winW_int, winH_int;
        SDL_GetWindowSizeInPixels(win->sdlWin, &winW_int, &winH_int);
        float winW = (float)winW_int;
        float winH = (float)winH_int;

        // For each canvas in window
        for (uint32 n=0; n<Array_Length(&drawList->canvasNodes); n++)
        {
            Node* node = *(Node**)Array_Get(&drawList->canvasNodes, n);
            NU_Canvas_Context* ctx = Container_Get(&GUI.canvasContexts, node->special.canvas.ctxHandle);
            if (ctx)
            {
                // Update / destroy render target if settings changed -> returns true if changed
                bool changed = Canvas_CommitDesiredRenderType(ctx, &GUI.displayManager, &GUI.imageResourceManager);

                // If there is a render target and the render target changed -> redraw
                if (ctx->currentRenderType != NU_CANVAS_TYPE_UNBUFFERED && (changed || !ctx->renderTargetUpToDate)) {
                     NU_BeginCanvasRenderTargetPass(ctx, win);
                     NU_CanvasDraw(ctx, win);
                     NU_EndCanvasRenderTargetPass(ctx, win);
                 }
            }
        }

        // Begin render pass
        Window_BeginRenderPass(win, &GUI.displayManager);

        // Clear border rect renderdata array
        Array_Clear(&GUI.borderRectsRenderData);

        // Clear per-font text renderdata arrays
        for (int j=0; j<Array_Length(&GUI.perFontTextRenderData); j++) {
            Array* textRenderData = Array_Get(&GUI.perFontTextRenderData, j);
            Array_Clear(textRenderData);
            Array* clippedTextRenderData = Array_Get(&GUI.perFontClippedTextRenderData, j);
            Array_Clear(clippedTextRenderData);
        }

        // Lazily create image render groups
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

        // Clear image group renderdata arrays (excluding groups that were just added)
        for (int j=0; j<Array_Length(&GUI.imageRenderGroups) - numGroupsToAdd; j++) {
            ImageRenderGroup* imageRenderGroup = Array_Get(&GUI.imageRenderGroups, j);
            Array_Clear(&imageRenderGroup->imagesRenderData);
        }

        // Construct renderdata for non-clipped nodes
        for (uint32 n=0; n<drawList->drawNodes.size; n++)
        {
            Node* node = *(Node**)Array_Get(&drawList->drawNodes, n);

            // Border rect render data
            Add_NodeRectRenderData(node, ComputeZ(node->layer + ABSOLUTE_Z_LAYER * NodeStatePosAbsolute(node), 0), 0.0f, winH, 0.0f, winW, &GUI.borderRectsRenderData);
            if ((node->layoutFlags & OVERFLOW_VERTICAL_SCROLL) && node->base.contentHeight > NODE_INNER_HEIGHT(node)) {
                Add_ScrollbarRenderData(node, ComputeZ(node->layer + ABSOLUTE_Z_LAYER * NodeStatePosAbsolute(node), 1), &GUI.stylesheet.scrollbarStyle, &GUI.borderRectsRenderData);
            }

            // Text content render data
            if (NODE_HAS_TEXT_CONTENT(node)) {
                Array* textRenderData = Array_Get(&GUI.perFontTextRenderData, node->fontIdx);
                NU_AddNodeTextToRenderDataArray(node, NULL, ComputeZ(node->layer + ABSOLUTE_Z_LAYER * NodeStatePosAbsolute(node), 1), textRenderData);
            }

            // Text input render data
            else if (node->type == NU_TYPE_INPUT) {
                NU_ClipBounds clip = {0};
                clip.top = node->base.y;
                clip.left = node->base.x + node->base.borderLeft + node->base.padLeft;
                clip.right = node->base.x + node->base.width - node->base.borderRight - node->base.padRight;
                clip.bottom = node->base.y + node->base.height + 1000;
                Array* textRenderData = Array_Get(&GUI.perFontTextRenderData, node->fontIdx);
                AddTextInputRenderData(node, &clip, textRenderData, &GUI.borderRectsRenderData);
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
                renderData->x = node->base.x + node->base.borderLeft + node->base.padLeft;
                renderData->y = node->base.y + node->base.borderTop + node->base.padTop;
                renderData->z = ComputeZ(node->layer + ABSOLUTE_Z_LAYER * NodeStatePosAbsolute(node), 2);
                renderData->w = node->base.width - NODE_BORDER_PAD_WIDTH(node);
                renderData->h = node->base.height - NODE_BORDER_PAD_HEIGHT(node);
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
            if (NODE_HAS_TEXT_CONTENT(node)) {
                Font* font = Stylesheet_Get_Font(&GUI.stylesheet, node->fontIdx);
                Array* textRenderData = Array_Get(&GUI.perFontClippedTextRenderData, node->fontIdx);
                NU_AddNodeTextToRenderDataArray(node, clip, ComputeZ(node->layer + ABSOLUTE_Z_LAYER * NodeStatePosAbsolute(node), 1), textRenderData);
            }

            // Text input render data
            else if (node->type == NU_TYPE_INPUT) {
                InputText* inputText = Container_Get(&GUI.textInputs, node->special.input.textInputHandle);
                if (inputText->numBytes > 0) {
                    NU_ClipBounds innerClip = *clip;
                    innerClip.left += node->base.borderLeft + node->base.padLeft;
                    innerClip.right -= node->base.borderRight + node->base.padRight;
                    Array* textRenderData = Array_Get(&GUI.perFontTextRenderData, node->fontIdx);
                    AddTextInputRenderData(node, &innerClip, textRenderData, &GUI.borderRectsRenderData);
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
                renderData->x = node->base.x + node->base.borderLeft + node->base.padLeft;
                renderData->y = node->base.y + node->base.borderTop + node->base.padTop;
                renderData->z = ComputeZ(node->layer + ABSOLUTE_Z_LAYER * NodeStatePosAbsolute(node), 2);
                renderData->w = node->base.width - NODE_BORDER_PAD_WIDTH(node);
                renderData->h = node->base.height - NODE_BORDER_PAD_HEIGHT(node);
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
        NU_DrawBorderRects(win, &GUI.displayManager, &GUI.borderRectsRenderData, winW, winH, false);

        // Draw unclipped text (1 draw call per font)
        for (uint32 t=0; t<Array_Length(&GUI.stylesheet.fonts); t++) {
            Font* font = Stylesheet_Get_Font(&GUI.stylesheet, t);
            Array* textRenderData = Array_Get(&GUI.perFontTextRenderData, t);
            NU_DrawText(win, &GUI.displayManager, font, textRenderData, 0, 0, false);
        }

        // Draw clipped text (1 draw call per font)
        for (uint32 t=0; t<Array_Length(&GUI.stylesheet.fonts); t++) {
            Font* font = Stylesheet_Get_Font(&GUI.stylesheet, t);
            Array* clippedTextRenderData = Array_Get(&GUI.perFontClippedTextRenderData, t);
            NU_DrawClippedText(win, &GUI.displayManager, font, clippedTextRenderData, 0, 0, false);
        }

        // Render unbuffered canvases + composite buffered canvas render targets
        for (uint32 n=0; n<Array_Length(&drawList->canvasNodes); n++) {
            Node* node = *(Node**)Array_Get(&drawList->canvasNodes, n);
            NU_Canvas_Context* ctx = Container_Get(&GUI.canvasContexts, node->special.canvas.ctxHandle);
            if (ctx == NULL) continue;

            // Draw to window frame buffer
            if (ctx->currentRenderType == NU_CANVAS_TYPE_UNBUFFERED) {
                NU_CanvasDraw(ctx, win);
            }
            // Composite already drawn canvas
            else {
                ImageRenderData renderData;
                renderData.x = node->base.x + node->base.borderLeft + node->base.padLeft;
                renderData.y = node->base.y + node->base.borderTop + node->base.padTop;
                renderData.z = ComputeZ(node->layer + ABSOLUTE_Z_LAYER * NodeStatePosAbsolute(node), 1);
                renderData.w = NODE_INNER_WIDTH(node);
                renderData.h = NODE_INNER_HEIGHT(node);
                renderData.u0 = 0.0f;
                renderData.v0 = 0.0f;
                renderData.u1 = 1.0f;
                renderData.v1 = 1.0f;
                renderData.clipTop = 0;
                renderData.clipBottom = 60000;
                renderData.clipLeft = 0;
                renderData.clipRight = 60000;
                renderData.clipRadiusTopLeft = 0;
                renderData.clipRadiusTopRight = 0;
                renderData.clipRadiusBottomLeft = 0;
                renderData.clipRadiusBottomRight = 0;
                renderData.textureIndex = 0;
                renderData.filter = IMAGE_FILTER_NEAREST;
                renderData.repeat = IMAGE_REPEAT_NONE;
                NU_DrawCanvasRenderTarget(win, &GUI.displayManager, &renderData, ctx->renderTarget.descriptorSet, false);
            }
        }

        // Draw images (1 draw call per image render group)
        for (int i=0; i<Array_Length(&GUI.imageRenderGroups); i++) {
            ImageRenderGroup* imageRenderGroup = Array_Get(&GUI.imageRenderGroups, i);
            NU_DrawImages(win, &GUI.displayManager, &GUI.imageResourceManager, imageRenderGroup, false);
        }

        // Display
        Window_Present(win, &GUI.displayManager);
    }

    GUI.awaiting_redraw = false;
}
