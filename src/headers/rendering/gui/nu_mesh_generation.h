#pragma once
#include <rendering/nu_draw_structures.h>
#include <math.h>

void Generate_Corner_Segment(
    Vertex_RGB_List* vertices, Index_List* indices, 
    vec2 anchor, 
    float angleStart, float angleEnd, 
    float radius, 
    float borderThicknessStart, float borderThicknessEnd, 
    float width, float height,
    float b_r, float b_g, float b_b,
    float bgR, float bgG, float bgB, 
    int cp, 
    int cornerIndex, bool hideBackground)
{

    int vertOffset = vertices->size;
    vertices->size += hideBackground ? 2 * cp : 3 * cp + 1;

    if (cp == 1)
    {           
        // --- Calculate inner vertex_rgb offsets based on corner index ---
        static const float signLutX[4] = { +1.0f, -1.0f, -1.0f, +1.0f };
        static const float signLutY[4] = { +1.0f, +1.0f, -1.0f, -1.0f };
        static const int border_select_lut_x[4] = { 0, 1, 0, 1 }; // 0 = start, 1 = end
        static const int border_select_lut_y[4] = { 1, 0, 1, 0 }; // 0 = start, 1 = end
        float border_x = border_select_lut_x[cornerIndex] ? borderThicknessEnd : borderThicknessStart;
        float border_y = border_select_lut_y[cornerIndex] ? borderThicknessEnd : borderThicknessStart;
        float offsetX = signLutX[cornerIndex] * border_x;
        float offsetY = signLutY[cornerIndex] * border_y;
        
        // --- Set values for inner and outer border vertices ---
        int outerIndex = vertOffset + 1;
        vertices->array[vertOffset].x = anchor.x + offsetX;
        vertices->array[vertOffset].y = anchor.y + offsetY;
        vertices->array[vertOffset].r = b_r;
        vertices->array[vertOffset].g = b_g;
        vertices->array[vertOffset].b = b_b;
        vertices->array[outerIndex].x = anchor.x;
        vertices->array[outerIndex].y = anchor.y;
        vertices->array[outerIndex].r = b_r;
        vertices->array[outerIndex].g = b_g;
        vertices->array[outerIndex].b = b_b;

        // -- Set value for innser and outer background vertices ---
        if (!hideBackground) {
            int bgOuterIndex = outerIndex + 1;
            int bgInnerIndex = outerIndex + 2;
            vertices->array[bgOuterIndex].x = vertices->array[vertOffset].x;
            vertices->array[bgOuterIndex].y = vertices->array[vertOffset].y;
            vertices->array[bgOuterIndex].r = bgR;
            vertices->array[bgOuterIndex].g = bgG;
            vertices->array[bgOuterIndex].b = bgB;
            vertices->array[bgInnerIndex].x = anchor.x + signLutX[cornerIndex] * width * 0.33f;
            vertices->array[bgInnerIndex].y = anchor.y + signLutY[cornerIndex] * height * 0.33f;
            vertices->array[bgInnerIndex].r = bgR;
            vertices->array[bgInnerIndex].g = bgG;
            vertices->array[bgInnerIndex].b = bgB;
        }
        return;
    }

    // --- Precomputations ---
    float angleStep = (angleEnd - angleStart) / (cp - 1);
    float currAngle = angleStart;
    float sinCurr = sinf(currAngle);
    float cosCurr = cosf(currAngle);
    float cosStep = cosf(angleStep);
    float sinStep = sinf(angleStep);

    // --- Inside radius, axis elliptical squish and border thickness ---
    float dirX = (cornerIndex == 1 || cornerIndex == 3) ? 1.0f : -1.0f; // left/right
    float dirY = (cornerIndex == 0 || cornerIndex == 2) ? -1.0f : 1.0f; // bottom/top
    float borderThicknessX = (dirX < 0) ? borderThicknessStart : borderThicknessEnd;
    float borderThicknessY = (dirY < 0) ? borderThicknessEnd : borderThicknessStart;
    float innerRadiusX = radius - borderThicknessX;
    float innerRadiusY = radius - borderThicknessY;
    int outerBackgroundOffset = vertOffset + 2 * cp;
    int innerBackgroundOffset = outerBackgroundOffset + cp;

    // --- Create inner background vertex_rgb ---
    if (!hideBackground) {
        static const float signLutX[4] = { +1.0f, -1.0f, -1.0f, +1.0f };
        static const float signLutY[4] = { +1.0f, +1.0f, -1.0f, -1.0f };
        vertices->array[innerBackgroundOffset].x = anchor.x;
        vertices->array[innerBackgroundOffset].y = anchor.y;
        vertices->array[innerBackgroundOffset].r = bgR;
        vertices->array[innerBackgroundOffset].g = bgG;
        vertices->array[innerBackgroundOffset].b = bgB;
    }

    // --- Generate vertices ---
    // --- Handles three possible cases for how the inner curve should be handled. This looks rediculous but is a necessary optimisation to remove 6 if statements ---
    float xCurveFactor    = innerRadiusX > 0.0f ? 1.0f : 0.0f; // Precompute curve/straight factors
    float yCurveFactor    = innerRadiusY > 0.0f ? 1.0f : 0.0f;
    float xStraightFactor = 1.0f - xCurveFactor;
    float yStraightFactor = 1.0f - yCurveFactor;
    static const float sx[4] = { -1.0f, -1.0f,  1.0f,  1.0f }; // Precompute straight edge offsets for each corner
    static const float sy[4] = { -1.0f,  1.0f,  1.0f, -1.0f };
    float innerXStraightTerm = xStraightFactor * sx[cornerIndex] * (borderThicknessX - radius); // Straight contributions (lookup-based, branchless)
    float innerYStraightTerm = yStraightFactor * sy[cornerIndex] * (borderThicknessY - radius);
    vertex_rgb* innerPtr = &vertices->array[vertOffset];
    vertex_rgb* outerPtr = &vertices->array[vertOffset + cp];
    vertex_rgb* outerBgPtr = hideBackground ? NULL : &vertices->array[outerBackgroundOffset];
    for (int i=0; i<cp; i++) 
    {
        // --- Rotate angle (sine/cosine) ---
        float sinA = sinCurr * cosStep + cosCurr * sinStep;
        float cosA = cosCurr * cosStep - sinCurr * sinStep;
        sinCurr = sinA;
        cosCurr = cosA;

        // --- Compute inner vertex ---
        float innerX = anchor.x + innerRadiusX * cosA * xCurveFactor + innerXStraightTerm;
        float innerY = anchor.y + innerRadiusY * sinA * yCurveFactor + innerYStraightTerm;
        innerPtr[i].x = innerX;
        innerPtr[i].y = innerY;
        innerPtr[i].r = b_r;
        innerPtr[i].g = b_g;
        innerPtr[i].b = b_b;

        // --- Compute outer vertex ---
        outerPtr[i].x = anchor.x + cosA * radius;
        outerPtr[i].y = anchor.y + sinA * radius;
        outerPtr[i].r = b_r;
        outerPtr[i].g = b_g;
        outerPtr[i].b = b_b;

        // --- Compute background vertex ---
        if (!hideBackground) {
            outerBgPtr[i].x = innerX;
            outerBgPtr[i].y = innerY;
            outerBgPtr[i].r = bgR;
            outerBgPtr[i].g = bgG;
            outerBgPtr[i].b = bgB;
        }
    }

    // --- Generate indices ---
    int indexOffset = indices->size;
    indices->size += hideBackground ? (cp - 1) * 6 : (cp - 1) * 6 + (cp - 1) * 3;
    int bgOffset = indexOffset + (cp-1) * 6;
    for (int i = 0; i < cp - 1; i++) 
    {
        // --- Border curve indices ---
        int idxInner0 = vertOffset + i;
        int idxInner1 = idxInner0 + 1;
        int idxOuter0 = idxInner0 + cp;
        int idxOuter1 = idxOuter0 + 1;
        indices->array[indexOffset + 0] = idxInner0;
        indices->array[indexOffset + 1] = idxOuter0;
        indices->array[indexOffset + 2] = idxOuter1;
        indices->array[indexOffset + 3] = idxInner0;
        indices->array[indexOffset + 4] = idxOuter1;
        indices->array[indexOffset + 5] = idxInner1;
        indexOffset += 6;

        // --- Background indices ---
        int idx0 = outerBackgroundOffset + i;
        int idx1 = idx0 + 1;
        indices->array[bgOffset + 0] = innerBackgroundOffset;
        indices->array[bgOffset + 1] = idx0;
        indices->array[bgOffset + 2] = idx1;
        bgOffset += 3;
    }
}

