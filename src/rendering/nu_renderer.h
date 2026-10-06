#pragma once

#include "nu_renderer_structures.h"
#include "nu_shader.h"
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <math.h>
#include <rendering/gui/nu_mesh_generation.h>
#include <rendering/nu_renderer_structures.h>
#include <rendering/nu_shader.h>

// Quad mesh
GLuint quadVao, quadVbo;

// Border rect SDF
GLuint sdfRectShader;
GLuint sdfRectVao, sdfRectVbo;
GLint uSdfRectScreenWidthLoc, uSdfRectScreenHeightLoc;
#define SDF_RECT_RENDER_DATA_GPU_RESERVE_COUNT 8000

// Image
GLuint ImageShader;
GLuint imageVao, imageVbo;
GLint uImageScreenWidthLoc, uImageScreenHeightLoc;
GLint uImageTextureLoc;
#define IMAGE_RENDER_DATA_GPU_RESERVE_COUNT 1000

// Border rects
GLuint BorderRectShader;
GLuint ClippedBorderRectShader;
GLuint borderVao, borderVbo, borderEbo;
GLint uBorderScreenWidthLoc, uBorderScreenHeightLoc, uBorderOffsetXLoc, uBorderOffsetYLoc;
GLint uClippedScreenWidthLoc, uClippedScreenHeightLoc, uClippedOffsetXLoc, uClippedOffsetYLoc;
GLint uBorderClipTopLoc, uBorderClipBottomLoc, uBorderClipLeftLoc, uBorderClipRightLoc;

// Text
GLuint TextMonoShader;
GLuint TextSubpixelShader;
GLuint ClippedTextMonoShader;
GLuint ClippedTextSubpixelShader;
GLuint TextVao, TextVbo, ClippedTextVbo;
GLuint uClippedTextVao, uClippedTextVbo;
// screen dimension uniforms
GLint uTextMonoScreenWidthLoc, uTextMonoScreenHeightLoc;
GLint uTextSubpixelScreenWidthLoc, uTextSubpixelScreenHeightLoc;
GLint uClippedTextMonoScreenWidthLoc, uClippedTextMonoScreenHeightLoc;
GLint uClippedTextSubpixelScreenWidthLoc, uClippedTextSubpixelScreenHeightLoc;
// offset uniforms
GLint uTextMonoOffsetXLoc, uTextMonoOffsetYLoc;
GLint uTextSubpixelOffsetXLoc, uTextSubpixelOffsetYLoc;
GLint uClippedTextMonoOffsetXLoc, uClippedTextMonoOffsetYLoc;
GLint uClippedTextSubpixelOffsetXLoc, uClippedTextSubpixelOffsetYLoc;
// scissor uniforms
GLint uClippedTextMonoScissorTopLoc, uClippedTextMonoScissorBottomLoc;
GLint uClippedTextMonoScissorLeftLoc, uClippedTextMonoScissorRightLoc;
GLint uClippedTextSubpixelScissorTopLoc, uClippedTextSubpixelScissorBottomLoc;
GLint uClippedTextSubpixelScissorLeftLoc, uClippedTextSubpixelScissorRightLoc;
// texture uniforms
GLint uTextMonoFontTextureLoc, uTextSubpixelFontTextureLoc;
GLint uClippedTextMonoFontTextureLoc, uClippedTextSubpixelFontTextureLoc;
#define TEXT_RENDER_DATA_GPU_RESERVE_COUNT 12000

