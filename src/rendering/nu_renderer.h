#pragma once

#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <math.h>
#include <rendering/gui/nu_mesh_generation.h>
#include <rendering/nu_renderer_structures.h>
#include <rendering/nu_shader.h>

// Border rect SDF
GLuint sdfRectShader;
GLuint sdfRectVao, sdfRectVbo;
GLint uSdfRectScreenWidthLoc, uSdfRectScreenHeightLoc;

// gui 
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

// text
GLuint Text_Mono_Shader_Program;
GLuint Text_Subpixel_Shader_Program;
GLuint text_vao, text_vbo, text_ebo;
GLint uMonoScreenWidthLoc, uMonoScreenHeightLoc, uMonoFontTextureLoc;
GLint uSubpixelScreenWidthLoc, uSubpixelScreenHeightLoc, uSubpixelFontTextureLoc;
GLint uSubpixelOffsetXLoc, uSubpixelOffsetYLoc;
GLint uMonoOffsetXLoc, uMonoOffsetYLoc;
GLint uMonoClipTopLoc, uMonoClipBottomLoc, uMonoClipLeftLoc, uMonoClipRightLoc;
GLint uSubpixelClipTopLoc, uSubpixelClipBottomLoc, uSubpixelClipLeftLoc, uSubpixelClipRightLoc;
GLint uMonoClipTopLoc, uMonoClipBottomLoc, uMonoClipLeftLoc, uMonoClipRightLoc;
GLint uSubpixelClipTopLoc, uSubpixelClipBottomLoc, uSubpixelClipLeftLoc, uSubpixelClipRightLoc;

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
    "out vec2 vWorldPos;\n"
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
    "    vec2 worldPos = iPos.xy + aQuad * iSize;\n"
    "    float ndc_x = (worldPos.x / uScreenWidth) * 2.0 - 1.0;\n"
    "    float ndc_y = 1.0 - (worldPos.y / uScreenHeight) * 2.0;\n"
    "    gl_Position = vec4(ndc_x, ndc_y, iPos.z * 0.003f, 1.0);\n"
    "    vLocalPos = aQuad * iSize;\n"
    "    vWorldPos = worldPos;\n"
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
    "in vec2 vWorldPos;\n"
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
    "    if (vWorldPos.x < vScissor.x ||\n"
    "        vWorldPos.y < vScissor.y ||\n"
    "        vWorldPos.x > vScissor.x + vScissor.z ||\n"
    "        vWorldPos.y > vScissor.y + vScissor.w)\n"
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
    "    FragColor = vec4(color, alpha);\n"
    "}\n";


    sdfRectShader = Create_Shader_Program(sdfRect_VertSrc, sdfRect_FragSrc);
    uSdfRectScreenWidthLoc  = glGetUniformLocation(sdfRectShader, "uScreenWidth");
    uSdfRectScreenHeightLoc = glGetUniformLocation(sdfRectShader, "uScreenHeight");

    float sdfQuad[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    // SDF Rect Vao
    glGenVertexArrays(1, &sdfRectVao);
    glBindVertexArray(sdfRectVao);

    // Static Quad Vbo
    GLuint quadVbo;
    glGenBuffers(1, &quadVbo);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sdfQuad), sdfQuad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    // SDF Rect instance data
    glGenBuffers(1, &sdfRectVbo);
    glBindBuffer(GL_ARRAY_BUFFER, sdfRectVbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(BorderRectRenderData) * 2000, NULL, GL_STREAM_DRAW);

    // IMPORTANT: keep instance buffer bound for attribute setup
    glBindBuffer(GL_ARRAY_BUFFER, sdfRectVbo);

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
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(BorderRectRenderData), (void*)offsetof(BorderRectRenderData, scissorX));
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
    "    gl_Position = vec4(ndc_x, ndc_y, aPos.z * 0.003f, 1.0);\n"
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
    "layout(location = 0) in vec3 aPos;\n"
    "layout(location = 1) in vec2 aUV;\n"
    "out vec2 vUV;\n"
    "out vec2 vScreenPos;\n"
    "uniform float uScreenWidth;\n"
    "uniform float uScreenHeight;\n"
    "void main() {\n"
    "    // Convert screen position (pixels) to NDC for gl_Position\n"
    "    float ndc_x = (aPos.x / uScreenWidth) * 2.0 - 1.0;\n"
    "    float ndc_y = 1.0 - (aPos.y / uScreenHeight) * 2.0;\n"
    "    gl_Position = vec4(ndc_x, ndc_y, aPos.z * 0.003f, 1.0);\n"
    "    vUV = aUV;\n"
    "    vScreenPos = vec2(aPos.x, aPos.y);\n"
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

    ImageShader = Create_Shader_Program(imageVertSrc, imageFragSrc);

    uImageScreenWidthLoc  = glGetUniformLocation(ImageShader, "uScreenWidth");
    uImageScreenHeightLoc = glGetUniformLocation(ImageShader, "uScreenHeight");
    uImageClipTopLoc      = glGetUniformLocation(ImageShader, "uClipTop");
    uImageClipBottomLoc   = glGetUniformLocation(ImageShader, "uClipBottom");
    uImageClipLeftLoc     = glGetUniformLocation(ImageShader, "uClipLeft");
    uImageClipRightLoc    = glGetUniformLocation(ImageShader, "uClipRight");
    uImageTextureLoc      = glGetUniformLocation(ImageShader, "uTexture");

    glGenVertexArrays(1, &imageVao);
    glBindVertexArray(imageVao);
    glGenBuffers(1, &imageVbo);
    glBindBuffer(GL_ARRAY_BUFFER, imageVbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_uv), (void*)0); // x,y,z
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_uv), (void*)(3 * sizeof(float))); // u,v
    glEnableVertexAttribArray(1);
    glGenBuffers(1, &imageEbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, imageEbo);
    glBindVertexArray(0); 
}