void Construct_Border_Rect(
    Node* node,
    float screen_width, 
    float screen_height,
    Vertex_RGB_List* vertices, Index_List* indices
)
{
    const float PI = 3.14159265f;

    // --- Constrain border radii ---
    float borderRadiusBl = node->borderRadiusBl;
    float borderRadiusBr = node->borderRadiusBr;
    float borderRadiusTl = node->borderRadiusTl;
    float borderRadiusTr = node->borderRadiusTr;
    float left_radii_sum   = borderRadiusTl + borderRadiusBl;
    float right_radii_sum  = borderRadiusTr + borderRadiusBr;
    float top_radii_sum    = borderRadiusTl + borderRadiusTr;
    float bottom_radii_sum = borderRadiusBl + borderRadiusBr;
    if (left_radii_sum   > node->height)  { float scale = node->height / left_radii_sum;   borderRadiusTl *= scale; borderRadiusBl *= scale; }
    if (right_radii_sum  > node->height)  { float scale = node->height / right_radii_sum;  borderRadiusTr *= scale; borderRadiusBr *= scale; }
    if (top_radii_sum    > node->width )  { float scale = node->width  / top_radii_sum;    borderRadiusTl *= scale; borderRadiusTr *= scale; }
    if (bottom_radii_sum > node->width )  { float scale = node->width  / bottom_radii_sum; borderRadiusBl *= scale; borderRadiusBr *= scale; }

    // --- Convert colors ---
    float border_r_fl      = (float)node->borderR / 255.0f;
    float border_g_fl      = (float)node->borderG / 255.0f;
    float border_b_fl      = (float)node->borderB / 255.0f;
    float bg_r_fl          = (float)node->backgroundR / 255.0f;
    float bg_g_fl          = (float)node->backgroundG / 255.0f;
    float bg_b_fl          = (float)node->backgroundB / 255.0f;

    // --- Determine corner points ---
    int max_pts            = 64;
    int tl_pts             = borderRadiusTl < 1.0f ? 1 : min((int)borderRadiusTl + 3, max_pts);
    int tr_pts             = borderRadiusTr < 1.0f ? 1 : min((int)borderRadiusTr + 3, max_pts);
    int br_pts             = borderRadiusBr < 1.0f ? 1 : min((int)borderRadiusBr + 3, max_pts);
    int bl_pts             = borderRadiusBl < 1.0f ? 1 : min((int)borderRadiusBl + 3, max_pts);
    int total_pts          = tl_pts + tr_pts + br_pts + bl_pts;

    // --- Corner anchors ---
    vec2 tl_a              = { floorf(node->x + borderRadiusTl),               floorf(node->y + borderRadiusTl) };
    vec2 tr_a              = { floorf(node->x + node->width - borderRadiusTr), floorf(node->y + borderRadiusTr) };
    vec2 bl_a              = { floorf(node->x + borderRadiusBl),               floorf(node->y + node->height - borderRadiusBl) };
    vec2 br_a              = { floorf(node->x + node->width - borderRadiusBr), floorf(node->y + node->height - borderRadiusBr) };

    // --- Allocate extra space in vertex and index lists ---
    uint32_t additional_vertices = node->hideBackground ? total_pts * 2 + 4 : total_pts * 3 + 4;    // each corner contributes 3*cp + 1 verts
    uint32_t additional_indices = (total_pts - 4) * 6                                                // curved edges
                                  + 24                                                               // straight sides
                                  + (node->hideBackground ? 0 : (total_pts - 4) * 3 + 30);          // background tris
    if (vertices->size + additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, additional_vertices);
    if (indices->size + additional_indices > indices->capacity) Index_List_Grow(indices, additional_indices);


    // --- Generate corner vertices and indices ---
    int TL = vertices->size;
    Generate_Corner_Segment(vertices, indices, tl_a, PI, 1.5f * PI, borderRadiusTl, node->borderLeft, node->borderTop, node->width, node->height, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, tl_pts, 0, node->hideBackground);
    int TR = vertices->size;
    Generate_Corner_Segment(vertices, indices, tr_a, 1.5f * PI, 2.0f * PI, borderRadiusTr, node->borderTop, node->borderRight, node->width, node->height, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, tr_pts, 1, node->hideBackground);
    int BR = vertices->size;
    Generate_Corner_Segment(vertices, indices, br_a, 0.0f, 0.5f * PI, borderRadiusBr, node->borderRight, node->borderBottom, node->width, node->height, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, br_pts, 2, node->hideBackground);
    int BL = vertices->size;
    Generate_Corner_Segment(vertices, indices, bl_a, 0.5f * PI, PI, borderRadiusBl, node->borderBottom, node->borderLeft, node->width, node->height, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, bl_pts, 3, node->hideBackground);



    // --- Fill in side indices ---
    uint32_t* indices_write = indices->array + indices->size;

    // --- Top side quad ---
    *indices_write++ = TL + tl_pts - 1;
    *indices_write++ = TL + 2 * tl_pts - 1;
    *indices_write++ = TR;
    *indices_write++ = TL + 2 * tl_pts - 1;
    *indices_write++ = TR + tr_pts;
    *indices_write++ = TR;

    // --- Right side quad ---
    *indices_write++ = TR + tr_pts - 1;
    *indices_write++ = TR + 2 * tr_pts - 1;
    *indices_write++ = BR;
    *indices_write++ = TR + 2 * tr_pts - 1;
    *indices_write++ = BR + br_pts;
    *indices_write++ = BR;

    // --- Bottom side quad ---
    *indices_write++ = BR + br_pts - 1;
    *indices_write++ = BR + 2 * br_pts - 1;
    *indices_write++ = BL;
    *indices_write++ = BR + 2 * br_pts - 1;
    *indices_write++ = BL + bl_pts;
    *indices_write++ = BL;

    // --- Left side quad ---
    *indices_write++ = BL + bl_pts - 1;
    *indices_write++ = BL + 2 * bl_pts - 1;
    *indices_write++ = TL;
    *indices_write++ = BL + 2 * bl_pts - 1;
    *indices_write++ = TL + tl_pts;
    *indices_write++ = TL;

    // --- Fill in background indices ---
    if (!node->hideBackground) 
    {
        int TL_bg_connector = TL + 3 * tl_pts; 
        int TR_bg_connector = TR + 3 * tr_pts;
        int BR_bg_connector = BR + 3 * br_pts;
        int BL_bg_connector = BL + 3 * bl_pts;

        // --- Central quad ---
        *indices_write++ = TL_bg_connector; *indices_write++ = TR_bg_connector; *indices_write++ = BR_bg_connector;
        *indices_write++ = TL_bg_connector; *indices_write++ = BR_bg_connector; *indices_write++ = BL_bg_connector;

        // --- Top inner quad ---
        *indices_write++ = TL_bg_connector; *indices_write++ = TL_bg_connector - 1; *indices_write++ = TR + tr_pts * 2;
        *indices_write++ = TL_bg_connector; *indices_write++ = TR + tr_pts * 2;     *indices_write++ = TR_bg_connector;

        // --- Right inner quad ---
        *indices_write++ = TR_bg_connector; *indices_write++ = TR_bg_connector - 1; *indices_write++ = BR + br_pts * 2;
        *indices_write++ = TR_bg_connector; *indices_write++ = BR + br_pts * 2;     *indices_write++ = BR_bg_connector;

        // --- Bottom inner quad ---
        *indices_write++ = BR_bg_connector; *indices_write++ = BR_bg_connector - 1; *indices_write++ = BL + bl_pts * 2;
        *indices_write++ = BR_bg_connector; *indices_write++ = BL + bl_pts * 2;     *indices_write++ = BL_bg_connector;

        // --- Left inner quad ---
        *indices_write++ = BL_bg_connector; *indices_write++ = BL_bg_connector - 1; *indices_write++ = TL + tl_pts * 2;
        *indices_write++ = BL_bg_connector; *indices_write++ = TL + tl_pts * 2;     *indices_write++ = TL_bg_connector;
    }

    // update size once
    indices->size = (int)(indices_write - indices->array);
}