void NU_Init_Quad()
{
    float quad[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    // Vao
    glGenVertexArrays(1, &quadVao);
    glBindVertexArray(quadVao);

    // Vbo
    glGenBuffers(1, &quadVbo);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
}

void NU_Init_SDF_Border_Rect_Shader()
{
    const char* sdfRect_VertSrc =
    "#version 330 core\n"
    "layout(location = 0) in vec2 aQuad;\n"
    "layout(location = 1) in vec3 iPos;\n"
    "layout(location = 2) in vec2 iSize;\n"
    "layout(location = 3) in uint iBgColor;\n"
    "layout(location = 4) in uint iBorderColor;\n"
    "layout(location = 5) in vec4 iRadius;\n"
    "layout(location = 6) in vec4 iBorder;\n"
    "layout(location = 7) in vec4 iScissor;\n"
    "\n"
    "out vec2 vLocalPos;\n"
    "out vec2 vScreenPos;\n"
    "out vec2 vSize;\n"
    "out vec4 vRadius;\n"
    "out vec4 vBorder;\n"
    "out vec4 vScissor;\n"
    "out vec4 vBgColor;\n"
    "out vec4 vBorderColor;\n"
    "\n"
    "uniform float uScreenWidth;\n"
    "uniform float uScreenHeight;\n"
    "\n"
    "vec4 unpackRGBA(uint c)\n"
    "{\n"
    "    return vec4(\n"
    "        float((c >> 0) & 255u) / 255.0,\n"
    "        float((c >> 8) & 255u) / 255.0,\n"
    "        float((c >> 16) & 255u) / 255.0,\n"
    "        float((c >> 24) & 255u) / 255.0\n"
    "    );\n"
    "}\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vScreenPos = iPos.xy + aQuad * iSize;\n"
    "    float ndc_x = (vScreenPos.x / uScreenWidth) * 2.0 - 1.0;\n"
    "    float ndc_y = 1.0 - (vScreenPos.y / uScreenHeight) * 2.0;\n"
    "    gl_Position = vec4(ndc_x, ndc_y, iPos.z * 0.015625f, 1.0);\n"
    "    vLocalPos = aQuad * iSize;\n"
    "    vSize = iSize;\n"
    "    vRadius = iRadius;\n"
    "    vBorder = iBorder;\n"
    "    vScissor = iScissor;\n"
    "    vBgColor = unpackRGBA(iBgColor);\n"
    "    vBorderColor = unpackRGBA(iBorderColor);\n"
    "}\n";

    const char* sdfRect_FragSrc =
    "#version 330 core\n"
    "in vec2 vLocalPos;\n"
    "in vec2 vScreenPos;\n"
    "in vec2 vSize;\n"
    "in vec4 vRadius;\n"
    "in vec4 vBorder;\n"
    "in vec4 vScissor;\n"
    "in vec4 vBgColor;\n"
    "in vec4 vBorderColor;\n"
    "out vec4 FragColor;\n"
    "\n"
    "float sdRoundRect(vec2 p, vec2 b, float r)\n"
    "{\n"
    "    vec2 q = abs(p) - b + r;\n"
    "    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;\n"
    "}\n"
    "\n"
    "void main()\n"
    "{\n"
    "    // ---- SCISSOR (world/screen space) ----\n"
    "    if (vScreenPos.x < vScissor.z ||\n"
    "        vScreenPos.y < vScissor.x ||\n"
    "        vScreenPos.x > vScissor.w ||\n"
    "        vScreenPos.y > vScissor.y)\n"
    "    {\n"
    "        discard;\n"
    "    }\n"
    "    // ---- SDF (local space, centered at 0) ----\n"
    "    vec2 p = vLocalPos - vSize * 0.5;\n"
    "    vec2 halfSize = vSize * 0.5;\n"
    "    // ---- PER-CORNER OUTER RADIUS ----\n"
    "    float outerR = p.x > 0.0 ? (p.y > 0.0 ? vRadius.w : vRadius.y)\n"
    "                             : (p.y > 0.0 ? vRadius.z : vRadius.x);\n"
    "    // ---- PER-CORNER INNER RADIUS (reduced by adjacent border thickness) ----\n"
    "    float innerR = p.x > 0.0 ? (p.y > 0.0 ? max(vRadius.w - max(vBorder.y, vBorder.w), 0.0)\n"
    "                                           : max(vRadius.y - max(vBorder.x, vBorder.w), 0.0))\n"
    "                             : (p.y > 0.0 ? max(vRadius.z - max(vBorder.y, vBorder.z), 0.0)\n"
    "                                          : max(vRadius.x - max(vBorder.x, vBorder.z), 0.0));\n"
    "    // ---- OUTER SHAPE ----\n"
    "    float outer = sdRoundRect(p, halfSize, outerR);\n"
    "    // ---- INNER SHAPE (border inset) ----\n"
    "    vec2 innerOffset = vec2(vBorder.z - vBorder.w, vBorder.x - vBorder.y) * 0.5;\n"
    "    vec2 innerHalf = halfSize - vec2(vBorder.z + vBorder.w, vBorder.x + vBorder.y) * 0.5;\n"
    "    innerHalf = max(innerHalf, vec2(0.0));\n"
    "    float inner = sdRoundRect(p - innerOffset, innerHalf, innerR);\n"
    "    // ---- MASKS ----\n"
    "    float outerMask = smoothstep(0.5, -0.5, outer);\n"
    "    float innerMask = (innerHalf.x > 0.0 && innerHalf.y > 0.0) ? smoothstep(0.5, -0.5, inner) : 1.0;\n"
    "    float fillMask   = innerMask * vBgColor.a;\n"
    "    float borderMask = clamp(outerMask - innerMask, 0.0, 1.0);\n"
    "    // ---- COLOR ----\n"
    "    vec3 color = (borderMask > fillMask) ? vBorderColor.rgb : vBgColor.rgb;\n"
    "    float alpha = clamp(fillMask + borderMask, 0.0, 1.0);\n"
    "    if (alpha <= 0.0) discard;\n"
    "    FragColor = vec4(color, alpha);\n"
    "}\n";


    sdfRectShader = Create_Shader_Program(sdfRect_VertSrc, sdfRect_FragSrc);
    uSdfRectScreenWidthLoc  = glGetUniformLocation(sdfRectShader, "uScreenWidth");
    uSdfRectScreenHeightLoc = glGetUniformLocation(sdfRectShader, "uScreenHeight");

    // SDF rect Vao
    glGenVertexArrays(1, &sdfRectVao);
    glBindVertexArray(sdfRectVao);

    // Bind quad mesh
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 2,
        GL_FLOAT, GL_FALSE,
        2 * sizeof(float),
        (void*)0
    );

    // SDF Rect instance data
    glGenBuffers(1, &sdfRectVbo);
    glBindBuffer(GL_ARRAY_BUFFER, sdfRectVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(BorderRectRenderData) * SDF_RECT_RENDER_DATA_GPU_RESERVE_COUNT, NULL, GL_STREAM_DRAW);

    // Position (x, y, z)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(BorderRectRenderData), (void*)offsetof(BorderRectRenderData, x));
    glVertexAttribDivisor(1, 1);

    // Size (w, h)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(BorderRectRenderData), (void*)offsetof(BorderRectRenderData, w));
    glVertexAttribDivisor(2, 1);

    // Background RGBA
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(BorderRectRenderData), (void*)offsetof(BorderRectRenderData, backgroundRGBA));
    glVertexAttribDivisor(3, 1);

    // Border RGBA
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT, sizeof(BorderRectRenderData), (void*)offsetof(BorderRectRenderData, borderRGBA));
    glVertexAttribDivisor(4, 1);

    // Radii
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(BorderRectRenderData), (void*)offsetof(BorderRectRenderData, radiusTl));
    glVertexAttribDivisor(5, 1);

    // Border thickness
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(BorderRectRenderData), (void*)offsetof(BorderRectRenderData, borderTop));
    glVertexAttribDivisor(6, 1);

    // Scissor
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(BorderRectRenderData), (void*)offsetof(BorderRectRenderData, scissorTop));
    glVertexAttribDivisor(7, 1);

    glBindVertexArray(0);
}