void NU_Init_Text_Shader()
{
    const char* textVertexSrc = 
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "layout(location = 1) in vec3 aColor;\n"
    "layout(location = 2) in vec2 aUV;\n"
    "out vec3 vColor;\n"
    "out vec2 vUV;\n"
    "out vec2 vScreenPos;\n"
    "uniform float uScreenWidth;\n"
    "uniform float uScreenHeight;\n"
    "uniform float uOffsetX;\n"
    "uniform float uOffsetY;\n"
    "void main() {\n"
    "    float ndc_x = ((aPos.x + uOffsetX) / uScreenWidth) * 2.0 - 1.0;\n"
    "    float ndc_y = 1.0 - ((aPos.y + uOffsetY) / uScreenHeight) * 2.0;\n"
    "    gl_Position = vec4(ndc_x, ndc_y, aPos.z * 0.003f, 1.0);\n"
    "    vColor = aColor;\n"
    "    vUV = aUV;\n"
    "    vScreenPos = vec2(aPos.x + uOffsetX, aPos.y + uOffsetY);\n"
    "}\n";

    const char* textMonoFragmentSrc = 
    "#version 330 core\n"
    "in vec3 vColor;\n"
    "in vec2 vUV;\n"
    "in vec2 vScreenPos;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uFontTexture;\n"
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
    "       float alpha = texture(uFontTexture, vUV).r;\n"
    "       FragColor = vec4(vColor, alpha);\n"
    "    }\n"
    "}\n";

    const char* textSubpixelFragmentSrc = 
    "#version 330 core\n"
    "in vec3 vColor;\n"
    "in vec2 vUV;\n"
    "in vec2 vScreenPos;\n"
    "layout(location = 0) out vec4 FragColor;\n"
    "layout(location = 1) out vec4 FragColor1;\n"
    "uniform sampler2D uFontTexture;\n"
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
    "       vec3 lcd = texture(uFontTexture, vUV).rgb;      // subpixel coverage\n"
    "       // Standard dual-source blending for subpixel rendering:\n"
    "       // FragColor contains the text color\n"
    "       // FragColor1 contains the coverage mask\n"
    "       FragColor = vec4(vColor, 1.0);                  // Pure text color\n"
    "       FragColor1 = vec4(lcd, 1.0);                    // Pure coverage mask\n"
    "    }\n"
    "}\n";

    Text_Mono_Shader_Program = Create_Shader_Program(textVertexSrc, textMonoFragmentSrc);
    Text_Subpixel_Shader_Program = Create_Shader_Program(textVertexSrc, textSubpixelFragmentSrc);
     
    uMonoScreenWidthLoc      = glGetUniformLocation(Text_Mono_Shader_Program, "uScreenWidth");
    uMonoScreenHeightLoc     = glGetUniformLocation(Text_Mono_Shader_Program, "uScreenHeight");
    uMonoFontTextureLoc      = glGetUniformLocation(Text_Mono_Shader_Program, "uFontTexture");
    uSubpixelScreenWidthLoc  = glGetUniformLocation(Text_Subpixel_Shader_Program, "uScreenWidth");
    uSubpixelScreenHeightLoc = glGetUniformLocation(Text_Subpixel_Shader_Program, "uScreenHeight");
    uSubpixelOffsetXLoc      = glGetUniformLocation(Text_Subpixel_Shader_Program, "uOffsetX");
    uSubpixelOffsetYLoc      = glGetUniformLocation(Text_Subpixel_Shader_Program, "uOffsetY");
    uMonoOffsetXLoc          = glGetUniformLocation(Text_Mono_Shader_Program, "uOffsetX");
    uMonoOffsetYLoc          = glGetUniformLocation(Text_Mono_Shader_Program, "uOffsetY");
    uSubpixelFontTextureLoc  = glGetUniformLocation(Text_Subpixel_Shader_Program, "uFontTexture");
    uMonoClipTopLoc          = glGetUniformLocation(Text_Mono_Shader_Program, "uClipTop");
    uMonoClipBottomLoc       = glGetUniformLocation(Text_Mono_Shader_Program, "uClipBottom");
    uMonoClipLeftLoc         = glGetUniformLocation(Text_Mono_Shader_Program, "uClipLeft");
    uMonoClipRightLoc        = glGetUniformLocation(Text_Mono_Shader_Program, "uClipRight");
    uSubpixelClipTopLoc      = glGetUniformLocation(Text_Subpixel_Shader_Program, "uClipTop");
    uSubpixelClipBottomLoc   = glGetUniformLocation(Text_Subpixel_Shader_Program, "uClipBottom");
    uSubpixelClipLeftLoc     = glGetUniformLocation(Text_Subpixel_Shader_Program, "uClipLeft");
    uSubpixelClipRightLoc    = glGetUniformLocation(Text_Subpixel_Shader_Program, "uClipRight");
    
    glGenVertexArrays(1, &text_vao);
    glBindVertexArray(text_vao);
    glGenBuffers(1, &text_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_rgb_uv), (void*)0); // x,y,z
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_rgb_uv), (void*)(3 * sizeof(float))); // r,g,b
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_rgb_uv), (void*)(6 * sizeof(float))); // u,v
    glEnableVertexAttribArray(2);
    glGenBuffers(1, &text_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_ebo);
    glBindVertexArray(0); 
}

