#pragma once

#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <math.h>
#include "nu_draw_structures.h"
#include "nu_shader.h"

GLuint BorderRectShader;
GLuint ClippedBorderRectShader;
GLuint ImageShader;
GLuint borderVao, borderVbo, borderEbo;
GLuint imageVao, imageVbo, imageEbo;
GLint uBorderScreenWidthLoc, uBorderScreenHeightLoc, uBorderOffsetXLoc, uBorderOffsetYLoc;
GLint uClippedScreenWidthLoc, uClippedScreenHeightLoc, uClippedOffsetXLoc, uClippedOffsetYLoc;
GLint uImageScreenWidthLoc, uImageScreenHeightLoc;
GLint uBorderClipTopLoc, uBorderClipBottomLoc, uBorderClipLeftLoc, uBorderClipRightLoc;
GLint uImageClipTopLoc, uImageClipBottomLoc, uImageClipLeftLoc, uImageClipRightLoc;
GLint uImageTextureLoc;


void NU_Draw_Init()
{   
    // -------------------------------------
    // --- Border rect vert and frag shaders
    // -------------------------------------
    const char* borderRectVertSrc =
    "#version 330 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "layout(location = 1) in vec3 aColor;\n"
    "out vec3 vColor;\n"
    "out vec2 vScreenPos;\n"
    "uniform float uScreenWidth;\n"
    "uniform float uScreenHeight;\n"
    "uniform float uOffsetX;\n"
    "uniform float uOffsetY;\n"
    "void main() {\n"
    "    // Convert screen position (pixels) to NDC for gl_Position\n"
    "    float ndc_x = ((aPos.x + uOffsetX) / uScreenWidth) * 2.0 - 1.0;\n"
    "    float ndc_y = 1.0 - ((aPos.y + uOffsetY) / uScreenHeight) * 2.0;\n"
    "    gl_Position = vec4(ndc_x, ndc_y, 0.0, 1.0);\n"
    "    vColor = aColor;\n"
    "    vScreenPos = vec2(aPos.x + uOffsetX, aPos.y + uOffsetY);\n"
    "}\n";

    const char* borderRectFragSrc =
    "#version 330 core\n"
    "in vec3 vColor;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    FragColor = vec4(vColor, 1.0);\n"
    "}\n";

    const char* clippedBorderRectFragSrc =
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
    const char* imageVertSrc =
    "#version 330 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "layout(location = 1) in vec2 aUV;\n"
    "out vec2 vUV;\n"
    "out vec2 vScreenPos;\n"
    "uniform float uScreenWidth;\n"
    "uniform float uScreenHeight;\n"
    "void main() {\n"
    "    // Convert screen position (pixels) to NDC for gl_Position\n"
    "    float ndc_x = (aPos.x / uScreenWidth) * 2.0 - 1.0;\n"
    "    float ndc_y = 1.0 - (aPos.y / uScreenHeight) * 2.0;\n"
    "    gl_Position = vec4(ndc_x, ndc_y, 0.0, 1.0);\n"
    "    vUV = aUV;\n"
    "    vScreenPos = aPos;\n"
    "}\n";
    const char* imageFragSrc =
    "#version 330 core\n"
    "in vec2 vUV;\n"
    "in vec2 vScreenPos;\n"
    "uniform float uClipTop;\n"
    "uniform float uClipBottom;\n"
    "uniform float uClipLeft;\n"
    "uniform float uClipRight;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTexture;\n"
    "void main() {\n"
    "    // Discard fragments outside the clip rectangle (in pixel space)\n"
    "    if (vScreenPos.x < uClipLeft || vScreenPos.x > uClipRight ||\n"
    "        vScreenPos.y < uClipTop  || vScreenPos.y > uClipBottom) {\n"
    "        discard;\n"
    "    } else {\n"
    "        FragColor = texture(uTexture, vUV);\n"
    "    }\n"
    "}\n";

    BorderRectShader = Create_Shader_Program(borderRectVertSrc, borderRectFragSrc);
    ClippedBorderRectShader = Create_Shader_Program(borderRectVertSrc, clippedBorderRectFragSrc);
    ImageShader = Create_Shader_Program(imageVertSrc, imageFragSrc);



    // Query uniforms once
    uBorderScreenWidthLoc  = glGetUniformLocation(BorderRectShader, "uScreenWidth");
    uBorderScreenHeightLoc = glGetUniformLocation(BorderRectShader, "uScreenHeight");
    uBorderOffsetXLoc      = glGetUniformLocation(BorderRectShader, "uOffsetX");
    uBorderOffsetYLoc      = glGetUniformLocation(BorderRectShader, "uOffsetY");

    uClippedScreenWidthLoc  = glGetUniformLocation(ClippedBorderRectShader, "uScreenWidth");
    uClippedScreenHeightLoc = glGetUniformLocation(ClippedBorderRectShader, "uScreenHeight");
    uClippedOffsetXLoc      = glGetUniformLocation(ClippedBorderRectShader, "uOffsetX");
    uClippedOffsetYLoc      = glGetUniformLocation(ClippedBorderRectShader, "uOffsetY");


    uImageScreenWidthLoc  = glGetUniformLocation(ImageShader, "uScreenWidth");
    uImageScreenHeightLoc = glGetUniformLocation(ImageShader, "uScreenHeight");
    uBorderClipTopLoc     = glGetUniformLocation(ClippedBorderRectShader, "uClipTop");
    uBorderClipBottomLoc  = glGetUniformLocation(ClippedBorderRectShader, "uClipBottom");
    uBorderClipLeftLoc    = glGetUniformLocation(ClippedBorderRectShader, "uClipLeft");
    uBorderClipRightLoc   = glGetUniformLocation(ClippedBorderRectShader, "uClipRight");
    uImageClipTopLoc      = glGetUniformLocation(ImageShader, "uClipTop");
    uImageClipBottomLoc   = glGetUniformLocation(ImageShader, "uClipBottom");
    uImageClipLeftLoc     = glGetUniformLocation(ImageShader, "uClipLeft");
    uImageClipRightLoc    = glGetUniformLocation(ImageShader, "uClipRight");
    uImageTextureLoc      = glGetUniformLocation(ImageShader, "uTexture");
 
    // Border VAO + buffers
    glGenVertexArrays(1, &borderVao);
    glBindVertexArray(borderVao);
    glGenBuffers(1, &borderVbo);
    glBindBuffer(GL_ARRAY_BUFFER, borderVbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_rgb), (void*)0); // x,y
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_rgb), (void*)(2 * sizeof(float))); // r,g,b
    glEnableVertexAttribArray(1);
    glGenBuffers(1, &borderEbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, borderEbo);
    glBindVertexArray(0);


    // Image VAO + buffers
    glGenVertexArrays(1, &imageVao);
    glBindVertexArray(imageVao);
    glGenBuffers(1, &imageVbo);
    glBindBuffer(GL_ARRAY_BUFFER, imageVbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_uv), (void*)0); // x,y
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_uv), (void*)(2 * sizeof(float))); // u,v
    glEnableVertexAttribArray(1);
    glGenBuffers(1, &imageEbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, imageEbo);
    glBindVertexArray(0); 
}

