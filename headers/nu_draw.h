#pragma once

#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <math.h>
#include "nu_image.h"
#include <nu_draw_structures.h>
#include <nu_shader.h>

GLuint Border_Rect_Shader_Program;
GLuint Clipped_Border_Rect_Shader_Program;
GLuint Image_Shader_Program;
GLuint border_vao, border_vbo, border_ebo;
GLuint image_vao, image_vbo, image_ebo;
GLint uBorderScreenWidthLoc, uBorderScreenHeightLoc;
GLint uClippedScreenWidthLoc, uClippedScreenHeightLoc;
GLint uClipTopLoc, uClipBottomLoc, uClipLeftLoc, uClipRightLoc;


void NU_Draw_Init()
{   
    // -------------------------------------
    // --- Border rect vert and frag shaders
    // -------------------------------------
    const char* border_rect_vertex_src =
    "#version 330 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "layout(location = 1) in vec3 aColor;\n"
    "out vec3 vColor;\n"
    "out vec2 vScreenPos;\n"
    "uniform float uScreenWidth;\n"
    "uniform float uScreenHeight;\n"
    "void main() {\n"
    "    // Convert screen position (pixels) to NDC for gl_Position\n"
    "    float ndc_x = (aPos.x / uScreenWidth) * 2.0 - 1.0;\n"
    "    float ndc_y = 1.0 - (aPos.y / uScreenHeight) * 2.0;\n"
    "    gl_Position = vec4(ndc_x, ndc_y, 0.0, 1.0);\n"
    "    vColor = aColor;\n"
    "    vScreenPos = aPos;\n"
    "}\n";

    const char* border_rect_fragment_src =
    "#version 330 core\n"
    "in vec3 vColor;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    FragColor = vec4(vColor, 1.0);\n"
    "}\n";

    const char* clipped_border_rect_fragment_src =
    "#version 330 core\n"
    "in vec3 vColor;\n"
    "in vec2 vScreenPos;\n"
    "out vec4 FragColor;\n"
    "uniform float uClipTop;\n"
    "uniform float uClipBottom;\n"
    "uniform float uClipLeft;\n"
    "uniform float uClipRight;\n"
    "void main() {\n"
    "    // Discard fragments outside the clip rectangle (in pixel space)\n"
    "    if (vScreenPos.x < uClipLeft || vScreenPos.x > uClipRight ||\n"
    "        vScreenPos.y < uClipTop  || vScreenPos.y > uClipBottom) {\n"
    "        discard;\n"
    "    } else {\n"
    "        FragColor = vec4(vColor, 1.0);\n"
    "    }\n"
    "}\n";

    // -------------------------------
    // --- Image vert and frag shaders
    // -------------------------------
    const char* image_vertex_src =
    "#version 330 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "layout(location = 1) in vec2 aUV;\n"
    "out vec2 vUV;\n"
    "void main() {\n"
    "    float ndc_x = aPos.x * 2.0 - 1.0;\n"
    "    float ndc_y = 1.0 - aPos.y * 2.0;\n"
    "    gl_Position = vec4(ndc_x, ndc_y, 0.0, 1.0);\n"
    "    vUV = aUV;\n"
    "}\n";
    const char* image_fragment_src =
    "#version 330 core\n"
    "in vec2 vUV;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTexture;\n"
    "void main() {\n"
    "    FragColor = texture(uTexture, vUV);\n"
    "}\n";

    Border_Rect_Shader_Program = Create_Shader_Program(border_rect_vertex_src, border_rect_fragment_src);
    Clipped_Border_Rect_Shader_Program = Create_Shader_Program(border_rect_vertex_src, clipped_border_rect_fragment_src);
    Image_Shader_Program = Create_Shader_Program(image_vertex_src, image_fragment_src);



    // Query uniforms once
    uBorderScreenWidthLoc  = glGetUniformLocation(Border_Rect_Shader_Program, "uScreenWidth");
    uBorderScreenHeightLoc = glGetUniformLocation(Border_Rect_Shader_Program, "uScreenHeight");
    uClippedScreenWidthLoc  = glGetUniformLocation(Clipped_Border_Rect_Shader_Program, "uScreenWidth");
    uClippedScreenHeightLoc = glGetUniformLocation(Clipped_Border_Rect_Shader_Program, "uScreenHeight");
    uClipTopLoc      = glGetUniformLocation(Clipped_Border_Rect_Shader_Program, "uClipTop");
    uClipBottomLoc   = glGetUniformLocation(Clipped_Border_Rect_Shader_Program, "uClipBottom");
    uClipLeftLoc     = glGetUniformLocation(Clipped_Border_Rect_Shader_Program, "uClipLeft");
    uClipRightLoc    = glGetUniformLocation(Clipped_Border_Rect_Shader_Program, "uClipRight");
 
    // Border VAO + buffers
    glGenVertexArrays(1, &border_vao);
    glBindVertexArray(border_vao);
    glGenBuffers(1, &border_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, border_vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_rgb), (void*)0); // x,y
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_rgb), (void*)(2 * sizeof(float))); // r,g,b
    glEnableVertexAttribArray(1);
    glGenBuffers(1, &border_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, border_ebo);
    glBindVertexArray(0);


    // Image VAO + buffers
    glGenVertexArrays(1, &image_vao);
    glBindVertexArray(image_vao);
    glGenBuffers(1, &image_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, image_vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_uv), (void*)0); // x,y
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, sizeof(vertex_uv), (void*)(2 * sizeof(float))); // r,g,b
    glEnableVertexAttribArray(1);
    glGenBuffers(1, &image_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, image_ebo);
    glBindVertexArray(0); 
}