void NU_Init_Mesh_Border_Rect_Shader()
{
    const char* borderRectVertSrc =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
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
    "    gl_Position = vec4(ndc_x, ndc_y, aPos.z * 0.015625f, 1.0);\n"
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
    // Discard fragments outside the clip rectangle (in pixel space)\n"
    "    if (vScreenPos.x < uClipLeft || vScreenPos.x > uClipRight ||\n"
    "        vScreenPos.y < uClipTop  || vScreenPos.y > uClipBottom) {\n"
    "        discard;\n"
    "    } else {\n"
    "        FragColor = vec4(vColor, 1.0);\n"
    "    }\n"
    "}\n";

    BorderRectShader = Create_Shader_Program(borderRectVertSrc, borderRectFragSrc);
    ClippedBorderRectShader = Create_Shader_Program(borderRectVertSrc, clippedBorderRectFragSrc);

    uBorderScreenWidthLoc  = glGetUniformLocation(BorderRectShader, "uScreenWidth");
    uBorderScreenHeightLoc = glGetUniformLocation(BorderRectShader, "uScreenHeight");
    uBorderOffsetXLoc      = glGetUniformLocation(BorderRectShader, "uOffsetX");
    uBorderOffsetYLoc      = glGetUniformLocation(BorderRectShader, "uOffsetY");
    uClippedScreenWidthLoc  = glGetUniformLocation(ClippedBorderRectShader, "uScreenWidth");
    uClippedScreenHeightLoc = glGetUniformLocation(ClippedBorderRectShader, "uScreenHeight");
    uClippedOffsetXLoc      = glGetUniformLocation(ClippedBorderRectShader, "uOffsetX");
    uClippedOffsetYLoc      = glGetUniformLocation(ClippedBorderRectShader, "uOffsetY");
    uBorderClipTopLoc     = glGetUniformLocation(ClippedBorderRectShader, "uClipTop");
    uBorderClipBottomLoc  = glGetUniformLocation(ClippedBorderRectShader, "uClipBottom");
    uBorderClipLeftLoc    = glGetUniformLocation(ClippedBorderRectShader, "uClipLeft");
    uBorderClipRightLoc   = glGetUniformLocation(ClippedBorderRectShader, "uClipRight");

    glGenVertexArrays(1, &borderVao);
    glBindVertexArray(borderVao);
    glGenBuffers(1, &borderVbo);
    glBindBuffer(GL_ARRAY_BUFFER, borderVbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_rgb), (void*)0); // x,y,z
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_rgb), (void*)(3 * sizeof(float))); // r,g,b
    glEnableVertexAttribArray(1);
    glGenBuffers(1, &borderEbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, borderEbo);
    glBindVertexArray(0);
}

