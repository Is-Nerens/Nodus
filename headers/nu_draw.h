#pragma once

#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <math.h>

typedef struct {
    float x;
    float y;
} vec2;

typedef struct {
    float x; 
    float y;
    float r;
    float g;
    float b;
} vertex;



GLuint Rect_Shader_Program;

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
    const char* vertex_src =
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

    const char* fragment_src =
    "#version 330 core\n"
    "in vec3 vColor;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    FragColor = vec4(vColor, 1.0);\n"
    "}\n";

    Rect_Shader_Program = Create_Shader_Program(vertex_src, fragment_src);
}

void Generate_Corner_Vertices(
    vertex* vertices, GLuint* indices, 
    vec2 anchor, 
    float angle_start, float angle_end, 
    float radius, 
    float border_thickness_start, float border_thickness_end, 
    float r, float g, float b, 
    int corner_points, 
    int vertex_offset, int index_offset, 
    int corner_index)
{
    if (corner_points == 1)
    {           
        // Calculate inner vertex offsets based on corner index
        static const float sign_lut_x[4] = { +1.0f, -1.0f, -1.0f, +1.0f };
        static const float sign_lut_y[4] = { +1.0f, +1.0f, -1.0f, -1.0f };
        static const int border_select_lut_x[4] = { 0, 1, 0, 1 }; // 0 = start, 1 = end
        static const int border_select_lut_y[4] = { 1, 0, 1, 0 }; // 0 = start, 1 = end
        float border_x = border_select_lut_x[corner_index] ? border_thickness_end : border_thickness_start;
        float border_y = border_select_lut_y[corner_index] ? border_thickness_end : border_thickness_start;
        float offset_x = sign_lut_x[corner_index] * border_x;
        float offset_y = sign_lut_y[corner_index] * border_y;
        
        // Set values for inner and outer vertices
        int outer_index = vertex_offset + 1;
        vertices[vertex_offset].x = anchor.x + offset_x;
        vertices[vertex_offset].y = anchor.y + offset_y;
        vertices[vertex_offset].r = r;
        vertices[vertex_offset].g = g;
        vertices[vertex_offset].b = b;
        vertices[outer_index].x = anchor.x;
        vertices[outer_index].y = anchor.y;
        vertices[outer_index].r = r;
        vertices[outer_index].g = g;
        vertices[outer_index].b = b;
        return;
    }

    // --- Optimisation -> Precomputations
    float angle_step = (angle_end - angle_start) / (corner_points - 1);
    float division_cache = 1.0f / (corner_points - 1);
    float current_angle = angle_start;
    float sin_current = sinf(current_angle);
    float cos_current = cosf(current_angle);
    float cos_step = cosf(angle_step);
    float sin_step = sinf(angle_step);
    const float PI = 3.14159265f;

    // Determine border thickness for each axis:
    // - x-axis uses the vertical border (left/right)
    // - y-axis uses the horizontal border (top/bottom)
    // Calculate inner radii for elliptical arc
    float dir_x = (corner_index == 1 || corner_index == 3) ? 1.0f : -1.0f; // left/right
    float dir_y = (corner_index == 0 || corner_index == 2) ? -1.0f : 1.0f; // bottom/top
    float border_thickness_x = (dir_x < 0) ? border_thickness_start : border_thickness_end;
    float border_thickness_y = (dir_y < 0) ? border_thickness_end : border_thickness_start;
    float inner_radius_x = radius - border_thickness_x;
    float inner_radius_y = radius - border_thickness_y;


    // --- Generate vertices ---
    for (int i=0; i<corner_points; i++) 
    {
        float sin_a = sin_current * cos_step + cos_current * sin_step;
        float cos_a = cos_current * cos_step - sin_current * sin_step;
        sin_current = sin_a;
        cos_current = cos_a;
        int inner_offset = vertex_offset + i;
        int outer_offset = vertex_offset + i + corner_points;

        // Handles three possible cases for how the inner curve should be handled. This looks rediculous but is a necessary optimisation to remove 6 if statements
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

        // Final vertex
        vertices[inner_offset].x = anchor.x + inner_x_curve_term + inner_x_straight_term;
        vertices[inner_offset].y = anchor.y + inner_y_curve_term + inner_y_straight_term;

        // Inner curve vertex colour
        vertices[inner_offset].r = r;
        vertices[inner_offset].g = g;
        vertices[inner_offset].b = b;

        // Outer curve position and colour
        vertices[outer_offset].x = anchor.x + cos_a * radius; // Outer vertex interpolated radius
        vertices[outer_offset].y = anchor.y + sin_a * radius;
        vertices[outer_offset].r = r;
        vertices[outer_offset].g = g;
        vertices[outer_offset].b = b;
    }

    // --- Generate indices ---
    for (int i=0; i<corner_points-1; i++) 
    {
        int idx_inner0 = vertex_offset + i;
        int idx_inner1 = vertex_offset + i + 1;
        int idx_outer0 = vertex_offset + i + corner_points;
        int idx_outer1 = vertex_offset + i + 1 + corner_points;
        int offset = index_offset + i * 6;
        indices[offset + 0] = idx_inner0; // First triangle
        indices[offset + 1] = idx_outer0;
        indices[offset + 2] = idx_outer1;
        indices[offset + 3] = idx_inner0; // Second triangle
        indices[offset + 4] = idx_outer1;
        indices[offset + 5] = idx_inner1;
    }
}

