#pragma once
#include <nu_draw_structures.h>
#include <math.h>

void Generate_Corner_Segment(
    Vertex_RGB_List* vertices, Index_List* indices, 
    vec2 anchor, 
    float angle_start, float angle_end, 
    float radius, 
    float border_thickness_start, float border_thickness_end, 
    float width, float height,
    float b_r, float b_g, float b_b,
    float bg_r, float bg_g, float bg_b, 
    int cp, 
    int corner_index, bool hide_background)
{

    int vertex_offset = vertices->size;
    vertices->size += hide_background ? 2 * cp : 3 * cp + 1;

    if (cp == 1)
    {           
        // --- Calculate inner vertex_rgb offsets based on corner index ---
        static const float sign_lut_x[4] = { +1.0f, -1.0f, -1.0f, +1.0f };
        static const float sign_lut_y[4] = { +1.0f, +1.0f, -1.0f, -1.0f };
        static const int border_select_lut_x[4] = { 0, 1, 0, 1 }; // 0 = start, 1 = end
        static const int border_select_lut_y[4] = { 1, 0, 1, 0 }; // 0 = start, 1 = end
        float border_x = border_select_lut_x[corner_index] ? border_thickness_end : border_thickness_start;
        float border_y = border_select_lut_y[corner_index] ? border_thickness_end : border_thickness_start;
        float offset_x = sign_lut_x[corner_index] * border_x;
        float offset_y = sign_lut_y[corner_index] * border_y;
        
        // --- Set values for inner and outer border vertices ---
        int outer_index = vertex_offset + 1;
        vertices->array[vertex_offset].x = anchor.x + offset_x;
        vertices->array[vertex_offset].y = anchor.y + offset_y;
        vertices->array[vertex_offset].r = b_r;
        vertices->array[vertex_offset].g = b_g;
        vertices->array[vertex_offset].b = b_b;
        vertices->array[outer_index].x = anchor.x;
        vertices->array[outer_index].y = anchor.y;
        vertices->array[outer_index].r = b_r;
        vertices->array[outer_index].g = b_g;
        vertices->array[outer_index].b = b_b;

        // -- Set value for innser and outer background vertices ---
        if (!hide_background) {
            int bg_outer_index = outer_index + 1;
            int bg_inner_index = outer_index + 2;
            vertices->array[bg_outer_index].x = vertices->array[vertex_offset].x;
            vertices->array[bg_outer_index].y = vertices->array[vertex_offset].y;
            vertices->array[bg_outer_index].r = bg_r;
            vertices->array[bg_outer_index].g = bg_g;
            vertices->array[bg_outer_index].b = bg_b;
            vertices->array[bg_inner_index].x = anchor.x + sign_lut_x[corner_index] * width * 0.33f;
            vertices->array[bg_inner_index].y = anchor.y + sign_lut_y[corner_index] * height * 0.33f;
            vertices->array[bg_inner_index].r = bg_r;
            vertices->array[bg_inner_index].g = bg_g;
            vertices->array[bg_inner_index].b = bg_b;
        }
        return;
    }

    // --- Precomputations ---
    float angle_step = (angle_end - angle_start) / (cp - 1);
    float current_angle = angle_start;
    float sin_current = sinf(current_angle);
    float cos_current = cosf(current_angle);
    float cos_step = cosf(angle_step);
    float sin_step = sinf(angle_step);

    // --- Inside radius, axis elliptical squish and border thickness ---
    float dir_x = (corner_index == 1 || corner_index == 3) ? 1.0f : -1.0f; // left/right
    float dir_y = (corner_index == 0 || corner_index == 2) ? -1.0f : 1.0f; // bottom/top
    float border_thickness_x = (dir_x < 0) ? border_thickness_start : border_thickness_end;
    float border_thickness_y = (dir_y < 0) ? border_thickness_end : border_thickness_start;
    float inner_radius_x = radius - border_thickness_x;
    float inner_radius_y = radius - border_thickness_y;
    int outer_background_offset = vertex_offset + 2 * cp;
    int inner_background_offset = outer_background_offset + cp;

    // --- Create inner background vertex_rgb ---
    if (!hide_background) {
        static const float sign_lut_x[4] = { +1.0f, -1.0f, -1.0f, +1.0f };
        static const float sign_lut_y[4] = { +1.0f, +1.0f, -1.0f, -1.0f };
        vertices->array[inner_background_offset].x = anchor.x;
        vertices->array[inner_background_offset].y = anchor.y;
        vertices->array[inner_background_offset].r = bg_r;
        vertices->array[inner_background_offset].g = bg_g;
        vertices->array[inner_background_offset].b = bg_b;
    }

    // --- Generate vertices ---
    // --- Handles three possible cases for how the inner curve should be handled. This looks rediculous but is a necessary optimisation to remove 6 if statements ---
    float x_curve_factor    = inner_radius_x > 0.0f ? 1.0f : 0.0f; // Precompute curve/straight factors
    float y_curve_factor    = inner_radius_y > 0.0f ? 1.0f : 0.0f;
    float x_straight_factor = 1.0f - x_curve_factor;
    float y_straight_factor = 1.0f - y_curve_factor;
    static const float sx[4] = { -1.0f, -1.0f,  1.0f,  1.0f }; // Precompute straight edge offsets for each corner
    static const float sy[4] = { -1.0f,  1.0f,  1.0f, -1.0f };
    float inner_x_straight_term = x_straight_factor * sx[corner_index] * (border_thickness_x - radius); // Straight contributions (lookup-based, branchless)
    float inner_y_straight_term = y_straight_factor * sy[corner_index] * (border_thickness_y - radius);
    vertex_rgb* inner_ptr = &vertices->array[vertex_offset];
    vertex_rgb* outer_ptr = &vertices->array[vertex_offset + cp];
    vertex_rgb* outer_bg_ptr = hide_background ? NULL : &vertices->array[outer_background_offset];
    for (int i=0; i<cp; i++) 
    {
        // --- Rotate angle (sine/cosine) ---
        float sin_a = sin_current * cos_step + cos_current * sin_step;
        float cos_a = cos_current * cos_step - sin_current * sin_step;
        sin_current = sin_a;
        cos_current = cos_a;

        // --- Compute inner vertex ---
        float inner_x = anchor.x + inner_radius_x * cos_a * x_curve_factor + inner_x_straight_term;
        float inner_y = anchor.y + inner_radius_y * sin_a * y_curve_factor + inner_y_straight_term;
        inner_ptr[i].x = inner_x;
        inner_ptr[i].y = inner_y;
        inner_ptr[i].r = b_r;
        inner_ptr[i].g = b_g;
        inner_ptr[i].b = b_b;

        // --- Compute outer vertex ---
        outer_ptr[i].x = anchor.x + cos_a * radius;
        outer_ptr[i].y = anchor.y + sin_a * radius;
        outer_ptr[i].r = b_r;
        outer_ptr[i].g = b_g;
        outer_ptr[i].b = b_b;

        // --- Compute background vertex ---
        if (!hide_background) {
            outer_bg_ptr[i].x = inner_x;
            outer_bg_ptr[i].y = inner_y;
            outer_bg_ptr[i].r = bg_r;
            outer_bg_ptr[i].g = bg_g;
            outer_bg_ptr[i].b = bg_b;
        }
    }

    // --- Generate indices ---
    int index_offset = indices->size;
    indices->size += hide_background ? (cp - 1) * 6 : (cp - 1) * 6 + (cp - 1) * 3;
    int bg_offset = index_offset + (cp-1) * 6;
    for (int i = 0; i < cp - 1; i++) 
    {
        // --- Border curve indices ---
        int idx_inner0 = vertex_offset + i;
        int idx_inner1 = idx_inner0 + 1;
        int idx_outer0 = idx_inner0 + cp;
        int idx_outer1 = idx_outer0 + 1;
        indices->array[index_offset + 0] = idx_inner0;
        indices->array[index_offset + 1] = idx_outer0;
        indices->array[index_offset + 2] = idx_outer1;
        indices->array[index_offset + 3] = idx_inner0;
        indices->array[index_offset + 4] = idx_outer1;
        indices->array[index_offset + 5] = idx_inner1;
        index_offset += 6;

        // --- Background indices ---
        int idx0 = outer_background_offset + i;
        int idx1 = idx0 + 1;
        indices->array[bg_offset + 0] = inner_background_offset;
        indices->array[bg_offset + 1] = idx0;
        indices->array[bg_offset + 2] = idx1;
        bg_offset += 3;
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
    uint32_t additional_vertices = node->hide_background ? total_pts * 2 + 4 : total_pts * 3 + 4;    // each corner contributes 3*cp + 1 verts
    uint32_t additional_indices = (total_pts - 4) * 6                                                // curved edges
                                  + 24                                                               // straight sides
                                  + (node->hide_background ? 0 : (total_pts - 4) * 3 + 30);          // background tris
    if (vertices->size + additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, additional_vertices);
    if (indices->size + additional_indices > indices->capacity) Index_List_Grow(indices, additional_indices);


    // --- Generate corner vertices and indices ---
    int TL = vertices->size;
    Generate_Corner_Segment(vertices, indices, tl_a, PI, 1.5f * PI, borderRadiusTl, node->borderLeft, node->borderTop, node->width, node->height, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, tl_pts, 0, node->hide_background);
    int TR = vertices->size;
    Generate_Corner_Segment(vertices, indices, tr_a, 1.5f * PI, 2.0f * PI, borderRadiusTr, node->borderTop, node->borderRight, node->width, node->height, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, tr_pts, 1, node->hide_background);
    int BR = vertices->size;
    Generate_Corner_Segment(vertices, indices, br_a, 0.0f, 0.5f * PI, borderRadiusBr, node->borderRight, node->borderBottom, node->width, node->height, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, br_pts, 2, node->hide_background);
    int BL = vertices->size;
    Generate_Corner_Segment(vertices, indices, bl_a, 0.5f * PI, PI, borderRadiusBl, node->borderBottom, node->borderLeft, node->width, node->height, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, bl_pts, 3, node->hide_background);



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
    if (!node->hide_background) 
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