void NU_Init_Image_Shader()
{
    const char* imageVertSrc =
    "#version 330 core\n"
    "layout(location = 0) in vec2 aQuad;\n"
    "layout(location = 1) in vec3 iPos;\n"
    "layout(location = 2) in vec2 iSize;\n"
    "layout(location = 3) in vec4 iUV;\n"
    "layout(location = 4) in vec4 iScissor;\n"
    "\n"
    "out vec2 vUV;\n"
    "out vec2 vScreenPos;\n"
    "out vec4 vScissor;\n"
    "\n"
    "uniform float uScreenWidth;\n"
    "uniform float uScreenHeight;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vec2 worldPos = iPos.xy + aQuad * iSize;\n"
    "    float ndc_x = (worldPos.x / uScreenWidth) * 2.0 - 1.0;\n"
    "    float ndc_y = 1.0 - (worldPos.y / uScreenHeight) * 2.0;\n"
    "    gl_Position = vec4(ndc_x, ndc_y, iPos.z * 0.015625f, 1.0);\n"
    "    vScreenPos = worldPos;\n"
    "    vScissor = iScissor;\n"
    "    vUV = vec2(\n"
    "        mix(iUV.x, iUV.z, aQuad.x),\n"
    "        mix(iUV.y, iUV.w, aQuad.y)\n"
    "    );\n"
    "}\n";

    const char* imageFragSrc =
    "#version 330 core\n"
    "in vec2 vUV;\n"
    "in vec2 vScreenPos;\n"
    "in vec4 vScissor;\n"
    "out vec4 FragColor;\n"
    "\n"
    "uniform sampler2D uTexture;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    if (vScreenPos.x < vScissor.z ||\n"
    "        vScreenPos.y < vScissor.x ||\n"
    "        vScreenPos.x > vScissor.w ||\n"
    "        vScreenPos.y > vScissor.y)\n"
    "    {\n"
    "        discard;\n"
    "    }\n"
    "    FragColor = texture(uTexture, vUV);\n"
    "}\n";

    ImageShader = Create_Shader_Program(imageVertSrc, imageFragSrc);
    uImageScreenWidthLoc  = glGetUniformLocation(ImageShader, "uScreenWidth");
    uImageScreenHeightLoc = glGetUniformLocation(ImageShader, "uScreenHeight");
    uImageTextureLoc = glGetUniformLocation(ImageShader, "uTexture");

    // Image rect Vao
    glGenVertexArrays(1, &imageVao);
    glBindVertexArray(imageVao);

    // Bind quad mesh
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 2,
        GL_FLOAT, GL_FALSE,
        2 * sizeof(float),
        (void*)0
    );

    // Image instance data
    glGenBuffers(1, &imageVbo);
    glBindBuffer(GL_ARRAY_BUFFER, imageVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ImageRenderData) * IMAGE_RENDER_DATA_GPU_RESERVE_COUNT, NULL, GL_STREAM_DRAW);

    // Position (x, y, z)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ImageRenderData), (void*)offsetof(ImageRenderData, x));
    glVertexAttribDivisor(1, 1);

    // Size (w, h)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ImageRenderData), (void*)offsetof(ImageRenderData, w));
    glVertexAttribDivisor(2, 1);

    // UVs
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(ImageRenderData), (void*)offsetof(ImageRenderData, u0));
    glVertexAttribDivisor(3, 1);

    // Scissor
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(ImageRenderData), (void*)offsetof(ImageRenderData, scissorTop));
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);
}

