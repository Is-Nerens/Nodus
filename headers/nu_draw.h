#pragma once

#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <math.h>
#include "nu_image.h"
#include "nu_draw_structures.h"

GLuint Border_Rect_Shader_Program;
GLuint Image_Shader_Program;
GLuint border_vao, border_vbo, border_ebo;
GLint uScreenWidthLoc, uScreenHeightLoc;


static GLuint Compile_Shader(GLenum type, const char* src) 
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(shader, 512, NULL, info);
        printf("Shader compile error: %s\n", info);
    }
    return shader;
}

static GLuint Create_Shader_Program(const char* vertex_src, const char* fragment_src) 
{
    GLuint vertexShader = Compile_Shader(GL_VERTEX_SHADER, vertex_src);
    GLuint fragmentShader = Compile_Shader(GL_FRAGMENT_SHADER, fragment_src);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(program, 512, NULL, info);
        printf("Shader link error: %s\n", info);
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

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
    "uniform float uScreenWidth;\n"
    "uniform float uScreenHeight;\n"
    "void main() {\n"
    "    float ndc_x = (aPos.x / uScreenWidth) * 2.0 - 1.0;\n"
    "    float ndc_y = 1.0 - (aPos.y / uScreenHeight) * 2.0;\n"
    "    gl_Position = vec4(ndc_x, ndc_y, 0.0, 1.0);\n"
    "    vColor = aColor;\n"
    "}\n";
    const char* border_rect_fragment_src =
    "#version 330 core\n"
    "in vec3 vColor;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    FragColor = vec4(vColor, 1.0);\n"
    "}\n";

    // -------------------------------
    // --- Image vert and frag shaders
    // -------------------------------
    const char* image_vertex_src =
    "#version 330 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "layout(location = 1) in vec2 aUV;\n"
    "out vec2 vUV;\n"
    "uniform float uScreenWidth;\n"
    "uniform float uScreenHeight;\n"
    "void main() {\n"
    "    float ndc_x = (aPos.x / uScreenWidth) * 2.0 - 1.0;\n"
    "    float ndc_y = 1.0 - (aPos.y / uScreenHeight) * 2.0;\n"
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
    Image_Shader_Program = Create_Shader_Program(image_vertex_src, image_fragment_src);



    // Query uniforms once
    uScreenWidthLoc  = glGetUniformLocation(Border_Rect_Shader_Program, "uScreenWidth");
    uScreenHeightLoc = glGetUniformLocation(Border_Rect_Shader_Program, "uScreenHeight");
    // VAO + buffers
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
    glBindVertexArray(0); // done
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
    int vertex_offset, int index_offset, 
    int corner_index, bool hide_background)
{
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
    float division_cache = 1.0f / (cp - 1);
    float current_angle = angle_start;
    float sin_current = sinf(current_angle);
    float cos_current = cosf(current_angle);
    float cos_step = cosf(angle_step);
    float sin_step = sinf(angle_step);
    const float PI = 3.14159265f;

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
    for (int i=0; i<cp; i++) 
    {
        float sin_a = sin_current * cos_step + cos_current * sin_step;
        float cos_a = cos_current * cos_step - sin_current * sin_step;
        sin_current = sin_a;
        cos_current = cos_a;
        int inner_offset = vertex_offset + i;
        int outer_offset = vertex_offset + i + cp;

        // --- Handles three possible cases for how the inner curve should be handled. This looks rediculous but is a necessary optimisation to remove 6 if statements ---
        float x_curve_factor    = inner_radius_x > 0.0f ? 1.0f : 0.0f; // Precompute curve/straight factors
        float y_curve_factor    = inner_radius_y > 0.0f ? 1.0f : 0.0f;
        float x_straight_factor = 1.0f - x_curve_factor;
        float y_straight_factor = 1.0f - y_curve_factor;
        static const float sx[4] = { -1.0f, -1.0f,  1.0f,  1.0f }; // Precompute straight edge offsets for each corner
        static const float sy[4] = { -1.0f,  1.0f,  1.0f, -1.0f };
        float inner_x_curve_term = x_curve_factor * inner_radius_x * cos_a; // Curve contributions
        float inner_y_curve_term = y_curve_factor * inner_radius_y * sin_a;
        float inner_x_straight_term = x_straight_factor * sx[corner_index] * (border_thickness_x - radius); // Straight contributions (lookup-based, branchless)
        float inner_y_straight_term = y_straight_factor * sy[corner_index] * (border_thickness_y - radius);

        // --- Inner border curve vertex_rgb ---
        vertices->array[inner_offset].x = anchor.x + inner_x_curve_term + inner_x_straight_term;
        vertices->array[inner_offset].y = anchor.y + inner_y_curve_term + inner_y_straight_term;
        vertices->array[inner_offset].r = b_r;
        vertices->array[inner_offset].g = b_g;
        vertices->array[inner_offset].b = b_b;

        // --- Outer border curve vertex_rgb ---
        vertices->array[outer_offset].x = anchor.x + cos_a * radius; // Outer vertex_rgb interpolated radius
        vertices->array[outer_offset].y = anchor.y + sin_a * radius;
        vertices->array[outer_offset].r = b_r;
        vertices->array[outer_offset].g = b_g;
        vertices->array[outer_offset].b = b_b;

        // --- Outer background vertex_rgb ---
        if (!hide_background) {
            vertices->array[outer_background_offset + i].x = vertices->array[inner_offset].x;
            vertices->array[outer_background_offset + i].y = vertices->array[inner_offset].y;
            vertices->array[outer_background_offset + i].r = bg_r;
            vertices->array[outer_background_offset + i].g = bg_g;
            vertices->array[outer_background_offset + i].b = bg_b;
        }
    }

    // --- Generate indices ---
    int bg_index_offset = index_offset + (cp-1) * 6;
    for (int i=0; i<cp-1; i++) 
    {
        // --- border curve indices ---
        int idx_inner0 = vertex_offset + i;
        int idx_inner1 = vertex_offset + i + 1;
        int idx_outer0 = vertex_offset + i + cp;
        int idx_outer1 = vertex_offset + i + 1 + cp;
        int offset = index_offset + i * 6;
        indices->array[offset + 0] = idx_inner0; // First triangle
        indices->array[offset + 1] = idx_outer0;
        indices->array[offset + 2] = idx_outer1;
        indices->array[offset + 3] = idx_inner0; // Second triangle
        indices->array[offset + 4] = idx_outer1;
        indices->array[offset + 5] = idx_inner1;

        // --- background indices ---
        int idx0 = outer_background_offset + i;
        int idx1 = outer_background_offset + i + 1;
        int idx_center = inner_background_offset;
        int bg_offset = bg_index_offset + i * 3;
        indices->array[bg_offset + 0] = idx_center;  
        indices->array[bg_offset + 1] = idx0;       
        indices->array[bg_offset + 2] = idx1;      
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

    // --- Constrain radii ---
    float left_radii_sum   = node->border_radius_tl + node->border_radius_bl;
    float right_radii_sum  = node->border_radius_tr + node->border_radius_br;
    float top_radii_sum    = node->border_radius_tl + node->border_radius_tr;
    float bottom_radii_sum = node->border_radius_bl + node->border_radius_br;
    if (left_radii_sum   > node->height)  { float scale = node->height / left_radii_sum;   node->border_radius_tl *= scale; node->border_radius_bl *= scale; }
    if (right_radii_sum  > node->height)  { float scale = node->height / right_radii_sum;  node->border_radius_tr *= scale; node->border_radius_br *= scale; }
    if (top_radii_sum    > node->width )  { float scale = node->width  / top_radii_sum;    node->border_radius_tl *= scale; node->border_radius_tr *= scale; }
    if (bottom_radii_sum > node->width )  { float scale = node->width  / bottom_radii_sum; node->border_radius_bl *= scale; node->border_radius_br *= scale; }

    // --- Convert colors ---
    float border_r_fl      = (float)node->border_r / 255.0f;
    float border_g_fl      = (float)node->border_g / 255.0f;
    float border_b_fl      = (float)node->border_b / 255.0f;
    float bg_r_fl          = (float)node->background_r / 255.0f;
    float bg_g_fl          = (float)node->background_g / 255.0f;
    float bg_b_fl          = (float)node->background_b / 255.0f;

    // --- Determine corner points ---
    int max_pts            = 64;
    int tl_pts             = node->border_radius_tl < 1.0f ? 1 : min((int)node->border_radius_tl + 3, max_pts);
    int tr_pts             = node->border_radius_tr < 1.0f ? 1 : min((int)node->border_radius_tr + 3, max_pts);
    int br_pts             = node->border_radius_br < 1.0f ? 1 : min((int)node->border_radius_br + 3, max_pts);
    int bl_pts             = node->border_radius_bl < 1.0f ? 1 : min((int)node->border_radius_bl + 3, max_pts);
    int total_pts          = tl_pts + tr_pts + br_pts + bl_pts;

    // --- Corner anchors ---
    vec2 tl_a              = { node->x + node->border_radius_tl,               node->y + node->border_radius_tl };
    vec2 tr_a              = { node->x + node->width - node->border_radius_tr, node->y + node->border_radius_tr };
    vec2 bl_a              = { node->x + node->border_radius_bl,               node->y + node->height - node->border_radius_bl };
    vec2 br_a              = { node->x + node->width - node->border_radius_br, node->y + node->height - node->border_radius_br };

    // --- Allocate extra space in vertex and index lists ---
    int vertex_offset = vertices->size;
    int index_count = indices->size;
    uint32_t additional_vertices = node->hide_background ? total_pts * 2 + 4 : total_pts * 3 + 4;    // each corner contributes 3*cp + 1 verts
    uint32_t additional_indices = (total_pts - 4) * 6                                                // curved edges
                                  + 24                                                               // straight sides
                                  + (node->hide_background ? 0 : (total_pts - 4) * 3 + 30);          // background tris
    if (vertices->size + additional_vertices > vertices->capacity) {
        Vertex_RGB_List_Grow(vertices, additional_vertices);
    }
    if (indices->size + additional_indices > indices->capacity) {
        Index_List_Grow(indices, additional_indices);
    }
    vertices->size += additional_vertices;
    indices->size += additional_indices;

    

    // --- Generate corner vertices and indices ---
    int TL = vertex_offset;
    Generate_Corner_Segment(vertices, indices, tl_a, PI, 1.5f * PI, node->border_radius_tl, node->border_left, node->border_top, node->width, node->height, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, tl_pts, vertex_offset, index_count, 0, node->hide_background);
    vertex_offset += node->hide_background ? 2 * tl_pts : 3 * tl_pts + 1;
    index_count += node->hide_background ? (tl_pts - 1) * 6 : (tl_pts - 1) * 6 + (tl_pts - 1) * 3;

    int TR = vertex_offset;
    Generate_Corner_Segment(vertices, indices, tr_a, 1.5f * PI, 2.0f * PI, node->border_radius_tr, node->border_top, node->border_right, node->width, node->height, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, tr_pts, vertex_offset, index_count, 1, node->hide_background);
    vertex_offset += node->hide_background ? 2 * tr_pts : 3 * tr_pts + 1;
    index_count += node->hide_background ? (tr_pts - 1) * 6 : (tr_pts - 1) * 6 + (tr_pts - 1) * 3;

    int BR = vertex_offset;
    Generate_Corner_Segment(vertices, indices, br_a, 0.0f, 0.5f * PI, node->border_radius_br, node->border_right, node->border_bottom, node->width, node->height, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, br_pts, vertex_offset, index_count, 2, node->hide_background);
    vertex_offset += node->hide_background ? 2 * br_pts : 3 * br_pts + 1;
    index_count += node->hide_background ? (br_pts - 1) * 6 : (br_pts - 1) * 6 + (br_pts - 1) * 3;

    int BL = vertex_offset;
    Generate_Corner_Segment(vertices, indices, bl_a, 0.5f * PI, PI, node->border_radius_bl, node->border_bottom, node->border_left, node->width, node->height, border_r_fl, border_g_fl, border_b_fl, bg_r_fl, bg_g_fl, bg_b_fl, bl_pts, vertex_offset, index_count, 3, node->hide_background);
    index_count += node->hide_background ? (bl_pts - 1) * 6 : (bl_pts - 1) * 6 + (bl_pts - 1) * 3;

    // Fill in side indices
    // --- Top side quad ---
    indices->array[index_count++] = TL + tl_pts - 1;        // Last outer vertex_rgb of TL
    indices->array[index_count++] = TL + 2 * tl_pts - 1;    // Last inner vertex_rgb of TL
    indices->array[index_count++] = TR;                               // First outer vertex_rgb of TR
    indices->array[index_count++] = TL + 2 * tl_pts - 1;    // Last inner vertex_rgb of TL
    indices->array[index_count++] = TR + tr_pts;            // First inner vertex_rgb of TR
    indices->array[index_count++] = TR;                               // First outer vertex_rgb of TR

    // --- Right side quad ---
    indices->array[index_count++] = TR + tr_pts - 1;       // Last outer vertex_rgb of TR
    indices->array[index_count++] = TR + 2 * tr_pts - 1;   // Last inner vertex_rgb of TR
    indices->array[index_count++] = BR;                              // First outer vertex_rgb of BR
    indices->array[index_count++] = TR + 2 * tr_pts - 1;   // Last inner vertex_rgb of TR
    indices->array[index_count++] = BR + br_pts;           // First inner vertex_rgb of BR
    indices->array[index_count++] = BR;                              // First outer vertex_rgb of BR

    // --- Bottom side quad ---
    indices->array[index_count++] = BR + br_pts - 1;       // Last outer vertex_rgb of BR
    indices->array[index_count++] = BR + 2 * br_pts - 1;   // Last inner vertex_rgb of BR
    indices->array[index_count++] = BL;                              // First outer vertex_rgb of BL
    indices->array[index_count++] = BR + 2 * br_pts - 1;   // Last inner vertex_rgb of BR
    indices->array[index_count++] = BL + bl_pts;           // First inner vertex_rgb of BL
    indices->array[index_count++] = BL;                              // First outer vertex_rgb of BL

    // --- Left side quad ---
    indices->array[index_count++] = BL + bl_pts - 1;       // Last outer vertex_rgb of BL
    indices->array[index_count++] = BL + 2 * bl_pts - 1;   // Last inner vertex_rgb of BL
    indices->array[index_count++] = TL;                              // First outer vertex_rgb of TL
    indices->array[index_count++] = BL + 2 * bl_pts - 1;   // Last inner vertex_rgb of BL
    indices->array[index_count++] = TL + tl_pts;           // First inner vertex_rgb of TL
    indices->array[index_count++] = TL;                              // First outer vertex_rgb of TL    


    // --- Central connector vertex_rgb indices (the innermost background verts from each corner) ---
    if (!node->hide_background) {
        int TL_bg_connector = TL + 3 * tl_pts; 
        int TR_bg_connector = TR + 3 * tr_pts;
        int BR_bg_connector = BR + 3 * br_pts;
        int BL_bg_connector = BL + 3 * bl_pts;

        // --- Central quad (made of 2 triangles) ---
        indices->array[index_count++] = TL_bg_connector; 
        indices->array[index_count++] = TR_bg_connector; 
        indices->array[index_count++] = BR_bg_connector;     
        indices->array[index_count++] = TL_bg_connector;
        indices->array[index_count++] = BR_bg_connector;
        indices->array[index_count++] = BL_bg_connector;

        // --- Top inner quad ---
        indices->array[index_count++] = TL_bg_connector;     // top left connector     
        indices->array[index_count++] = TL_bg_connector - 1; // last top left background corner vertex_rgb
        indices->array[index_count++] = TR + tr_pts * 2;     // first top right background corner vertex_rgb
        indices->array[index_count++] = TL_bg_connector;
        indices->array[index_count++] = TR + tr_pts * 2;
        indices->array[index_count++] = TR_bg_connector;

        // --- Right inner quad ---
        indices->array[index_count++] = TR_bg_connector;     // top right connector
        indices->array[index_count++] = TR_bg_connector - 1; // last top right background corner vertex_rgb
        indices->array[index_count++] = BR + br_pts * 2;     // first bottom right corner vertex_rgb
        indices->array[index_count++] = TR_bg_connector;
        indices->array[index_count++] = BR + br_pts * 2;
        indices->array[index_count++] = BR_bg_connector;

        // --- Bottom inner quad ---
        indices->array[index_count++] = BR_bg_connector;     // bottom right connector
        indices->array[index_count++] = BR_bg_connector - 1; // last bottom right background corner vertex_rgb
        indices->array[index_count++] = BL + bl_pts * 2;     // first bottom left background corner vertex_rgb
        indices->array[index_count++] = BR_bg_connector;
        indices->array[index_count++] = BL + bl_pts * 2;
        indices->array[index_count++] = BL_bg_connector;

        // --- Left inner quad ---
        indices->array[index_count++] = BL_bg_connector;     // bottom left connector
        indices->array[index_count++] = BL_bg_connector - 1; // last bottom left background corner vertex_rgb
        indices->array[index_count++] = TL + tl_pts * 2;     // first top left background corner vertex_rgb
        indices->array[index_count++] = BL_bg_connector;
        indices->array[index_count++] = TL + tl_pts * 2;
        indices->array[index_count++] = TL_bg_connector;
    }
}

void Draw_Vertex_RGB_List(Vertex_RGB_List* vertices, Index_List* indices, float screen_width, float screen_height)
{
    glUseProgram(Border_Rect_Shader_Program);
    glUniform1f(uScreenWidthLoc, screen_width);
    glUniform1f(uScreenHeightLoc, screen_height);
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

    x = roundf(x);
    y = roundf(y);
    w = roundf(w);
    h = roundf(h);

    // Quad vertices 
    vertex_uv verts[6] = {
        { x,     y,     0.0f, 0.0f }, // top-left
        { x+w,   y,     1.0f, 0.0f }, // top-right
        { x+w,   y+h,   1.0f, 1.0f }, // bottom-right
        { x,     y,     0.0f, 0.0f }, // top-left
        { x+w,   y+h,   1.0f, 1.0f }, // bottom-right
        { x,     y+h,   0.0f, 1.0f }  // bottom-left
    };


    ClearGLErrors();
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);

    // Position (x,y)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_uv), (void*)0);
    glEnableVertexAttribArray(0);

    // UV (u,v)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_uv), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Use image shader
    glUseProgram(Image_Shader_Program);

    // Screen uniforms (must be set each frame or cached somewhere)
    glUniform1f(glGetUniformLocation(Image_Shader_Program, "uScreenWidth"), (float)screen_width);
    glUniform1f(glGetUniformLocation(Image_Shader_Program, "uScreenHeight"), (float)screen_height);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, image_handle);
    glUniform1i(glGetUniformLocation(Image_Shader_Program, "uTexture"), 0);

    PrintGLErrors();

    // Draw
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Cleanup (or cache vao/vbo for reuse if you draw many images per frame)
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}