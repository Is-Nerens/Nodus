#pragma once
#include <rendering/nu_renderer_structures.h>
#include <math.h>

static inline u32 PackRGBA(u8 r, u8 g, u8 b, u8 a)
{
    return ((u32)r << 0) |
           ((u32)g << 8) |
           ((u32)b << 16) |
           ((u32)a << 24);
}

void Add_NodeRectRenderData(
    NodeP* node, float z,
    float scissorTop, float scissorBottom, float scissorLeft, float scissorRight,
    Array* borderRects
)
{
    Node* n = &node->node;

    u8 alpha = 255;
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

    renderData->backgroundRGBA = PackRGBA(
        n->backgroundR,
        n->backgroundG,
        n->backgroundB,
        alpha
    );
    renderData->borderRGBA = PackRGBA(
        n->borderR,
        n->borderG,
        n->borderB,
        255
    );
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
    NodeP* node, float z,
    Stylesheet_Scrollbar_Style* scrollbarStyle,
    Array* borderRects
)
{
    Node* n = &node->node;

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
    float thumbY = trackY + scrollbarStyle->trackPadTop + node->scrollV * scrollTravel;
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
    trackRData->backgroundRGBA = PackRGBA(
        scrollbarStyle->trackBackgroundR,
        scrollbarStyle->trackBackgroundG,
        scrollbarStyle->trackBackgroundB,
        255
    );
    trackRData->borderRGBA = PackRGBA(
        scrollbarStyle->trackBorderR,
        scrollbarStyle->trackBorderG,
        scrollbarStyle->trackBorderB,
        255
    );
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
    thumbRData->backgroundRGBA = PackRGBA(
        scrollbarStyle->thumbBackgroundR,
        scrollbarStyle->thumbBackgroundG,
        scrollbarStyle->thumbBackgroundB,
        255
    );
    thumbRData->borderRGBA = PackRGBA(
        scrollbarStyle->thumbBorderR,
        scrollbarStyle->thumbBorderG,
        scrollbarStyle->thumbBorderB,
        255
    );
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
    NodeP* node, float z,
    InputText* inputText,
    Vertex_RGB_List* vertices,
    Index_List* indices)
{
    Node* n = &node->node;
    float x = n->x + n->borderLeft + n->padLeft + inputText->cursorOffset;
    float y = n->y + n->borderTop + n->padTop;
    float innerHeight = n->height - n->borderTop  - n->borderBottom - n->padTop - n->padBottom;

    vertices->array[0] = (vertex_rgb){ x, y, z, 1.0f, 1.0f, 1.0f }; // top left
    vertices->array[1] = (vertex_rgb){ x + 1, y, z, 1.0f, 1.0f, 1.0f }; // top right
    vertices->array[2] = (vertex_rgb){ x, y + innerHeight - 1, z, 1.0f, 1.0f, 1.0f }; // bottom left
    vertices->array[3] = (vertex_rgb){ x + 1, y + innerHeight - 1, z, 1.0f, 1.0f, 1.0f }; // bottom right

    // indices
    u32* indices_write = indices->array;
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
    NodeP* node, float z,
    InputText* inputText,
    Vertex_RGB_List* vertices,
    Index_List* indices)
{
    Node* n = &node->node;

    float cursorX = n->x + n->borderLeft + n->padLeft + inputText->cursorOffset;
    float highlightX = n->x + n->borderLeft + n->padLeft + inputText->highlightOffset;
    float x = highlightX; if (cursorX < x) x = cursorX;
    float y = n->y + n->borderTop + n->padTop;
    float width = fabs(cursorX - highlightX);
    float height = n->height - n->borderTop - n->borderBottom - n->padTop - n->padBottom;

    float highlightR = 0.26f;
    float highlightG = 0.54f;
    float highlightB = 0.96f;

    vertices->array[0] = (vertex_rgb){ x, y, z, highlightR, highlightG, highlightB }; // top left
    vertices->array[1] = (vertex_rgb){ x + width, y, z, highlightR, highlightG, highlightB }; // top right
    vertices->array[2] = (vertex_rgb){ x, y + height, z, highlightR, highlightG, highlightB }; // bottom left
    vertices->array[3] = (vertex_rgb){ x + width, y + height, z, highlightR, highlightG, highlightB }; // bottom right

    // indices
    u32* indices_write = indices->array;
    *indices_write++ = 0;
    *indices_write++ = 1;
    *indices_write++ = 2;
    *indices_write++ = 1;
    *indices_write++ = 2;
    *indices_write++ = 3;

    vertices->size = 4;
    indices->size = 6;
}