void NU_Init_Text_Shader()
{
    const char* textShaderVertSrc =
        "#version 330 core\n"
        "layout(location = 0) in vec2 aQuad;\n"
        "layout(location = 1) in vec3 iPos;\n"
        "layout(location = 2) in vec2 iSize;\n"
        "layout(location = 3) in vec4 iUV;\n"
        "layout(location = 4) in uint iColor;\n"
        "\n"
        "out vec2 vUV;\n"
        "out vec2 vScreenPos;\n"
        "out vec4 vColor;\n"
        "\n"
        "uniform float uScreenWidth;\n"
        "uniform float uScreenHeight;\n"
        "uniform float uOffsetX;\n"
        "uniform float uOffsetY;\n"
        "\n"
        "vec4 unpackRGBA(uint c)\n"
        "{\n"
        "    return vec4(\n"
        "        float((c >> 0) & 255u) / 255.0,\n"
        "        float((c >> 8) & 255u) / 255.0,\n"
        "        float((c >> 16) & 255u) / 255.0,\n"
        "        float((c >> 24) & 255u) / 255.0\n"
        "    );\n"
        "}\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec2 worldPos = (iPos.xy + vec2(uOffsetX, uOffsetY)) + aQuad * iSize;\n"
        "    float ndc_x = (worldPos.x / uScreenWidth) * 2.0 - 1.0;\n"
        "    float ndc_y = 1.0 - (worldPos.y / uScreenHeight) * 2.0;\n"
        "    gl_Position = vec4(ndc_x, ndc_y, iPos.z * 0.015625, 1.0);\n"
        "    vScreenPos = worldPos;\n"
        "    vUV = vec2(\n"
        "        mix(iUV.x, iUV.z, aQuad.x),\n"
        "        mix(iUV.y, iUV.w, aQuad.y)\n"
        "    );\n"
        "    vColor = unpackRGBA(iColor);\n"
        "}\n";

    const char* textMonoShaderFragSrc =
        "#version 330 core\n"
        "in vec2 vUV;\n"
        "in vec2 vScreenPos;\n"
        "in vec4 vColor;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D uFontTexture;\n"
        "void main()\n"
        "{\n"
        "    float alpha = texture(uFontTexture, vUV).r;\n"
        "    FragColor = vec4(vColor.rgb, vColor.a * alpha);\n"
        "}\n";

    const char* clippedTextMonoShaderFragSrc =
        "#version 330 core\n"
        "in vec2 vUV;\n"
        "in vec2 vScreenPos;\n"
        "in vec4 vColor;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D uFontTexture;\n"
        "uniform float uClipTop;\n"
        "uniform float uClipBottom;\n"
        "uniform float uClipLeft;\n"
        "uniform float uClipRight;\n"
        "void main()\n"
        "{\n"
        "    if (vScreenPos.x < uClipLeft || vScreenPos.x > uClipRight ||\n"
        "        vScreenPos.y < uClipTop  || vScreenPos.y > uClipBottom)\n"
        "    {\n"
        "        discard;\n"
        "    }\n"
        "    float alpha = texture(uFontTexture, vUV).r;\n"
        "    FragColor = vec4(vColor.rgb, vColor.a * alpha);\n"
        "}\n";

    const char* textSubpixelShaderFragSrc =
        "#version 330 core\n"
        "in vec2 vUV;\n"
        "in vec2 vScreenPos;\n"
        "in vec4 vColor;\n"
        "layout(location = 0) out vec4 FragColor;\n"
        "layout(location = 1) out vec4 FragColor1;\n"
        "uniform sampler2D uFontTexture;\n"
        "void main()\n"
        "{\n"
        "    vec3 lcd = texture(uFontTexture, vUV).rgb;\n"
        "    FragColor  = vec4(vColor.rgb, 1.0);\n"
        "    FragColor1 = vec4(lcd, 1.0);\n"
        "}\n";

    const char* clippedTextSubpixelShaderFragSrc =
        "#version 330 core\n"
        "in vec2 vUV;\n"
        "in vec2 vScreenPos;\n"
        "in vec4 vColor;\n"
        "layout(location = 0) out vec4 FragColor;\n"
        "layout(location = 1) out vec4 FragColor1;\n"
        "uniform sampler2D uFontTexture;\n"
        "uniform float uClipTop;\n"
        "uniform float uClipBottom;\n"
        "uniform float uClipLeft;\n"
        "uniform float uClipRight;\n"
        "void main()\n"
        "{\n"
        "    if (vScreenPos.x < uClipLeft || vScreenPos.x > uClipRight ||\n"
        "        vScreenPos.y < uClipTop  || vScreenPos.y > uClipBottom)\n"
        "    {\n"
        "        discard;\n"
        "    }\n"
        "    vec3 lcd = texture(uFontTexture, vUV).rgb;\n"
        "    FragColor  = vec4(vColor.rgb, 1.0);\n"
        "    FragColor1 = vec4(lcd, 1.0);\n"
        "}\n";

    // Create text shader programs
    TextMonoShader            = Create_Shader_Program(textShaderVertSrc, textMonoShaderFragSrc);
    TextSubpixelShader        = Create_Shader_Program(textShaderVertSrc, textSubpixelShaderFragSrc);
    ClippedTextMonoShader     = Create_Shader_Program(textShaderVertSrc, clippedTextMonoShaderFragSrc);
    ClippedTextSubpixelShader = Create_Shader_Program(textShaderVertSrc, clippedTextSubpixelShaderFragSrc);

    // Uniform locations: mono
    uTextMonoScreenWidthLoc  = glGetUniformLocation(TextMonoShader, "uScreenWidth");
    uTextMonoScreenHeightLoc = glGetUniformLocation(TextMonoShader, "uScreenHeight");
    uTextMonoOffsetXLoc      = glGetUniformLocation(TextMonoShader, "uOffsetX");
    uTextMonoOffsetYLoc      = glGetUniformLocation(TextMonoShader, "uOffsetY");
    uTextMonoFontTextureLoc  = glGetUniformLocation(TextMonoShader, "uFontTexture");

    // Uniform locations: subpixel
    uTextSubpixelScreenWidthLoc  = glGetUniformLocation(TextSubpixelShader, "uScreenWidth");
    uTextSubpixelScreenHeightLoc = glGetUniformLocation(TextSubpixelShader, "uScreenHeight");
    uTextSubpixelOffsetXLoc      = glGetUniformLocation(TextSubpixelShader, "uOffsetX");
    uTextSubpixelOffsetYLoc      = glGetUniformLocation(TextSubpixelShader, "uOffsetY");
    uTextSubpixelFontTextureLoc  = glGetUniformLocation(TextSubpixelShader, "uFontTexture");

    // Uniform locations: clipped mono
    uClippedTextMonoScreenWidthLoc   = glGetUniformLocation(ClippedTextMonoShader, "uScreenWidth");
    uClippedTextMonoScreenHeightLoc  = glGetUniformLocation(ClippedTextMonoShader, "uScreenHeight");
    uClippedTextMonoOffsetXLoc       = glGetUniformLocation(ClippedTextMonoShader, "uOffsetX");
    uClippedTextMonoOffsetYLoc       = glGetUniformLocation(ClippedTextMonoShader, "uOffsetY");
    uClippedTextMonoScissorTopLoc    = glGetUniformLocation(ClippedTextMonoShader, "uClipTop");
    uClippedTextMonoScissorBottomLoc = glGetUniformLocation(ClippedTextMonoShader, "uClipBottom");
    uClippedTextMonoScissorLeftLoc   = glGetUniformLocation(ClippedTextMonoShader, "uClipLeft");
    uClippedTextMonoScissorRightLoc  = glGetUniformLocation(ClippedTextMonoShader, "uClipRight");
    uClippedTextMonoFontTextureLoc   = glGetUniformLocation(ClippedTextMonoShader, "uFontTexture");

    // Uniform locations: clipped subpixel
    uClippedTextSubpixelScreenWidthLoc   = glGetUniformLocation(ClippedTextSubpixelShader, "uScreenWidth");
    uClippedTextSubpixelScreenHeightLoc  = glGetUniformLocation(ClippedTextSubpixelShader, "uScreenHeight");
    uClippedTextSubpixelOffsetXLoc       = glGetUniformLocation(ClippedTextSubpixelShader, "uOffsetX");
    uClippedTextSubpixelOffsetYLoc       = glGetUniformLocation(ClippedTextSubpixelShader, "uOffsetY");
    uClippedTextSubpixelScissorTopLoc    = glGetUniformLocation(ClippedTextSubpixelShader, "uClipTop");
    uClippedTextSubpixelScissorBottomLoc = glGetUniformLocation(ClippedTextSubpixelShader, "uClipBottom");
    uClippedTextSubpixelScissorLeftLoc   = glGetUniformLocation(ClippedTextSubpixelShader, "uClipLeft");
    uClippedTextSubpixelScissorRightLoc  = glGetUniformLocation(ClippedTextSubpixelShader, "uClipRight");
    uClippedTextSubpixelFontTextureLoc   = glGetUniformLocation(ClippedTextSubpixelShader, "uFontTexture");

    // Text Vao
    glGenVertexArrays(1, &TextVao);
    glBindVertexArray(TextVao);

    // Bind quad mesh
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 2,
        GL_FLOAT, GL_FALSE,
        2 * sizeof(float),
        (void*)0
    );

    // Text instance data
    glGenBuffers(1, &TextVbo);
    glBindBuffer(GL_ARRAY_BUFFER, TextVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GlyphRenderData) * TEXT_RENDER_DATA_GPU_RESERVE_COUNT, NULL, GL_STREAM_DRAW);

    // Position (x, y, z)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GlyphRenderData), (void*)offsetof(GlyphRenderData, x));
    glVertexAttribDivisor(1, 1);

    // Size (w, h)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GlyphRenderData), (void*)offsetof(GlyphRenderData, w));
    glVertexAttribDivisor(2, 1);

    // UVs (u0, v0, u1, v1)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(GlyphRenderData), (void*)offsetof(GlyphRenderData, u0));
    glVertexAttribDivisor(3, 1);

    // Color RGBA
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT, sizeof(GlyphRenderData), (void*)offsetof(GlyphRenderData, colourRGBA));
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);

    // ClippedText Vao
    glGenVertexArrays(1, &uClippedTextVao);
    glBindVertexArray(uClippedTextVao);

    // Static quad Vbo
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    // ClippedText instance data
    glGenBuffers(1, &uClippedTextVbo);
    glBindBuffer(GL_ARRAY_BUFFER, uClippedTextVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GlyphRenderData) * 10000, NULL, GL_STREAM_DRAW);

    // Position (x, y, z)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GlyphRenderData), (void*)offsetof(GlyphRenderData, x));
    glVertexAttribDivisor(1, 1);

    // Size (w, h)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GlyphRenderData), (void*)offsetof(GlyphRenderData, w));
    glVertexAttribDivisor(2, 1);

    // UVs (u0, v0, u1, v1)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(GlyphRenderData), (void*)offsetof(GlyphRenderData, u0));
    glVertexAttribDivisor(3, 1);

    // Color RGBA
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT, sizeof(GlyphRenderData), (void*)offsetof(GlyphRenderData, colourRGBA));
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);
}

