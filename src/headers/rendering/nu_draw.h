#pragma once

#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <math.h>
#include <rendering/gui/nu_mesh_generation.h>
#include <rendering/nu_draw_structures.h>
#include <rendering/nu_shader.h>

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
GLint uMonoClipTopLoc, uMonoClipBottomLoc, uMonoClipLeftLoc, uMonoClipRightLoc;
GLint uSubpixelClipTopLoc, uSubpixelClipBottomLoc, uSubpixelClipLeftLoc, uSubpixelClipRightLoc;
GLint uMonoClipTopLoc, uMonoClipBottomLoc, uMonoClipLeftLoc, uMonoClipRightLoc;
GLint uSubpixelClipTopLoc, uSubpixelClipBottomLoc, uSubpixelClipLeftLoc, uSubpixelClipRightLoc;



int NU_Draw_Init()
{   
    if (FT_Init_FreeType(&nu_global_freetype)) {
        printf("Could not init FreeType.\n");
        return 0;
    }

    // ---------------
    // --- gui shaders
    // ---------------
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

    // -----------------
    // --- image shaders
    // -----------------
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

    // border rect and image shaders
    BorderRectShader = Create_Shader_Program(borderRectVertSrc, borderRectFragSrc);
    ClippedBorderRectShader = Create_Shader_Program(borderRectVertSrc, clippedBorderRectFragSrc);
    ImageShader = Create_Shader_Program(imageVertSrc, imageFragSrc);

    // border rect and image uniforms
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
 
    // border rect buffer and attrib objects
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

    // image buffer and attrib objects
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





    // ----------------
    // --- text shaders
    // ----------------
    const char* textVertexSrc = 
    "#version 330 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "layout(location = 1) in vec3 aColor;\n"
    "layout(location = 2) in vec2 aUV;\n"
    "out vec3 vColor;\n"
    "out vec2 vUV;\n"
    "out vec2 vScreenPos;\n"
    "uniform float uScreenWidth;\n"
    "uniform float uScreenHeight;\n"
    "void main() {\n"
    "    float ndc_x = (aPos.x / uScreenWidth) * 2.0 - 1.0;\n"
    "    float ndc_y = 1.0 - (aPos.y / uScreenHeight) * 2.0;\n"
    "    gl_Position = vec4(ndc_x, ndc_y, 0.0, 1.0);\n"
    "    vColor = aColor;\n"
    "    vUV = aUV;\n"
    "    vScreenPos = aPos;\n"
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

    // text shader programs
    Text_Mono_Shader_Program = Create_Shader_Program(textVertexSrc, textMonoFragmentSrc);
    Text_Subpixel_Shader_Program = Create_Shader_Program(textVertexSrc, textSubpixelFragmentSrc);
     
    // text uniforms
    uMonoScreenWidthLoc      = glGetUniformLocation(Text_Mono_Shader_Program, "uScreenWidth");
    uMonoScreenHeightLoc     = glGetUniformLocation(Text_Mono_Shader_Program, "uScreenHeight");
    uMonoFontTextureLoc      = glGetUniformLocation(Text_Mono_Shader_Program, "uFontTexture");
    uSubpixelScreenWidthLoc  = glGetUniformLocation(Text_Subpixel_Shader_Program, "uScreenWidth");
    uSubpixelScreenHeightLoc = glGetUniformLocation(Text_Subpixel_Shader_Program, "uScreenHeight");
    uSubpixelFontTextureLoc  = glGetUniformLocation(Text_Subpixel_Shader_Program, "uFontTexture");
    uMonoClipTopLoc          = glGetUniformLocation(Text_Mono_Shader_Program, "uClipTop");
    uMonoClipBottomLoc       = glGetUniformLocation(Text_Mono_Shader_Program, "uClipBottom");
    uMonoClipLeftLoc         = glGetUniformLocation(Text_Mono_Shader_Program, "uClipLeft");
    uMonoClipRightLoc        = glGetUniformLocation(Text_Mono_Shader_Program, "uClipRight");
    uSubpixelClipTopLoc      = glGetUniformLocation(Text_Subpixel_Shader_Program, "uClipTop");
    uSubpixelClipBottomLoc   = glGetUniformLocation(Text_Subpixel_Shader_Program, "uClipBottom");
    uSubpixelClipLeftLoc     = glGetUniformLocation(Text_Subpixel_Shader_Program, "uClipLeft");
    uSubpixelClipRightLoc    = glGetUniformLocation(Text_Subpixel_Shader_Program, "uClipRight");
    
    // text buffer and attrib objects
    glGenVertexArrays(1, &text_vao);
    glBindVertexArray(text_vao);
    glGenBuffers(1, &text_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_rgb_uv), (void*)0); // x,y
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_rgb_uv), (void*)(2 * sizeof(float))); // r,g,b
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_rgb_uv), (void*)(5 * sizeof(float))); // u,v
    glEnableVertexAttribArray(2);
    glGenBuffers(1, &text_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_ebo);
    glBindVertexArray(0); 

    return 1;
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




// -------------------------------
// --- Function for drawing images
// -------------------------------
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




// -----------------------------
// --- Function for drawing text
// -----------------------------
void NU_Render_Text
(
    Vertex_RGB_UV_List* vertices, 
    Index_List* indices, 
    NU_Font* font, 
    float screen_width, 
    float screen_height,
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