// -------------------------------------------------
// --- Functions resposible for drawing border rects
// -------------------------------------------------
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
    struct Node* node,
    float screen_width, 
    float screen_height,
    Vertex_RGB_List* vertices, Index_List* indices
)
{
    const float PI = 3.14159265f;

    // --- Border radii ---
    float border_radius_bl = node->border_radius_bl;
    float border_radius_br = node->border_radius_br;
    float border_radius_tl = node->border_radius_tl;
    float border_radius_tr = node->border_radius_tr;
    float left_radii_sum   = border_radius_tl + border_radius_bl;
    float right_radii_sum  = border_radius_tr + border_radius_br;
    float top_radii_sum    = border_radius_tl + border_radius_tr;
    float bottom_radii_sum = border_radius_bl + border_radius_br;
    if (left_radii_sum   > node->height)  { float scale = node->height / left_radii_sum;   border_radius_tl *= scale; border_radius_bl *= scale; }
    if (right_radii_sum  > node->height)  { float scale = node->height / right_radii_sum;  border_radius_tr *= scale; border_radius_br *= scale; }
    if (top_radii_sum    > node->width )  { float scale = node->width  / top_radii_sum;    border_radius_tl *= scale; border_radius_tr *= scale; }
    if (bottom_radii_sum > node->width )  { float scale = node->width  / bottom_radii_sum; border_radius_bl *= scale; border_radius_br *= scale; }

    // --- Convert colors ---
    float border_r_fl      = (float)node->border_r / 255.0f;
    float border_g_fl      = (float)node->border_g / 255.0f;
    float border_b_fl      = (float)node->border_b / 255.0f;
    float bg_r_fl          = (float)node->background_r / 255.0f;
    float bg_g_fl          = (float)node->background_g / 255.0f;
    float bg_b_fl          = (float)node->background_b / 255.0f;

    // --- Determine corner points ---
    int max_pts            = 64;
    int tl_pts             = border_radius_tl < 1.0f ? 1 : min((int)border_radius_tl + 3, max_pts);
    int tr_pts             = border_radius_tr < 1.0f ? 1 : min((int)border_radius_tr + 3, max_pts);
    int br_pts             = border_radius_br < 1.0f ? 1 : min((int)border_radius_br + 3, max_pts);
    int bl_pts             = border_radius_bl < 1.0f ? 1 : min((int)border_radius_bl + 3, max_pts);
    int total_pts          = tl_pts + tr_pts + br_pts + bl_pts;

    // --- Corner anchors ---
    vec2 tl_a              = { floorf(node->x + border_radius_tl),               floorf(node->y + border_radius_tl) };
    vec2 tr_a              = { floorf(node->x + node->width - border_radius_tr), floorf(node->y + border_radius_tr) };
    vec2 bl_a              = { floorf(node->x + border_radius_bl),               floorf(node->y + node->height - border_radius_bl) };
    vec2 br_a              = { floorf(node->x + node->width - border_radius_br), floorf(node->y + node->height - border_radius_br) };

    // --- Allocate extra space in vertex and index lists ---
    uint32_t additional_vertices = node->hide_background ? total_pts * 2 + 4 : total_pts * 3 + 4;    // each corner contributes 3*cp + 1 verts
    uint32_t additional_indices = (total_pts - 4) * 6                                                // curved edges
                                  + 24                                                               // straight sides
                                  + (node->hide_background ? 0 : (total_pts - 4) * 3 + 30);          // background tris
    if (vertices->size + additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, additional_vertices);
    if (indices->size + additional_indices > indices->capacity) Index_List_Grow(indices, additional_indices);


    // --- Generate corner vertices and indices ---
    int TL = vertices->size;
    Generate_Corner_Segment(vertices, indices, tl_a, PI, 1.5f * PI, border_radius_tl, node->border_left, node->border_top, node->width, node->height, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, tl_pts, 0, node->hide_background);
    int TR = vertices->size;
    Generate_Corner_Segment(vertices, indices, tr_a, 1.5f * PI, 2.0f * PI, border_radius_tr, node->border_top, node->border_right, node->width, node->height, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, tr_pts, 1, node->hide_background);
    int BR = vertices->size;
    Generate_Corner_Segment(vertices, indices, br_a, 0.0f, 0.5f * PI, border_radius_br, node->border_right, node->border_bottom, node->width, node->height, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, br_pts, 2, node->hide_background);
    int BL = vertices->size;
    Generate_Corner_Segment(vertices, indices, bl_a, 0.5f * PI, PI, border_radius_bl, node->border_bottom, node->border_left, node->width, node->height, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, bl_pts, 3, node->hide_background);



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

void Construct_Scroll_Thumb(struct Node* node,
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

    float x = node->x + node->width - node->border_right - 12.0f;
    float y = node->y + node->border_top;
    float thumb_y = node->y + node->border_top + node->scroll_v;
    float w = 12.0f;
    float track_h = node->height - node->border_top - node->border_bottom;
    float inner_height_w_pad = track_h - node->pad_top - node->pad_bottom;
    float inner_proportion_of_content_height = inner_height_w_pad / node->content_height;
    float thumb_h = inner_proportion_of_content_height * track_h;


    uint32_t vertex_offset = vertices->size;

    // Background Rect TL
    vertices->array[vertex_offset + 0].x = x;
    vertices->array[vertex_offset + 0].y = y;
    vertices->array[vertex_offset + 0].r = 0.1f;
    vertices->array[vertex_offset + 0].g = 0.1f;
    vertices->array[vertex_offset + 0].b = 0.1f;

    // Background Rect TR
    vertices->array[vertex_offset + 1].x = x + w;
    vertices->array[vertex_offset + 1].y = y;
    vertices->array[vertex_offset + 1].r = 0.1f;
    vertices->array[vertex_offset + 1].g = 0.1f;
    vertices->array[vertex_offset + 1].b = 0.1f;

    // Background Rect BL
    vertices->array[vertex_offset + 2].x = x;
    vertices->array[vertex_offset + 2].y = y + track_h;
    vertices->array[vertex_offset + 2].r = 0.1f;
    vertices->array[vertex_offset + 2].g = 0.1f;
    vertices->array[vertex_offset + 2].b = 0.1f;

    // Background Rect BR
    vertices->array[vertex_offset + 3].x = x + w;
    vertices->array[vertex_offset + 3].y = y + track_h;
    vertices->array[vertex_offset + 3].r = 0.1f;
    vertices->array[vertex_offset + 3].g = 0.1f;
    vertices->array[vertex_offset + 3].b = 0.1f;

    // Background Thumb TL
    vertices->array[vertex_offset + 4].x = x;
    vertices->array[vertex_offset + 4].y = thumb_y;
    vertices->array[vertex_offset + 4].r = 0.4f;
    vertices->array[vertex_offset + 4].g = 0.8f;
    vertices->array[vertex_offset + 4].b = 0.8f;

    // Background Thumb TR
    vertices->array[vertex_offset + 5].x = x + w;
    vertices->array[vertex_offset + 5].y = thumb_y;
    vertices->array[vertex_offset + 5].r = 0.4f;
    vertices->array[vertex_offset + 5].g = 0.8f;
    vertices->array[vertex_offset + 5].b = 0.8f;

    // Background Thumb BL
    vertices->array[vertex_offset + 6].x = x;
    vertices->array[vertex_offset + 6].y = thumb_y + thumb_h;
    vertices->array[vertex_offset + 6].r = 0.4f;
    vertices->array[vertex_offset + 6].g = 0.8f;
    vertices->array[vertex_offset + 6].b = 0.8f;

    // Background Thumb BR
    vertices->array[vertex_offset + 7].x = x + w;
    vertices->array[vertex_offset + 7].y = thumb_y + thumb_h;
    vertices->array[vertex_offset + 7].r = 0.4f;
    vertices->array[vertex_offset + 7].g = 0.8f;
    vertices->array[vertex_offset + 7].b = 0.8f;

    // Indices
    uint32_t* indices_write = indices->array + indices->size;
    *indices_write++ = vertex_offset + 0;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 3;
    *indices_write++ = vertex_offset + 4;
    *indices_write++ = vertex_offset + 5;
    *indices_write++ = vertex_offset + 6;
    *indices_write++ = vertex_offset + 5;
    *indices_write++ = vertex_offset + 6;
    *indices_write++ = vertex_offset + 7;

    vertices->size += additional_vertices;
    indices->size += additional_indices;
}

void Draw_Vertex_RGB_List(Vertex_RGB_List* vertices, Index_List* indices, float screen_width, float screen_height)
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(Border_Rect_Shader_Program);
    glUniform1f(uBorderScreenWidthLoc, screen_width);
    glUniform1f(uBorderScreenHeightLoc, screen_height);
    glBindBuffer(GL_ARRAY_BUFFER, border_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices->size * sizeof(vertex_rgb), vertices->array, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, border_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->size * sizeof(GLuint), indices->array, GL_DYNAMIC_DRAW);
    glBindVertexArray(border_vao);
    glDrawElements(GL_TRIANGLES, indices->size, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Draw_Clipped_Vertex_RGB_List
(
    Vertex_RGB_List* vertices, 
    Index_List* indices, 
    float screen_width, 
    float screen_height,
    float clip_top,
    float clip_bottom,
    float clip_left,
    float clip_right
)
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(Clipped_Border_Rect_Shader_Program);
    glUniform1f(uClippedScreenWidthLoc, screen_width);
    glUniform1f(uClippedScreenHeightLoc, screen_height);
    glUniform1f(uClipTopLoc, clip_top);
    glUniform1f(uClipBottomLoc, clip_bottom);
    glUniform1f(uClipLeftLoc, clip_left);
    glUniform1f(uClipRightLoc, clip_right);
    glBindBuffer(GL_ARRAY_BUFFER, border_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices->size * sizeof(vertex_rgb), vertices->array, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, border_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->size * sizeof(GLuint), indices->array, GL_DYNAMIC_DRAW);
    glBindVertexArray(border_vao);
    glDrawElements(GL_TRIANGLES, indices->size, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}


void ClearGLErrors()
{
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {}
}

void PrintGLErrors()
{
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        printf("OpenGL Error: %d\n", error);
    }
}

// -----------------------------------------------
// --- Function resposible for drawing rect images
// -----------------------------------------------
void Draw_Image(float x, float y, float w, float h, float screen_width, float screen_height, GLuint image_handle)
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    x = floorf(x) / screen_width;
    y = floorf(y) / screen_height;
    w = floorf(w) / screen_width;
    h = floorf(h) / screen_height;

    // Mesh 
    vertex_uv vertices[4] = {
        { x,     y,     0.0f, 0.0f }, // top-left
        { x+w,   y,     1.0f, 0.0f }, // top-right
        { x,     y+h,   0.0f, 1.0f }, // bottom-left
        { x+w,   y+h,   1.0f, 1.0f }, // bottom-right
    };
    GLuint indices[6] = { 0, 1, 2, 1, 2, 3 };

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(Image_Shader_Program);
    glBindBuffer(GL_ARRAY_BUFFER, image_vbo);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(vertex_uv), vertices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, image_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), indices, GL_DYNAMIC_DRAW);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, image_handle);
    glUniform1i(glGetUniformLocation(Image_Shader_Program, "uTexture"), 0);
    glBindVertexArray(image_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}