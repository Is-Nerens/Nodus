#pragma once
#include <rendering/nu_renderer_structures.h>
#include <math.h>

void Generate_Corner_Segment(
    Vertex_RGB_List* vertices, Index_List* indices, 
    vec2 anchor, 
    float angleStart, float angleEnd, 
    float radius, 
    float borderThicknessStart, float borderThicknessEnd, 
    float width, float height, float z,
    float bR, float bG, float bB,
    float bgR, float bgG, float bgB, 
    int cp, 
    int cornerIndex, int hideBackground)
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
        vertices->array[vertOffset].z = z;
        vertices->array[vertOffset].r = bR;
        vertices->array[vertOffset].g = bG;
        vertices->array[vertOffset].b = bB;
        vertices->array[outerIndex].x = anchor.x;
        vertices->array[outerIndex].y = anchor.y;
        vertices->array[outerIndex].z = z;
        vertices->array[outerIndex].r = bR;
        vertices->array[outerIndex].g = bG;
        vertices->array[outerIndex].b = bB;

        // -- Set value for innser and outer background vertices ---
        if (!hideBackground) {
            int bgOuterIndex = outerIndex + 1;
            int bgInnerIndex = outerIndex + 2;
            vertices->array[bgOuterIndex].x = vertices->array[vertOffset].x;
            vertices->array[bgOuterIndex].y = vertices->array[vertOffset].y;
            vertices->array[bgOuterIndex].z = z;
            vertices->array[bgOuterIndex].r = bgR;
            vertices->array[bgOuterIndex].g = bgG;
            vertices->array[bgOuterIndex].b = bgB;
            vertices->array[bgInnerIndex].x = anchor.x + signLutX[cornerIndex] * width * 0.33f;
            vertices->array[bgInnerIndex].y = anchor.y + signLutY[cornerIndex] * height * 0.33f;
            vertices->array[bgInnerIndex].z = z;
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
        vertices->array[innerBackgroundOffset].z = z;
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
        // --- Use current angle BEFORE rotating (fixes off-by-one) ---
        float sinA = sinCurr;
        float cosA = cosCurr;

        // --- Rotate angle (sine/cosine) ---
        float sinNext = sinCurr * cosStep + cosCurr * sinStep;
        float cosNext = cosCurr * cosStep - sinCurr * sinStep;
        sinCurr = sinNext;
        cosCurr = cosNext;

        // --- Compute inner vertex ---
        float innerX = anchor.x + innerRadiusX * cosA * xCurveFactor + innerXStraightTerm;
        float innerY = anchor.y + innerRadiusY * sinA * yCurveFactor + innerYStraightTerm;
        innerPtr[i].x = innerX;
        innerPtr[i].y = innerY;
        innerPtr[i].z = z;
        innerPtr[i].r = bR;
        innerPtr[i].g = bG;
        innerPtr[i].b = bB;

        // --- Compute outer vertex ---
        outerPtr[i].x = anchor.x + cosA * radius;
        outerPtr[i].y = anchor.y + sinA * radius;
        outerPtr[i].z = z;
        outerPtr[i].r = bR;
        outerPtr[i].g = bG;
        outerPtr[i].b = bB;

        // --- Compute background vertex ---
        if (!hideBackground) {
            outerBgPtr[i].x = innerX;
            outerBgPtr[i].y = innerY;
            outerBgPtr[i].z = z;
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

int CornerPoints(float r)
{
    if (r < 1.0f) return 1;
    float arc = 0.5f * 3.14159265f * r; // quarter arc length
    float step = 2.0f;                 // pixels per segment
    int pts = (int)(arc / step) + 1;
    if (pts < 4) pts = 4;              // minimum smoothness
    if (pts > 64) pts = 64;            // cap
    return pts;
}

void Construct_NodeBorderRect(
    NodeP* node, float z,
    Vertex_RGB_List* vertices, Index_List* indices
)
{
    const float PI = 3.14159265f;
    Node* n = &node->node;

    // --- Constrain border radii ---
    float borderRadiusBl = n->borderRadiusBl;
    float borderRadiusBr = n->borderRadiusBr;
    float borderRadiusTl = n->borderRadiusTl;
    float borderRadiusTr = n->borderRadiusTr;
    float left_radii_sum   = borderRadiusTl + borderRadiusBl;
    float right_radii_sum  = borderRadiusTr + borderRadiusBr;
    float top_radii_sum    = borderRadiusTl + borderRadiusTr;
    float bottom_radii_sum = borderRadiusBl + borderRadiusBr;
    if (left_radii_sum   > n->height)  { float scale = n->height / left_radii_sum;   borderRadiusTl *= scale; borderRadiusBl *= scale; }
    if (right_radii_sum  > n->height)  { float scale = n->height / right_radii_sum;  borderRadiusTr *= scale; borderRadiusBr *= scale; }
    if (top_radii_sum    > n->width )  { float scale = n->width  / top_radii_sum;    borderRadiusTl *= scale; borderRadiusTr *= scale; }
    if (bottom_radii_sum > n->width )  { float scale = n->width  / bottom_radii_sum; borderRadiusBl *= scale; borderRadiusBr *= scale; }

    // --- Convert colors ---
    float border_r_fl      = (float)n->borderR / 255.0f;
    float border_g_fl      = (float)n->borderG / 255.0f;
    float border_b_fl      = (float)n->borderB / 255.0f;
    float bg_r_fl          = (float)n->backgroundR / 255.0f;
    float bg_g_fl          = (float)n->backgroundG / 255.0f;
    float bg_b_fl          = (float)n->backgroundB / 255.0f;

    // --- Determine corner points ---
    int max_pts            = 32;
    int tl_pts             = borderRadiusTl < 1.0f ? 1 : min((int)borderRadiusTl + 5, max_pts);
    int tr_pts             = borderRadiusTr < 1.0f ? 1 : min((int)borderRadiusTr + 5, max_pts);
    int br_pts             = borderRadiusBr < 1.0f ? 1 : min((int)borderRadiusBr + 5, max_pts);
    int bl_pts             = borderRadiusBl < 1.0f ? 1 : min((int)borderRadiusBl + 5, max_pts);
    int total_pts          = tl_pts + tr_pts + br_pts + bl_pts;

    // --- Corner anchors ---
    vec2 tl_a              = { (float)(int)(n->x + borderRadiusTl),               (float)(int)(n->y + borderRadiusTl) };
    vec2 tr_a              = { (float)(int)(n->x + n->width - borderRadiusTr), (float)(int)(n->y + borderRadiusTr) };
    vec2 bl_a              = { (float)(int)(n->x + borderRadiusBl),               (float)(int)(n->y + n->height - borderRadiusBl) };
    vec2 br_a              = { (float)(int)(n->x + n->width - borderRadiusBr), (float)(int)(n->y + n->height - borderRadiusBr) };

    // --- Allocate extra space in vertex and index lists ---
    u32 additional_vertices = (node->layoutFlags & HIDE_BACKGROUND) ? total_pts * 2 + 4 : total_pts * 3 + 4;         // each corner contributes 3*cp + 1 verts
    u32 additional_indices = (total_pts - 4) * 6                                                                     // curved edges
                                  + 24                                                                               // straight sides
                                  + ((node->layoutFlags & HIDE_BACKGROUND) ? 0 : (total_pts - 4) * 3 + 30);          // background tris
    if (vertices->size + additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, additional_vertices);
    if (indices->size + additional_indices > indices->capacity) Index_List_Grow(indices, additional_indices);

    // --- Generate corner vertices and indices ---
    int TL = vertices->size;
    Generate_Corner_Segment(vertices, indices, tl_a, PI, 1.5f * PI, borderRadiusTl, n->borderLeft, n->borderTop, n->width, n->height, z, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, tl_pts, 0, node->layoutFlags & HIDE_BACKGROUND);
    int TR = vertices->size;
    Generate_Corner_Segment(vertices, indices, tr_a, 1.5f * PI, 2.0f * PI, borderRadiusTr, n->borderTop, n->borderRight, n->width, n->height, z, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, tr_pts, 1, node->layoutFlags & HIDE_BACKGROUND);
    int BR = vertices->size;
    Generate_Corner_Segment(vertices, indices, br_a, 0.0f, 0.5f * PI, borderRadiusBr, n->borderRight, n->borderBottom, n->width, n->height, z, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, br_pts, 2, node->layoutFlags & HIDE_BACKGROUND);
    int BL = vertices->size;
    Generate_Corner_Segment(vertices, indices, bl_a, 0.5f * PI, PI, borderRadiusBl, n->borderBottom, n->borderLeft, n->width, n->height, z, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, bl_pts, 3, node->layoutFlags & HIDE_BACKGROUND);



    // --- Fill in side indices ---
    u32* indices_write = indices->array + indices->size;

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
    if (!(node->layoutFlags & HIDE_BACKGROUND)) 
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

void Construct_BorderRect(
    float x, float y, float z,
    float width, float height,
    float borderTop, float borderBottom, float borderLeft, float borderRight,
    float radiusTl, float radiusTr, float radiusBl, float radiusBr, 
    float bgR, float bgG, float bgB, 
    float bR, float bG, float bB,
    Vertex_RGB_List* vertices, Index_List* indices
)
{
    const float PI = 3.14159265f;

    // Constrain radii
    float leftRadiiSum   = radiusTl + radiusBl;
    float rightRadiiSum  = radiusTr + radiusBr;
    float topRadiiSum    = radiusTl + radiusTr;
    float bottomRadiiSum = radiusBl + radiusBr; 
    if (leftRadiiSum   > height) { float scale = height / leftRadiiSum;   radiusTl *= scale; radiusBl *= scale; }
    if (rightRadiiSum  > height) { float scale = height / rightRadiiSum;  radiusTr *= scale; radiusBr *= scale; }
    if (topRadiiSum    > width ) { float scale = width  / topRadiiSum;    radiusTl *= scale; radiusTr *= scale; } 
    if (bottomRadiiSum > width ) { float scale = width  / bottomRadiiSum; radiusBl *= scale; radiusBr *= scale; } 

    // Determine corner points
    int max_pts  = 32;
    int tl_pts             = radiusTl < 1.0f ? 1 : min((int)radiusTl + 5, max_pts);
    int tr_pts             = radiusTr < 1.0f ? 1 : min((int)radiusTr + 5, max_pts);
    int br_pts             = radiusBr < 1.0f ? 1 : min((int)radiusBr + 5, max_pts);
    int bl_pts             = radiusBl < 1.0f ? 1 : min((int)radiusBl + 5, max_pts);
    int total_pts          = tl_pts + tr_pts + br_pts + bl_pts;

    // Corner anchors
    vec2 tl_a              = { (float)(int)(x + radiusTl),         (float)(int)(y + radiusTl) };
    vec2 tr_a              = { (float)(int)(x + width - radiusTr), (float)(int)(y + radiusTr) };
    vec2 bl_a              = { (float)(int)(x + radiusBl),         (float)(int)(y + height - radiusBl) };
    vec2 br_a              = { (float)(int)(x + width - radiusBr), (float)(int)(y + height - radiusBr) };

    // Allocate extra space in vertex and index lists
    u32 additional_vertices = total_pts * 3 + 4;              // each corner contributes 3*cp + 1 verts
    u32 additional_indices = (total_pts - 4) * 6              // curved edges
                                  + 24                        // straight sides
                                  + (total_pts - 4) * 3 + 30; // background tris
    if (vertices->size + additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, additional_vertices);
    if (indices->size + additional_indices > indices->capacity) Index_List_Grow(indices, additional_indices);

    // Generate corner vertices and indices 
    int TL = vertices->size;
    Generate_Corner_Segment(vertices, indices, tl_a, PI, 1.5f * PI, radiusTl, borderLeft, borderTop, width, height, z, bR, bG, bB, bgR, bgG, bgB, tl_pts, 0, false);
    int TR = vertices->size;
    Generate_Corner_Segment(vertices, indices, tr_a, 1.5f * PI, 2.0f * PI, radiusTr, borderTop, borderRight, width, height, z, bR, bG, bB, bgR, bgG, bgB, tr_pts, 1, false);
    int BR = vertices->size;
    Generate_Corner_Segment(vertices, indices, br_a, 0.0f, 0.5f * PI, radiusBr, borderRight, borderBottom, width, height, z, bR, bG, bB, bgR, bgG, bgB, br_pts, 2, false);
    int BL = vertices->size;
    Generate_Corner_Segment(vertices, indices, bl_a, 0.5f * PI, PI, radiusBl, borderBottom, borderLeft, width, height, z, bR, bG, bB, bgR, bgG, bgB, bl_pts, 3, false);

    // Fill in side indices
    u32* indices_write = indices->array + indices->size;

    // Top side quad
    *indices_write++ = TL + tl_pts - 1;
    *indices_write++ = TL + 2 * tl_pts - 1;
    *indices_write++ = TR;
    *indices_write++ = TL + 2 * tl_pts - 1;
    *indices_write++ = TR + tr_pts;
    *indices_write++ = TR;

    // Right side quad
    *indices_write++ = TR + tr_pts - 1;
    *indices_write++ = TR + 2 * tr_pts - 1;
    *indices_write++ = BR;
    *indices_write++ = TR + 2 * tr_pts - 1;
    *indices_write++ = BR + br_pts;
    *indices_write++ = BR;

    // Bottom side quad
    *indices_write++ = BR + br_pts - 1;
    *indices_write++ = BR + 2 * br_pts - 1;
    *indices_write++ = BL;
    *indices_write++ = BR + 2 * br_pts - 1;
    *indices_write++ = BL + bl_pts;
    *indices_write++ = BL;

    // Left side quad
    *indices_write++ = BL + bl_pts - 1;
    *indices_write++ = BL + 2 * bl_pts - 1;
    *indices_write++ = TL;
    *indices_write++ = BL + 2 * bl_pts - 1;
    *indices_write++ = TL + tl_pts;
    *indices_write++ = TL;

    // Fill in background indices
    int TL_bg_connector = TL + 3 * tl_pts; 
    int TR_bg_connector = TR + 3 * tr_pts;
    int BR_bg_connector = BR + 3 * br_pts;
    int BL_bg_connector = BL + 3 * bl_pts;

    // Central quad
    *indices_write++ = TL_bg_connector; 
    *indices_write++ = TR_bg_connector; 
    *indices_write++ = BR_bg_connector;
    *indices_write++ = TL_bg_connector; 
    *indices_write++ = BR_bg_connector; 
    *indices_write++ = BL_bg_connector;

    // Top inner quad
    *indices_write++ = TL_bg_connector; 
    *indices_write++ = TL_bg_connector - 1; 
    *indices_write++ = TR + tr_pts * 2;
    *indices_write++ = TL_bg_connector; 
    *indices_write++ = TR + tr_pts * 2;     
    *indices_write++ = TR_bg_connector;

    // Right inner quad
    *indices_write++ = TR_bg_connector; 
    *indices_write++ = TR_bg_connector - 1; 
    *indices_write++ = BR + br_pts * 2;
    *indices_write++ = TR_bg_connector; 
    *indices_write++ = BR + br_pts * 2;     
    *indices_write++ = BR_bg_connector;

    // Bottom inner quad
    *indices_write++ = BR_bg_connector; 
    *indices_write++ = BR_bg_connector - 1; 
    *indices_write++ = BL + bl_pts * 2;
    *indices_write++ = BR_bg_connector; 
    *indices_write++ = BL + bl_pts * 2;     
    *indices_write++ = BL_bg_connector;

    // Left inner quad
    *indices_write++ = BL_bg_connector; 
    *indices_write++ = BL_bg_connector - 1; 
    *indices_write++ = TL + tl_pts * 2;
    *indices_write++ = BL_bg_connector; 
    *indices_write++ = TL + tl_pts * 2;     
    *indices_write++ = TL_bg_connector;

    // Update indices size
    indices->size = (int)(indices_write - indices->array);
}


static inline u32 PackRGBA(u8 r, u8 g, u8 b, u8 a)
{
    return ((u32)r << 0) |
           ((u32)g << 8) |
           ((u32)b << 16) |
           ((u32)a << 24);
}

void Add_NodeRectRenderData(
    NodeP* node, float z, 
    float scissorX, float scissorY, float scissorW, float scissorH,
    Array* borderRects
)
{
    Node* n = &node->node;

    u8 alpha = 255;
    if (node->layoutFlags & HIDE_BACKGROUND) alpha = 0;

    BorderRectRenderData* renderData = ArrayPushEmpty(borderRects);
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
    float top_radii_sum    = borderRadiusTl + borderRadiusTr;
    float bottom_radii_sum = borderRadiusBl + borderRadiusBr;
    float left_radii_sum   = borderRadiusTl + borderRadiusBl;
    float right_radii_sum  = borderRadiusTr + borderRadiusBr;
    if (top_radii_sum    > renderData->w) { float scale = renderData->w / top_radii_sum;    borderRadiusTl *= scale; borderRadiusTr *= scale; }
    if (bottom_radii_sum > renderData->w) { float scale = renderData->w / bottom_radii_sum; borderRadiusBl *= scale; borderRadiusBr *= scale; }
    if (left_radii_sum   > renderData->h) { float scale = renderData->h / left_radii_sum;   borderRadiusTl *= scale; borderRadiusBl *= scale; }
    if (right_radii_sum  > renderData->h) { float scale = renderData->h / right_radii_sum;  borderRadiusTr *= scale; borderRadiusBr *= scale; }

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
    renderData->scissorX = scissorX;
    renderData->scissorY = scissorY;
    renderData->scissorW = scissorW;
    renderData->scissorH = scissorH;
}

void Construct_Scrollbar(
    NodeP* node, float z,
    NU_Stylesheet_Scrollbar_Style* scrollbarStyle,
    Vertex_RGB_List* vertices, Index_List* indices
)
{
    Node* n = &node->node;
    
    // Compute RGB colours
    float trackR_bg = scrollbarStyle->trackBackgroundR / 255.0f;
    float trackG_bg = scrollbarStyle->trackBackgroundG / 255.0f;
    float trackB_bg = scrollbarStyle->trackBackgroundB / 255.0f;
    float trackR_b = scrollbarStyle->trackBorderR / 255.0f;
    float trackG_b = scrollbarStyle->trackBorderG / 255.0f;
    float trackB_b = scrollbarStyle->trackBorderB / 255.0f;
    float thumbR_bg = scrollbarStyle->thumbBackgroundR / 255.0f;
    float thumbG_bg = scrollbarStyle->thumbBackgroundG / 255.0f;
    float thumbB_bg = scrollbarStyle->thumbBackgroundB / 255.0f;
    float thumbR_b = scrollbarStyle->thumbBorderR / 255.0f;
    float thumbG_b = scrollbarStyle->thumbBorderG / 255.0f;
    float thumbB_b = scrollbarStyle->thumbBorderB / 255.0f;

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

    // Track geometry
    Construct_BorderRect(
        trackX, trackY, z, trackWidth, trackHeight,
        trackBorderTop, trackBorderBottom, trackBorderLeft, trackBorderRight,
        trackRadiusTl, trackRadiusTr, trackRadiusBl, trackRadiusBr, 
        trackR_bg, trackG_bg, trackB_bg,
        trackR_b, trackG_b, trackB_b,
        vertices, indices
    );

    // Thumb geometry
    Construct_BorderRect(
        thumbX, thumbY, z, thumbWidth, thumbHeight,
        thumbBorderTop, thumbBorderBottom, thumbBorderLeft, thumbBorderRight,
        thumbRadiusTl, thumbRadiusTr, thumbRadiusBl, thumbRadiusBr, 
        thumbR_bg, thumbG_bg, thumbB_bg,
        thumbR_b, thumbG_b, thumbB_b,
        vertices, indices
    );
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