// -------------------------------------------------
// --- Functions resposible for drawing border rects
// -------------------------------------------------
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

void Draw_Vertex_RGB_List
(
    Vertex_RGB_List* vertices, 
    Index_List* indices, 
    float screen_width, 
    float screen_height,
    float offsetX,
    float offsetY
)
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(BorderRectShader);
    glUniform1f(uBorderScreenWidthLoc, screen_width);
    glUniform1f(uBorderScreenHeightLoc, screen_height);
    glUniform1f(uBorderOffsetXLoc, offsetX);
    glUniform1f(uBorderOffsetYLoc, offsetY);
    glBindBuffer(GL_ARRAY_BUFFER, borderVbo);
    glBufferData(GL_ARRAY_BUFFER, vertices->size * sizeof(vertex_rgb), vertices->array, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, borderEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->size * sizeof(GLuint), indices->array, GL_DYNAMIC_DRAW);
    glBindVertexArray(borderVao);
    glDrawElements(GL_TRIANGLES, indices->size, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Draw_Clipped_Vertex_RGB_List
(
    Vertex_RGB_List* vertices, 
    Index_List* indices, 
    float screen_width, 
    float screen_height,
    float offsetX,
    float offsetY,
    float clip_top,
    float clip_bottom,
    float clip_left,
    float clip_right
)
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(ClippedBorderRectShader);
    glUniform1f(uClippedScreenWidthLoc, screen_width);
    glUniform1f(uClippedScreenHeightLoc, screen_height);
    glUniform1f(uClippedOffsetXLoc, offsetX);
    glUniform1f(uClippedOffsetYLoc, offsetY);
    glUniform1f(uBorderClipTopLoc, clip_top);
    glUniform1f(uBorderClipBottomLoc, clip_bottom);
    glUniform1f(uBorderClipLeftLoc, clip_left);
    glUniform1f(uBorderClipRightLoc, clip_right);
    glBindBuffer(GL_ARRAY_BUFFER, borderVbo);
    glBufferData(GL_ARRAY_BUFFER, vertices->size * sizeof(vertex_rgb), vertices->array, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, borderEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->size * sizeof(GLuint), indices->array, GL_DYNAMIC_DRAW);
    glBindVertexArray(borderVao);
    glDrawElements(GL_TRIANGLES, indices->size, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}




// -----------------------------------------------
// --- Function resposible for drawing rect images
// -----------------------------------------------
void NU_Draw_Image(
    float x, float y, 
    float w, float h, 
    float screen_width, float screen_height,
    float clip_top, float clip_bottom, float clip_left, float clip_right, 
    GLuint image_handle)
{
    // Mesh 
    vertex_uv vertices[4] = {
        { x,     y,     0.0f, 0.0f }, // top-left
        { x+w,   y,     1.0f, 0.0f }, // top-right
        { x,     y+h,   0.0f, 1.0f }, // bottom-left
        { x+w,   y+h,   1.0f, 1.0f }, // bottom-right
    };
    GLuint indices[6] = { 0, 1, 2, 1, 2, 3 };

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(ImageShader);
    glUniform1f(uImageScreenWidthLoc, screen_width);
    glUniform1f(uImageScreenHeightLoc, screen_height);
    glUniform1f(uImageClipTopLoc, clip_top);
    glUniform1f(uImageClipBottomLoc, clip_bottom);
    glUniform1f(uImageClipLeftLoc, clip_left);
    glUniform1f(uImageClipRightLoc, clip_right);
    glBindVertexArray(imageVao);
    glBindBuffer(GL_ARRAY_BUFFER, imageVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, imageEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, image_handle);
    glUniform1i(uImageTextureLoc, 0);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}