void Construct_Scroll_Thumb(Node* node,
    float screen_width, 
    float screen_height,
    Vertex_RGB_List* vertices, Index_List* indices
)
{
    // --- Allocate extra space in vertex and index lists ---
    uint32_t additional_vertices = 8;    
    uint32_t additional_indices = 12;          
    if (vertices->size + additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, additional_vertices);
    if (indices->size + additional_indices > indices->capacity) Index_List_Grow(indices, additional_indices);


    NU_Layer* child_layer = &__NGUI.tree.layers[node->layer + 1];
    Node* first_child = NU_Layer_Get(child_layer, node->firstChildIndex);
    float scroll_view_height = node->contentHeight;
    float track_height = node->height - node->borderTop - node->borderBottom;
    float inner_height_w_pad = track_height - node->padTop - node->padBottom;
    float inner_proportion_of_content_height = inner_height_w_pad / scroll_view_height;
    float thumb_height = inner_proportion_of_content_height * track_height;


    float x = node->x + node->width - node->borderRight - 8.0f;
    float y = node->y + node->borderTop;
    float thumb_y = node->y + node->borderTop + (node->scrollV * (track_height - thumb_height));
    float w = 8.0f;

    uint32_t vertOffset = vertices->size;

    // Background Rect TL
    vertices->array[vertOffset + 0].x = x;
    vertices->array[vertOffset + 0].y = y;
    vertices->array[vertOffset + 0].r = 0.1f;
    vertices->array[vertOffset + 0].g = 0.1f;
    vertices->array[vertOffset + 0].b = 0.1f;

    // Background Rect TR
    vertices->array[vertOffset + 1].x = x + w;
    vertices->array[vertOffset + 1].y = y;
    vertices->array[vertOffset + 1].r = 0.1f;
    vertices->array[vertOffset + 1].g = 0.1f;
    vertices->array[vertOffset + 1].b = 0.1f;

    // Background Rect BL
    vertices->array[vertOffset + 2].x = x;
    vertices->array[vertOffset + 2].y = y + track_height;
    vertices->array[vertOffset + 2].r = 0.1f;
    vertices->array[vertOffset + 2].g = 0.1f;
    vertices->array[vertOffset + 2].b = 0.1f;

    // Background Rect BR
    vertices->array[vertOffset + 3].x = x + w;
    vertices->array[vertOffset + 3].y = y + track_height;
    vertices->array[vertOffset + 3].r = 0.1f;
    vertices->array[vertOffset + 3].g = 0.1f;
    vertices->array[vertOffset + 3].b = 0.1f;

    // Background Thumb TL
    vertices->array[vertOffset + 4].x = x;
    vertices->array[vertOffset + 4].y = thumb_y;
    vertices->array[vertOffset + 4].r = 0.9f;
    vertices->array[vertOffset + 4].g = 0.9f;
    vertices->array[vertOffset + 4].b = 0.9f;

    // Background Thumb TR
    vertices->array[vertOffset + 5].x = x + w;
    vertices->array[vertOffset + 5].y = thumb_y;
    vertices->array[vertOffset + 5].r = 0.9f;
    vertices->array[vertOffset + 5].g = 0.9f;
    vertices->array[vertOffset + 5].b = 0.9f;

    // Background Thumb BL
    vertices->array[vertOffset + 6].x = x;
    vertices->array[vertOffset + 6].y = thumb_y + thumb_height;
    vertices->array[vertOffset + 6].r = 0.9f;
    vertices->array[vertOffset + 6].g = 0.9f;
    vertices->array[vertOffset + 6].b = 0.9f;

    // Background Thumb BR
    vertices->array[vertOffset + 7].x = x + w;
    vertices->array[vertOffset + 7].y = thumb_y + thumb_height;
    vertices->array[vertOffset + 7].r = 0.9f;
    vertices->array[vertOffset + 7].g = 0.9f;
    vertices->array[vertOffset + 7].b = 0.9f;

    // Indices
    uint32_t* indices_write = indices->array + indices->size;
    *indices_write++ = vertOffset + 0;
    *indices_write++ = vertOffset + 1;
    *indices_write++ = vertOffset + 2;
    *indices_write++ = vertOffset + 1;
    *indices_write++ = vertOffset + 2;
    *indices_write++ = vertOffset + 3;
    *indices_write++ = vertOffset + 4;
    *indices_write++ = vertOffset + 5;
    *indices_write++ = vertOffset + 6;
    *indices_write++ = vertOffset + 5;
    *indices_write++ = vertOffset + 6;
    *indices_write++ = vertOffset + 7;

    vertices->size += additional_vertices;
    indices->size += additional_indices;
}