int NU_Draw_Init()
{
    if (FT_Init_FreeType(&nu_global_freetype)) {
        printf("Could not init FreeType.\n");
        return 0;
    }

    NU_Init_Quad();
    NU_Init_SDF_Border_Rect_Shader();
    NU_Init_Mesh_Border_Rect_Shader();
    NU_Init_Image_Shader();
    NU_Init_Text_Shader();
    return 1;
}





// ----------------------
// --- Draw Functions ---
// ----------------------
void Draw_SDF_Border_Rects(
    Array borderRects,
    float screenW,
    float screenH
)
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(sdfRectShader);
    glUniform1f(uSdfRectScreenWidthLoc, screenW);
    glUniform1f(uSdfRectScreenHeightLoc, screenH);
    glBindVertexArray(sdfRectVao);
    glBindBuffer(GL_ARRAY_BUFFER, sdfRectVbo);

    // Orphan render data
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(BorderRectRenderData) * SDF_RECT_RENDER_DATA_GPU_RESERVE_COUNT,
        NULL,
        GL_STREAM_DRAW
    );

    // Perform multiple passes until all elements are drawn
    size_t offset = 0;
    while (offset < borderRects.size)
    {
        size_t count = borderRects.size - offset;
        if (count > SDF_RECT_RENDER_DATA_GPU_RESERVE_COUNT) count = SDF_RECT_RENDER_DATA_GPU_RESERVE_COUNT;
        glBufferSubData(
            GL_ARRAY_BUFFER, 0,
            count * borderRects.elementSize,
            (char*)borderRects.data + offset * borderRects.elementSize
        );
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, count);
        offset += count;
    }

    glBindVertexArray(0);
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

