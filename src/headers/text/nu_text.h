#pragma once
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#include <stdint.h> 
#include <string.h> 
#include "nu_font.h"
#include "../draw/nu_draw_structures.h"
#include "../draw/nu_shader.h"

GLuint Text_Mono_Shader_Program;
GLuint Text_Subpixel_Shader_Program;
GLuint text_vao, text_vbo, text_ebo;


GLint uMonoScreenWidthLoc, uMonoScreenHeightLoc, uMonoFontTextureLoc;
GLint uSubpixelScreenWidthLoc, uSubpixelScreenHeightLoc, uSubpixelFontTextureLoc;
GLint uMonoClipTopLoc, uMonoClipBottomLoc, uMonoClipLeftLoc, uMonoClipRightLoc;
GLint uSubpixelClipTopLoc, uSubpixelClipBottomLoc, uSubpixelClipLeftLoc, uSubpixelClipRightLoc;
GLint uMonoClipTopLoc, uMonoClipBottomLoc, uMonoClipLeftLoc, uMonoClipRightLoc;
GLint uSubpixelClipTopLoc, uSubpixelClipBottomLoc, uSubpixelClipLeftLoc, uSubpixelClipRightLoc;


int NU_Text_Renderer_Init()
{
    if (FT_Init_FreeType(&nu_global_freetype))
    {
        printf("Could not init FreeType.\n");
        return 0;
    }

    const char* vertex_src = 
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

    const char* mono_fragment_src = 
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

    const char* subpixel_fragment_src = 
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


    // Create shader program
    Text_Mono_Shader_Program = Create_Shader_Program(vertex_src, mono_fragment_src);
    Text_Subpixel_Shader_Program = Create_Shader_Program(vertex_src, subpixel_fragment_src);
     
    // Query uniforms once
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
    
    // VAO + buffers
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

float NU_Calculate_Text_Min_Wrap_Width(NU_Font* font, const char* string)
{
    size_t string_len = strlen(string);
    float result = 0.0f;
    char prev_char = 0;
    float word_width = 0.0f;
    int word_start_i = 0;
    int word_len = 0;
    int k=0;
    while(k < string_len)
    {
        char word_c = string[k];
        if (word_c < 32 || word_c > 126) continue;
        if (word_c == ' ' || k == string_len - 1) // Word completes
        {
            if (k == string_len - 1 && word_c != ' ') word_len += 1;
            if (word_len > 0)
            {
                // Remove tiny extra x space before first glyph and after last glyph in the word
                NU_Glyph* first_glyph_in_word = (NU_Glyph*)Vector_Get(&font->glyphs, string[word_start_i] - 32);
                NU_Glyph* last_glyph_in_word = (NU_Glyph*)Vector_Get(&font->glyphs, string[word_start_i + word_len - 1] - 32);
                word_width -= last_glyph_in_word->advance - last_glyph_in_word->bearingX - last_glyph_in_word->width - first_glyph_in_word->bearingX;
                if (word_width > result) result = word_width;
            }
            word_len = 0;
            word_start_i = k + 1;
            word_width = 0.0f;
        }
        else
        {
            NU_Glyph* glyph = (NU_Glyph*)Vector_Get(&font->glyphs, word_c - 32);
            float kern = 0.0f;
            if (prev_char) kern = font->kerning_table[prev_char - 32][word_c - 32];
            word_width += glyph->advance + kern;
            word_len += 1;
        }
        k += 1;
    }
    return result;
}

float NU_Calculate_FreeText_Height_From_Wrap_Width(NU_Font* font, const char* string, float width)
{
    int string_len = strlen(string);
    if (string_len == 0) return 0.0f;
    NU_Glyph* first_glyph = (NU_Glyph*)Vector_Get(&font->glyphs, string[0] - 32);
    float pen_x = -first_glyph->bearingX;
    int wraps = 0;
    for (int i=0; i<string_len; i++)
    {
        char c = string[i];

        if (c == ' ' && i > 0)
        {
            NU_Glyph* space_glyph = (NU_Glyph*)Vector_Get(&font->glyphs, string[i] - 32);
            float space_advance = space_glyph->advance + font->kerning_table[string[i-1] - 32][string[i] - 32];

            // Calculate width of next word
            float next_word_width = 0.0f;
            int j = i + 1;
            while (j < string_len)
            {
                if (string[j] == ' ') break;
                NU_Glyph* g = (NU_Glyph*)Vector_Get(&font->glyphs, string[j] - 32);
                next_word_width += g->advance;
                if (j > i + 1) next_word_width += font->kerning_table[string[j-1] - 32][string[j] - 32]; // Kerning
                j++;
            }
            if (j - i - 1 > 0)
            {
                NU_Glyph* word_last_g = (NU_Glyph*)Vector_Get(&font->glyphs, string[j-1] - 32);
                next_word_width -= word_last_g->bearingX;
            }

            // If next word overflows width -> wrap onto new line
            if (pen_x + space_advance + next_word_width > width + 1.0f)
            {
                first_glyph = (NU_Glyph*)Vector_Get(&font->glyphs, string[i + 1] - 32);
                pen_x = -first_glyph->bearingX;
                wraps++;
            }
            else
            {
                pen_x += space_advance;
            }
        }
        else
        {
            NU_Glyph* glyph = (NU_Glyph*)Vector_Get(&font->glyphs, string[i] - 32);
            pen_x += glyph->advance;
            if (i != 0) { // Kerning
                pen_x += font->kerning_table[string[i-1] - 32][string[i] - 32];
            }
        }
    }

    return wraps * font->line_height + font->ascent - font->descent;
}

float NU_Calculate_Text_Unwrapped_Width(NU_Font* font, const char* string)
{
    int string_len = strlen(string);
    if (string_len == 0) return 0.0f;
    float width = 0.0f;
    for (int i=0; i<string_len; i++)
    {
        char c = string[i];
        NU_Glyph* g = (NU_Glyph*)Vector_Get(&font->glyphs, c - 32);
        width += g->advance;
        if (i > 0) width += font->kerning_table[string[i-1] - 32][string[i] - 32]; // Kerning
    }
    NU_Glyph* first_glyph = (NU_Glyph*)Vector_Get(&font->glyphs, string[0] - 32);
    NU_Glyph* last_glyph = (NU_Glyph*)Vector_Get(&font->glyphs, string[string_len - 1] - 32);
    return width - first_glyph->bearingX - last_glyph->bearingX;
}

void NU_Add_Glyph_Mesh(Vertex_RGB_UV_List* vertices, Index_List* indices, NU_Glyph* glyph, float pen_x, float pen_y, float r, float g, float b)
{
    float left = pen_x + glyph->bearingX;
    float top = pen_y - glyph->bearingY;
    float bottom = top + glyph->height;
    int vertex_offset = vertices->size;
    vertices->array[vertex_offset].x = left;
    vertices->array[vertex_offset].y = top;
    vertices->array[vertex_offset].r = r;
    vertices->array[vertex_offset].g = g;
    vertices->array[vertex_offset].b = b;
    vertices->array[vertex_offset].u = glyph->uv_x0;
    vertices->array[vertex_offset].v = glyph->uv_y0;
    vertices->array[vertex_offset + 1].x = left + (float)glyph->width;
    vertices->array[vertex_offset + 1].y = top;
    vertices->array[vertex_offset + 1].r = r;
    vertices->array[vertex_offset + 1].g = g;
    vertices->array[vertex_offset + 1].b = b;
    vertices->array[vertex_offset + 1].u = glyph->uv_x1;
    vertices->array[vertex_offset + 1].v = glyph->uv_y0;
    vertices->array[vertex_offset + 2].x = left;
    vertices->array[vertex_offset + 2].y = bottom;
    vertices->array[vertex_offset + 2].r = r;
    vertices->array[vertex_offset + 2].g = g;
    vertices->array[vertex_offset + 2].b = b;
    vertices->array[vertex_offset + 2].u = glyph->uv_x0;
    vertices->array[vertex_offset + 2].v = glyph->uv_y1;
    vertices->array[vertex_offset + 3].x = left + (float)glyph->width;
    vertices->array[vertex_offset + 3].y = bottom;
    vertices->array[vertex_offset + 3].r = r;
    vertices->array[vertex_offset + 3].g = g;
    vertices->array[vertex_offset + 3].b = b;
    vertices->array[vertex_offset + 3].u = glyph->uv_x1;
    vertices->array[vertex_offset + 3].v = glyph->uv_y1;
    vertices->size += 4;
    uint32_t* indices_write = indices->array + indices->size;
    *indices_write++ = vertex_offset;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 3;
    indices->size += 6;
}

void NU_Generate_Text_Mesh(Vertex_RGB_UV_List* vertices, Index_List* indices, NU_Font* font, const char* string, float x, float y, float r, float g, float b, float maxWidth)
{
    int string_len = strlen(string);
    if (string_len == 0) return;

    // --- Allocate extra space in vertex and index lists ---
    uint32_t additional_vertices = 4 * string_len;   
    uint32_t additional_indices = 6 * string_len;
    if (vertices->size + additional_vertices > vertices->capacity) Vertex_RGB_UV_List_Grow(vertices, additional_vertices);
    if (indices->size + additional_indices > indices->capacity) Index_List_Grow(indices, additional_indices);

    NU_Glyph* first_glyph = (NU_Glyph*)Vector_Get(&font->glyphs, string[0] - 32);

    float pen_x = x - first_glyph->bearingX;
    float pen_y = y + font->ascent;
    for (int i=0; i<string_len; i++)
    {
        char c = string[i];

        if (c == ' ' && i > 0)
        {
            NU_Glyph* space_glyph = (NU_Glyph*)Vector_Get(&font->glyphs, string[i] - 32);
            float space_advance = space_glyph->advance + font->kerning_table[string[i-1] - 32][string[i] - 32];

            // Calculate width of next word
            float next_word_width = 0.0f;
            int j = i + 1;
            while (j < string_len)
            {
                if (string[j] == ' ') break;
                NU_Glyph* g = (NU_Glyph*)Vector_Get(&font->glyphs, string[j] - 32);
                next_word_width += g->advance;
                if (j > i + 1) next_word_width += font->kerning_table[string[j-1] - 32][string[j] - 32]; // Kerning
                j++;
            }
            if (j - i - 1 > 0)
            {
                NU_Glyph* word_last_g = (NU_Glyph*)Vector_Get(&font->glyphs, string[j - 1] - 32);
                next_word_width -= word_last_g->bearingX;
            }

            // If next word overflows width -> wrap onto new line
            if (pen_x - x + space_advance + next_word_width > maxWidth + 1.0f)
            {
                first_glyph = (NU_Glyph*)Vector_Get(&font->glyphs, string[i + 1] - 32);
                pen_x = x - first_glyph->bearingX;
                pen_y += font->line_height;
            }
            else
            {
                NU_Glyph* glyph = (NU_Glyph*)Vector_Get(&font->glyphs, c - 32);
                pen_x += font->kerning_table[string[i-1] - 32][string[i] - 32]; // Kerning
                NU_Add_Glyph_Mesh(vertices, indices, glyph, pen_x, pen_y, r, g, b);
                pen_x += space_advance;
            }
        }
        else
        {
            NU_Glyph* glyph = (NU_Glyph*)Vector_Get(&font->glyphs, string[i] - 32);
            if (i > 0) pen_x += font->kerning_table[string[i-1] - 32][string[i] - 32]; // Kerning
            NU_Add_Glyph_Mesh(vertices, indices, glyph, pen_x, pen_y, r, g, b);
            pen_x += glyph->advance;
        }
    }
}

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

void NU_Render_Font_Atlas(NU_Font_Atlas* atlas, float screen_width, float screen_height, bool subpixel_rendering) 
{
    Vertex_RGB_UV_List vertices;
    Index_List indices;
    Vertex_RGB_UV_List_Init(&vertices, 4);
    Index_List_Init(&indices, 6);

    vertices.array[0].x = 0.0f;                // TL
    vertices.array[0].y = 0.0f;
    vertices.array[0].r = 1.0f;
    vertices.array[0].g = 1.0f;
    vertices.array[0].b = 1.0f;
    vertices.array[0].u = 0.0f;
    vertices.array[0].v = 0.0f;
    vertices.array[1].x = (float)atlas->width; // TR
    vertices.array[1].y = 0.0f;
    vertices.array[1].r = 1.0f;
    vertices.array[1].g = 1.0f;
    vertices.array[1].b = 1.0f;
    vertices.array[1].u = 1.0f;
    vertices.array[1].v = 0.0f;
    vertices.array[2].x = 0.0f;                // BL
    vertices.array[2].y = (float)atlas->height;
    vertices.array[2].r = 1.0f;
    vertices.array[2].g = 1.0f;
    vertices.array[2].b = 1.0f;
    vertices.array[2].u = 0.0f;
    vertices.array[2].v = 1.0f;
    vertices.array[3].x = (float)atlas->width; // BR
    vertices.array[3].y = (float)atlas->height;
    vertices.array[3].r = 1.0f;
    vertices.array[3].g = 1.0f;
    vertices.array[3].b = 1.0f;
    vertices.array[3].u = 1.0f;
    vertices.array[3].v = 1.0f;
    vertices.size += 4;
    uint32_t* indices_write = indices.array + indices.size;
    *indices_write++ = 0;
    *indices_write++ = 1;
    *indices_write++ = 2;
    *indices_write++ = 1;
    *indices_write++ = 2;
    *indices_write++ = 3;
    indices.size += 6;

    // Render
    if (subpixel_rendering)
    {
        glUseProgram(Text_Subpixel_Shader_Program);
        glUniform1i(uSubpixelFontTextureLoc, 0);
        glUniform1f(uSubpixelScreenWidthLoc, screen_width);
        glUniform1f(uSubpixelScreenHeightLoc, screen_height);
    }
    else
    {
        glUseProgram(Text_Mono_Shader_Program);
        glUniform1i(uMonoFontTextureLoc, 0);
        glUniform1f(uMonoScreenWidthLoc, screen_width);
        glUniform1f(uMonoScreenHeightLoc, screen_height);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas->handle);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size * sizeof(vertex_rgb_uv), vertices.array, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size * sizeof(GLuint), indices.array, GL_DYNAMIC_DRAW);
    glBindVertexArray(text_vao);
    glDrawElements(GL_TRIANGLES, indices.size, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Free
    Vertex_RGB_UV_List_Free(&vertices);
    Index_List_Free(&indices);
}