int NU_Draw_Init()
{   
    if (FT_Init_FreeType(&nu_global_freetype)) {
        printf("Could not init FreeType.\n");
        return 0;
    }

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
    glBufferSubData(
        GL_ARRAY_BUFFER,0,
        borderRects.size * sizeof(BorderRectRenderData),
        borderRects.data
    );
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, borderRects.size);
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

void NU_Draw_Image(
    float x, float y, 
    float w, float h, float z,
    float screen_width, float screen_height,
    float clip_top, float clip_bottom, float clip_left, float clip_right, 
    GLuint image_handle
)
{
    // Mesh 
    vertex_uv vertices[4] = {
        { x,     y,   z, 0.0f, 0.0f }, // top-left
        { x+w,   y,   z, 1.0f, 0.0f }, // top-right
        { x,     y+h, z, 0.0f, 1.0f }, // bottom-left
        { x+w,   y+h, z, 1.0f, 1.0f }, // bottom-right
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

void NU_Render_Text
(
    Vertex_RGB_UV_List* vertices, 
    Index_List* indices, 
    NU_Font* font, 
    float screen_width, 
    float screen_height,
    float offset_x,
    float offset_y,
    float clip_top,
    float clip_bottom,
    float clip_left,
    float clip_right
)
{
    if (font->subpixel_rendering) glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    else glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render
    if (font->subpixel_rendering)
    {
        glUseProgram(Text_Subpixel_Shader_Program);
        glUniform1i(uSubpixelFontTextureLoc, 0);
        glUniform1f(uSubpixelScreenWidthLoc, screen_width);
        glUniform1f(uSubpixelScreenHeightLoc, screen_height);
        glUniform1f(uSubpixelOffsetXLoc, offset_x);
        glUniform1f(uSubpixelOffsetYLoc, offset_y);
        glUniform1f(uSubpixelClipTopLoc, clip_top);
        glUniform1f(uSubpixelClipBottomLoc, clip_bottom);
        glUniform1f(uSubpixelClipLeftLoc, clip_left);
        glUniform1f(uSubpixelClipRightLoc, clip_right);
    }
    else
    {
        glUseProgram(Text_Mono_Shader_Program);
        glUniform1i(uMonoFontTextureLoc, 0);
        glUniform1f(uMonoScreenWidthLoc, screen_width);
        glUniform1f(uMonoScreenHeightLoc, screen_height);
        glUniform1f(uMonoOffsetXLoc, offset_x);
        glUniform1f(uMonoOffsetYLoc, offset_y);
        glUniform1f(uMonoClipTopLoc, clip_top);
        glUniform1f(uMonoClipBottomLoc, clip_bottom);
        glUniform1f(uMonoClipLeftLoc, clip_left);
        glUniform1f(uMonoClipRightLoc, clip_right);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font->atlas.handle);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices->size * sizeof(vertex_rgb_uv), vertices->array, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->size * sizeof(GLuint), indices->array, GL_DYNAMIC_DRAW);
    glBindVertexArray(text_vao);
    glDrawElements(GL_TRIANGLES, indices->size, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}