void NU_Draw_Images(
    Array imageRenderDatas,
    GLuint imageHandle,
    float screenW,
    float screenH
)
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(ImageShader);
    glUniform1f(uImageScreenWidthLoc, screenW);
    glUniform1f(uImageScreenHeightLoc, screenH);
    glBindVertexArray(imageVao);
    glBindBuffer(GL_ARRAY_BUFFER, imageVbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, imageHandle);
    glUniform1i(uImageTextureLoc, 0);

    // Perform multiple passes until all elements are drawn
    size_t offset = 0;
    while (offset < imageRenderDatas.size)
    {
        size_t count = imageRenderDatas.size - offset;
        if (count > IMAGE_RENDER_DATA_GPU_RESERVE_COUNT) count = IMAGE_RENDER_DATA_GPU_RESERVE_COUNT;
        glBufferSubData(
            GL_ARRAY_BUFFER, 0,
            count * imageRenderDatas.elementSize,
            (char*)imageRenderDatas.data + offset * imageRenderDatas.elementSize
        );
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, count);
        offset += count;
    }

    glBindVertexArray(0);
}

void NU_Draw_Image(
    ImageRenderData* renderData,
    float screenW,
    float screenH,
    GLuint imageHandle
)
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(ImageShader);
    glUniform1f(uImageScreenWidthLoc, screenW);
    glUniform1f(uImageScreenHeightLoc, screenH);
    glBindVertexArray(imageVao);
    glBindBuffer(GL_ARRAY_BUFFER, imageVbo);

    // Orphan render data
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(ImageRenderData) * IMAGE_RENDER_DATA_GPU_RESERVE_COUNT,
        NULL,
        GL_STREAM_DRAW
    );

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, imageHandle);
    glUniform1i(uImageTextureLoc, 0);
    glBufferSubData(
        GL_ARRAY_BUFFER, 0,
        (GLsizeiptr)(sizeof(ImageRenderData)),
        renderData
    );
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1);
    glBindVertexArray(0);
}