void Draw_Varying_Rounded_Rect(
    float x, float y,
    float width, float height, 
    float border_top, float border_bottom, float border_left, float border_right, 
    float top_left_radius, float top_right_radius, float bottom_left_radius, float bottom_right_radius, 
    unsigned char r, unsigned char g, unsigned char b, float screen_width, float screen_height)
{
    float fl_r = (float)r / 255.0f;
    float fl_g = (float)g / 255.0f;
    float fl_b = (float)b / 255.0f;

    int max_corner_points = 64;
    int tl_corner_points = 1;
    int tr_corner_points = 1;
    int br_corner_points = 1;
    int bl_corner_points = 1;
    if (top_left_radius < 1.0f) {
        top_left_radius = 0.0f;
    } else {
        tl_corner_points = min((int)top_left_radius + 3, max_corner_points);
    }
    if (top_right_radius < 1.0f) {
        top_right_radius = 0.0f;
    } else {
        tr_corner_points = min((int)top_right_radius + 3, max_corner_points);
    }
    if (bottom_left_radius < 1.0f) {
        bottom_left_radius = 0.0f;
    } else {
        bl_corner_points = min((int)bottom_left_radius + 3, max_corner_points);
    }
    if (bottom_right_radius < 1.0f) {
        bottom_right_radius = 0.0f;
    } else {
        br_corner_points = min((int)bottom_right_radius + 3, max_corner_points);
    }

    // Find corner anchors
    vec2 tl_a = { x + top_left_radius, y + top_left_radius };
    vec2 tr_a = { x + width - top_right_radius, y + top_right_radius };
    vec2 bl_a = { x + bottom_left_radius, y + height - bottom_left_radius };
    vec2 br_a = { x + width - bottom_right_radius, y + height - bottom_right_radius };

    // --- Allocate vertex and index arrays ---
    vertex vertices[(tl_corner_points + tr_corner_points + br_corner_points + bl_corner_points) * 2]; 
    GLuint indices[(tl_corner_points + tr_corner_points + br_corner_points + bl_corner_points - 4) * 6 * 4 + 24];

    // Generate corner vertices and indices
    int vertex_offset = 0;
    int index_offset = 0;
    const float PI = 3.14159265f;

    int TL = vertex_offset;
    Generate_Corner_Vertices(&vertices[0], &indices[0], tl_a, PI, 1.5f * PI, top_left_radius, border_left, border_top, fl_r, fl_g, fl_b, tl_corner_points, vertex_offset, index_offset, 0);
    vertex_offset += 2 * tl_corner_points;
    index_offset += (tl_corner_points - 1) * 6;

    int TR = vertex_offset;
    Generate_Corner_Vertices(&vertices[0], &indices[0], tr_a, 1.5f * PI, 2.0f * PI, top_right_radius, border_top, border_right, fl_r, fl_g, fl_b, tr_corner_points, vertex_offset, index_offset, 1);
    vertex_offset += 2 * tr_corner_points;
    index_offset += (tr_corner_points - 1) * 6;

    int BR = vertex_offset;
    Generate_Corner_Vertices(&vertices[0], &indices[0], br_a, 0.0f, 0.5f * PI, bottom_right_radius, border_right, border_bottom, fl_r, fl_g, fl_b, br_corner_points, vertex_offset, index_offset, 2);
    vertex_offset += 2 * br_corner_points;
    index_offset += (br_corner_points - 1) * 6;

    int BL = vertex_offset;
    Generate_Corner_Vertices(&vertices[0], &indices[0], bl_a, 0.5f * PI, PI, bottom_left_radius, border_bottom, border_left, fl_r, fl_g, fl_b, bl_corner_points, vertex_offset, index_offset, 3);
    index_offset += (bl_corner_points - 1) * 6;

    // Fill in side indices
    // --- Top side quad ---
    indices[index_offset + 0] = TL + tl_corner_points - 1;        // Last outer vertex of TL
    indices[index_offset + 1] = TL + 2 * tl_corner_points - 1;    // Last inner vertex of TL
    indices[index_offset + 2] = TR;                               // First outer vertex of TR
    indices[index_offset + 3] = TL + 2 * tl_corner_points - 1;    // Last inner vertex of TL
    indices[index_offset + 4] = TR + tr_corner_points;            // First inner vertex of TR
    indices[index_offset + 5] = TR;                               // First outer vertex of TR

    // --- Right side quad ---
    indices[index_offset + 6]  = TR + tr_corner_points - 1;       // Last outer vertex of TR
    indices[index_offset + 7]  = TR + 2 * tr_corner_points - 1;   // Last inner vertex of TR
    indices[index_offset + 8]  = BR;                              // First outer vertex of BR
    indices[index_offset + 9]  = TR + 2 * tr_corner_points - 1;   // Last inner vertex of TR
    indices[index_offset + 10] = BR + br_corner_points;           // First inner vertex of BR
    indices[index_offset + 11] = BR;                              // First outer vertex of BR

    // --- Bottom side quad ---
    indices[index_offset + 12] = BR + br_corner_points - 1;       // Last outer vertex of BR
    indices[index_offset + 13] = BR + 2 * br_corner_points - 1;   // Last inner vertex of BR
    indices[index_offset + 14] = BL;                              // First outer vertex of BL
    indices[index_offset + 15] = BR + 2 * br_corner_points - 1;   // Last inner vertex of BR
    indices[index_offset + 16] = BL + bl_corner_points;           // First inner vertex of BL
    indices[index_offset + 17] = BL;                              // First outer vertex of BL

    // --- Left side quad ---
    indices[index_offset + 18] = BL + bl_corner_points - 1;       // Last outer vertex of BL
    indices[index_offset + 19] = BL + 2 * bl_corner_points - 1;   // Last inner vertex of BL
    indices[index_offset + 20] = TL;                              // First outer vertex of TL
    indices[index_offset + 21] = BL + 2 * bl_corner_points - 1;   // Last inner vertex of BL
    indices[index_offset + 22] = TL + tl_corner_points;           // First inner vertex of TL
    indices[index_offset + 23] = TL;                              // First outer vertex of TL
    int index_count = index_offset + 24;
    

    // --- Upload to GPU ---
    glUseProgram(Rect_Shader_Program);
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glUniform1f(glGetUniformLocation(Rect_Shader_Program, "uScreenWidth"), screen_width);
    glUniform1f(glGetUniformLocation(Rect_Shader_Program, "uScreenHeight"), screen_height);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0); // x, y
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex), (void*)(2 * sizeof(float))); // r,g,b
    glEnableVertexAttribArray(1);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
}