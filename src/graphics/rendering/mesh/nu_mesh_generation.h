#pragma once

void Add_NodeRectRenderData(
    Node* node, float z,
    float scissorTop, float scissorBottom, float scissorLeft, float scissorRight,
    Array* borderRects
)
{
    NuNodeGenericAccess* n = &node->public.generic;

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