void NU_Draw_Text(
    Array textRenderData,
    NU_Font* font,
    float screenW,
    float screenH,
    float offsetX,
    float offsetY
)
{
    if (font->subpixelRendering) {
        glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
        glUseProgram(TextSubpixelShader);
        glUniform1f(uTextSubpixelScreenWidthLoc,  screenW);
        glUniform1f(uTextSubpixelScreenHeightLoc, screenH);
        glUniform1f(uTextSubpixelOffsetXLoc, offsetX);
        glUniform1f(uTextSubpixelOffsetYLoc, offsetY);
        glUniform1i(uTextSubpixelFontTextureLoc, 0);
    }
    else {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(TextMonoShader);
        glUniform1f(uTextMonoScreenWidthLoc,  screenW);
        glUniform1f(uTextMonoScreenHeightLoc, screenH);
        glUniform1f(uTextMonoOffsetXLoc, offsetX);
        glUniform1f(uTextMonoOffsetYLoc, offsetY);
        glUniform1i(uTextMonoFontTextureLoc, 0);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font->atlas.handle);
    glBindVertexArray(TextVao);
    glBindBuffer(GL_ARRAY_BUFFER, TextVbo);

    // Orphan render data
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(GlyphRenderData) * TEXT_RENDER_DATA_GPU_RESERVE_COUNT,
        NULL,
        GL_STREAM_DRAW
    );

    // Perform multiple passes until all elements are drawn
    size_t offset = 0;
    while (offset < textRenderData.size)
    {
        size_t count = textRenderData.size - offset;
        if (count > TEXT_RENDER_DATA_GPU_RESERVE_COUNT) count = TEXT_RENDER_DATA_GPU_RESERVE_COUNT;


        glBufferSubData(
            GL_ARRAY_BUFFER, 0,
            count * textRenderData.elementSize,
            (char*)textRenderData.data + offset * textRenderData.elementSize
        );
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, count);
        offset += count;
    }

    glBindVertexArray(0);
}

void NU_Draw_Clipped_Text(
    Array textRenderData,
    NU_Font* font,
    float screenW,
    float screenH,
    float offsetX,
    float offsetY,
    float clipTop,
    float clipBottom,
    float clipLeft,
    float clipRight
)
{
    if (font->subpixelRendering) {
        glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
        glUseProgram(ClippedTextSubpixelShader);
        glUniform1f(uClippedTextSubpixelScreenWidthLoc,  screenW);
        glUniform1f(uClippedTextSubpixelScreenHeightLoc, screenH);
        glUniform1f(uClippedTextSubpixelOffsetXLoc, offsetX);
        glUniform1f(uClippedTextSubpixelOffsetYLoc, offsetY);
        glUniform1f(uClippedTextSubpixelScissorTopLoc, clipTop);
        glUniform1f(uClippedTextSubpixelScissorBottomLoc, clipBottom);
        glUniform1f(uClippedTextSubpixelScissorLeftLoc, clipLeft);
        glUniform1f(uClippedTextSubpixelScissorRightLoc, clipRight);
        glUniform1i(uClippedTextSubpixelFontTextureLoc, 0);
    }
    else {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(ClippedTextMonoShader);
        glUniform1f(uClippedTextMonoScreenWidthLoc,  screenW);
        glUniform1f(uClippedTextMonoScreenHeightLoc, screenH);
        glUniform1f(uClippedTextMonoOffsetXLoc, offsetX);
        glUniform1f(uClippedTextMonoOffsetYLoc, offsetY);
        glUniform1f(uClippedTextMonoScissorTopLoc, clipTop);
        glUniform1f(uClippedTextMonoScissorBottomLoc, clipBottom);
        glUniform1f(uClippedTextMonoScissorLeftLoc, clipLeft);
        glUniform1f(uClippedTextMonoScissorRightLoc, clipRight);
        glUniform1i(uClippedTextMonoFontTextureLoc, 0);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font->atlas.handle);
    glBindVertexArray(TextVao);
    glBindBuffer(GL_ARRAY_BUFFER, TextVbo);

    // Orphan render data
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(GlyphRenderData) * TEXT_RENDER_DATA_GPU_RESERVE_COUNT,
        NULL,
        GL_STREAM_DRAW
    );

    // Perform multiple passes until all elements are drawn
    size_t offset = 0;
    while (offset < textRenderData.size)
    {
        size_t count = textRenderData.size - offset;
        if (count > TEXT_RENDER_DATA_GPU_RESERVE_COUNT) count = TEXT_RENDER_DATA_GPU_RESERVE_COUNT;
        glBufferSubData(
            GL_ARRAY_BUFFER, 0,
            count * textRenderData.elementSize,
            (char*)textRenderData.data + offset * textRenderData.elementSize
        );
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, count);
        offset += count;
    }

    glBindVertexArray(0);
}
