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

void Generate_Corner_Vertices(vertex* vertices, GLuint* indices, vec2 anchor, float angle_start, float angle_end, float radius, float border_thickness_start, float border_thickness_end, float r, float g, float b, int corner_segments, int vertex_offset, int index_offset)
{
    float angle_step = (angle_end - angle_start) / (corner_segments - 1);
    float outer_arc_dist_start = border_thickness_start;         
    float outer_arc_dist_end = border_thickness_end;
    float division_cache = 1.0f / (corner_segments - 1);

    // --- Optimisation -> Precomputation for sin and cos
    float current_angle = angle_start;
    float sin_current = sinf(current_angle);
    float cos_current = cosf(current_angle);
    float cos_step = cosf(angle_step);
    float sin_step = sinf(angle_step);

    // --- Generate vertices ---
    for (int i=0; i<corner_segments; i++) 
    {
        float a = angle_start + i * angle_step;
        float t = (float)i * division_cache;
        float outer_arc_dist = outer_arc_dist_start * (1.0f - t) + outer_arc_dist_end * t;
        float sin_a = sin_current * cos_step + cos_current * sin_step;
        float cos_a = cos_current * cos_step - sin_current * sin_step;
        sin_current = sin_a;
        cos_current = cos_a;
        int inner_offset = vertex_offset + i;
        int outer_offset = vertex_offset + i + corner_segments;
        vertices[inner_offset].x = anchor.x + cos_a * radius; // Inner vertex fixed radius
        vertices[inner_offset].y = anchor.y + sin_a * radius;
        vertices[inner_offset].r = r;
        vertices[inner_offset].g = g;
        vertices[inner_offset].b = b;
        vertices[outer_offset].x = anchor.x + cos_a * outer_arc_dist; // Outer vertex interpolated radius
        vertices[outer_offset].y = anchor.y + sin_a * outer_arc_dist;
        vertices[outer_offset].r = r;
        vertices[outer_offset].g = g;
        vertices[outer_offset].b = b;
    }

    // --- Generate indices ---
    for (int i=0; i<corner_segments-1; i++) 
    {
        int idx_inner0 = vertex_offset + i;
        int idx_inner1 = vertex_offset + i + 1;
        int idx_outer0 = vertex_offset + i + corner_segments;
        int idx_outer1 = vertex_offset + i + 1 + corner_segments;
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
    uint8_t r, uint8_t g, uint8_t b, float screen_width, float screen_height)
{
    float fl_r = (float)r / 255.0f;
    float fl_g = (float)g / 255.0f;
    float fl_b = (float)b / 255.0f;

    // // Compute available inner width and height
    // float inner_w = width  - border_left - border_right;
    // float inner_h = height - border_top  - border_bottom;

    // // Scale radii if too large horizontally
    // float scale_x = 1.0f;
    // float sum_top    = top_left_radius    + top_right_radius;
    // float sum_bottom = bottom_left_radius + bottom_right_radius;
    // if (sum_top > inner_w)    scale_x = fminf(scale_x, inner_w / sum_top);
    // if (sum_bottom > inner_w) scale_x = fminf(scale_x, inner_w / sum_bottom);

    // // Scale radii if too large vertically
    // float scale_y = 1.0f;
    // float sum_left  = top_left_radius    + bottom_left_radius;
    // float sum_right = top_right_radius   + bottom_right_radius;
    // if (sum_left > inner_h)  scale_y = fminf(scale_y, inner_h / sum_left);
    // if (sum_right > inner_h) scale_y = fminf(scale_y, inner_h / sum_right);

    // // Apply the smallest scaling factor
    // float scale = fminf(scale_x, scale_y);
    // if (scale < 1.0f) {
    //     top_left_radius     *= scale;
    //     top_right_radius    *= scale;
    //     bottom_left_radius  *= scale;
    //     bottom_right_radius *= scale;
    // }



    // Find corner anchors
    vec2 tl_a = { x + top_left_radius, y + top_left_radius };
    vec2 tr_a = { x + width - top_right_radius, y + top_right_radius };
    vec2 bl_a = { x + bottom_left_radius, y + height - bottom_left_radius };
    vec2 br_a = { x + width - bottom_right_radius, y + height - bottom_right_radius };

    // Constrain achor positions
    // tl_a.x = fminf(tl_a.x, tr_a.x);
    // tr_a.x = fmaxf(tr_a.x, tl_a.x);
    // bl_a.x = fminf(bl_a.x, br_a.x);
    // br_a.x = fmaxf(br_a.x, bl_a.x);
    // tl_a.x = fminf(tl_a.x, bl_a.x);
    // bl_a.x = fmaxf(bl_a.x, tl_a.x);
    // tr_a.x = fminf(tr_a.x, br_a.x);
    // br_a.x = fmaxf(br_a.x, tr_a.x);
    // tl_a.y = fminf(tl_a.y, bl_a.y);
    // bl_a.y = fmaxf(bl_a.y, tl_a.y);
    // tr_a.y = fminf(tr_a.y, br_a.y);
    // br_a.y = fmaxf(br_a.y, tr_a.y);
    // tl_a.y = fminf(tl_a.y, tr_a.y);
    // tr_a.y = fmaxf(tr_a.y, tl_a.y);
    // bl_a.y = fminf(bl_a.y, br_a.y);
    // br_a.y = fmaxf(br_a.y, bl_a.y);


    int corner_segments = 32;
    const float PI = 3.14159265f;

    // --- Allocate vertex and index arrays ---
    vertex vertices[corner_segments * 8]; // (inner + outer) * 4 corners
    GLuint indices[(corner_segments - 1) * 6 * 4 + 24];

    // Precompute vertex buffer offsets
    int TL = 0;
    int TR = corner_segments * 2; 
    int BR = corner_segments * 4;
    int BL = corner_segments * 6; 

    // Generate corner vertices and indices
    Generate_Corner_Vertices(&vertices[0], &indices[0], tl_a, PI       , 1.5f * PI, top_left_radius    , border_left  , border_top   , fl_r, fl_g, fl_b, corner_segments, TL, 0);
    Generate_Corner_Vertices(&vertices[0], &indices[0], tr_a, 1.5f * PI, 2.0f * PI, top_right_radius   , border_top   , border_right , fl_r, fl_g, fl_b, corner_segments, TR, (corner_segments - 1) * 6 * 1);
    Generate_Corner_Vertices(&vertices[0], &indices[0], br_a, 0.0f     , 0.5f * PI, bottom_right_radius, border_right , border_bottom, fl_r, fl_g, fl_b, corner_segments, BR, (corner_segments - 1) * 6 * 2);
    Generate_Corner_Vertices(&vertices[0], &indices[0], bl_a, 0.5f * PI,        PI, bottom_left_radius , border_bottom, border_left  , fl_r, fl_g, fl_b, corner_segments, BL, (corner_segments - 1) * 6 * 3);
   
    // Fill in side indices
    int idx_offset = (corner_segments - 1) * 6 * 4; // start of last 24 indices
    int corner_segs_minus_one = corner_segments - 1;
    int two_corner_segs_minus_one = corner_segments * 2 - 1;
    // --- Top side quad ---
    indices[idx_offset + 0] = TL + corner_segs_minus_one;
    indices[idx_offset + 1] = TL + two_corner_segs_minus_one;
    indices[idx_offset + 2] = TR;
    indices[idx_offset + 3] = TL + two_corner_segs_minus_one;
    indices[idx_offset + 4] = TR + corner_segments;
    indices[idx_offset + 5] = TR;
    // --- Right side quad ---
    indices[idx_offset + 6]  = TR + corner_segs_minus_one;        
    indices[idx_offset + 7]  = TR + two_corner_segs_minus_one;    
    indices[idx_offset + 8]  = BR;                              
    indices[idx_offset + 9]  = TR + two_corner_segs_minus_one;   
    indices[idx_offset + 10] = BR + corner_segments;           
    indices[idx_offset + 11] = BR;
    // --- Bottom side quad ---
    indices[idx_offset + 12] = BR + corner_segs_minus_one;
    indices[idx_offset + 13] = BR + two_corner_segs_minus_one;
    indices[idx_offset + 14] = BL;
    indices[idx_offset + 15] = BR + two_corner_segs_minus_one;
    indices[idx_offset + 16] = BL + corner_segments;
    indices[idx_offset + 17] = BL;
    // --- Left side quad ---
    indices[idx_offset + 18] = BL + corner_segs_minus_one;
    indices[idx_offset + 19] = BL + two_corner_segs_minus_one;
    indices[idx_offset + 20] = TL;
    indices[idx_offset + 21] = BL + two_corner_segs_minus_one;
    indices[idx_offset + 22] = TL + corner_segments;
    indices[idx_offset + 23] = TL;
    

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
    glDrawElements(GL_TRIANGLES, (corner_segments - 1) * 6 * 4 + 24, GL_UNSIGNED_INT, 0);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
}