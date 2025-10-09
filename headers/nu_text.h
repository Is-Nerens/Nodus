#pragma once
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#include <stdint.h> 
#include <string.h> 
#include <GL/glew.h>
#include <freetype/freetype.h>
#include <datastructures/vector.h>
#include <nu_font.h>
#include <nu_draw_structures.h>
#include <nu_shader.h>
#include <SDL3/SDL.h>

GLuint Text_Mono_Shader_Program;
GLuint Text_Subpixel_Shader_Program;
GLuint text_vao, text_vbo, text_ebo;
GLint uMonoScreenWidthLoc, uMonoScreenHeightLoc, uMonoTextColourLoc, uMonoFontTextureLoc;
GLint uSubpixelScreenWidthLoc, uSubpixelScreenHeightLoc, uSubpixelTextColourLoc, uSubpixelFontTextureLoc;


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

    const char* mono_fragment_src = 
    "#version 330 core\n"
    "in vec2 vUV;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uFontTexture;\n"
    "uniform vec3 uTextColour;\n"
    "void main() {\n"
    "    float alpha = texture(uFontTexture, vUV).r;\n"
    "    FragColor = vec4(uTextColour, alpha);\n"
    "}\n";

    const char* subpixel_fragment_src = 
    "#version 330 core\n"
    "in vec2 vUV;\n"
    "layout(location = 0) out vec4 FragColor;\n"
    "layout(location = 1) out vec4 FragColor1;\n"
    "uniform sampler2D uFontTexture;\n"
    "uniform vec3 uTextColour;\n"
    "void main() {\n"
    "    // Sample 3-channel LCD glyph\n"
    "    vec3 lcd = texture(uFontTexture, vUV).rgb;\n"
    "    \n"
    "    // Multiply by text color\n"
    "    vec3 color = lcd * uTextColour;\n"
    "    \n"
    "    FragColor  = vec4(1.0); // primary output ignored\n"
    "    FragColor1 = vec4(color, 1.0);         // dual-source output for blending\n"
    "}\n";


    // Create shader program
    Text_Mono_Shader_Program = Create_Shader_Program(vertex_src, mono_fragment_src);
    Text_Subpixel_Shader_Program = Create_Shader_Program(vertex_src, subpixel_fragment_src);
     
    // Query uniforms once
    uMonoScreenWidthLoc  = glGetUniformLocation(Text_Mono_Shader_Program, "uScreenWidth");
    uMonoScreenHeightLoc = glGetUniformLocation(Text_Mono_Shader_Program, "uScreenHeight");
    uMonoTextColourLoc = glGetUniformLocation(Text_Mono_Shader_Program, "uTextColour");
    uMonoFontTextureLoc = glGetUniformLocation(Text_Mono_Shader_Program, "uFontTexture");
    uSubpixelScreenWidthLoc = glGetUniformLocation(Text_Subpixel_Shader_Program, "uScreenWidth");
    uSubpixelScreenHeightLoc = glGetUniformLocation(Text_Subpixel_Shader_Program, "uScreenHeight");
    uSubpixelTextColourLoc = glGetUniformLocation(Text_Subpixel_Shader_Program, "uTextColour");
    uSubpixelFontTextureLoc = glGetUniformLocation(Text_Subpixel_Shader_Program, "uFontTexture");
    
    // VAO + buffers
    glGenVertexArrays(1, &text_vao);
    glBindVertexArray(text_vao);
    glGenBuffers(1, &text_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_uv), (void*)0); // x,y
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_uv), (void*)(2 * sizeof(float))); // u,v
    glEnableVertexAttribArray(1);
    glGenBuffers(1, &text_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_ebo);
    glBindVertexArray(0); // done
    return 1;
}

float NU_Calculate_FreeText_Min_Wrap_Width(NU_Font* font, const char* string)
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
            if (prev_char) kern = (float)font->kerning_table[prev_char - 32][word_c - 32];
            word_width += glyph->advance + kern;
            word_len += 1;
        }
        k += 1;
    }
    return result;
}

float NU_Calculate_FreeText_Height_From_Wrap_Width(NU_Font* font, const char* string, float width)
{
    size_t string_len = strlen(string);
    float result = 0.0f;
    char prev_char = 0;
    float pen_x = 0.0f;
    float pen_y = 0.0f + font->y_max;
    float word_width = 0.0f;
    NU_Glyph* space_glyph = (NU_Glyph*)Vector_Get(&font->glyphs, 0);
    float space_advance = space_glyph->advance;
    int word_start_i = 0;
    int word_len = 0;
    int k=0;
    while (k < string_len)
    {
        char word_c = string[k];
        if (word_c < 32 || word_c > 126) continue;
        if (word_c == ' ' || k == string_len - 1) { // Word completes
            if (k == string_len - 1 && word_c != ' ') word_len += 1;

            // Loop over all the letters in the word
            if (word_len > 0)
            {
                // Remove tiny extra x space before first glyph and after last glyph in the word
                NU_Glyph* first_glyph_in_word = (NU_Glyph*)Vector_Get(&font->glyphs, string[word_start_i] - 32);
                NU_Glyph* last_glyph_in_word = (NU_Glyph*)Vector_Get(&font->glyphs, string[word_start_i + word_len - 1] - 32);
                word_width -= (last_glyph_in_word->advance - last_glyph_in_word->bearingX - last_glyph_in_word->width) - first_glyph_in_word->bearingX;

                // Wrap onto new line
                if (pen_x + word_width > width) {
                    pen_x = 0.0f - first_glyph_in_word->bearingX;
                    pen_y += font->line_height;
                    result = pen_y;
                }

                int i=word_start_i;
                while (i < word_start_i + word_len) {
                    char c = string[i];
                    if (c < 32 || c > 126) continue;
                    NU_Glyph* glyph = (NU_Glyph*)Vector_Get(&font->glyphs, c - 32);

                    float kern = 0.0f;
                    if (prev_char) kern = (float)font->kerning_table[prev_char - 32][c - 32];
                    float glyph_advance = glyph->advance + kern;

                    // Update result
                    result = pen_y + font->y_min;

                    // Move ahead
                    word_width += glyph_advance;
                    pen_x += glyph_advance;
                    prev_char = c;
                    i += 1;
                }
            }

            if (word_c == ' ')
            {
                float kern = (float)font->kerning_table[prev_char - 32][word_c - 32];
                pen_x += space_advance;
                prev_char = word_c;
            }

            word_len = 0;
            word_start_i = k + 1;
            word_width = 0.0f;
        }
        else
        {
            NU_Glyph* glyph = (NU_Glyph*)Vector_Get(&font->glyphs, word_c - 32);
            float kern = 0.0f;
            if (prev_char) kern = (float)font->kerning_table[prev_char - 32][word_c - 32];
            word_width += glyph->advance + kern;
            word_len += 1;
        }
        k += 1;
    }
    return result;
}

float NU_Calculate_Text_Unwrapped_Width(NU_Font* font, const char* string)
{
    size_t string_len = strlen(string);
    NU_Glyph* first_glyph = (NU_Glyph*)Vector_Get(&font->glyphs, string[0] - 32);
    NU_Glyph* last_glyph = (NU_Glyph*)Vector_Get(&font->glyphs, string[string_len - 1] - 32);
    float result = 0.0f;
    char prev_char = 0;
    float pen_x = 0.0f;
    int i = 0;
    while (i < string_len)
    {
        char word_c = string[i];
        if (word_c < 32 || word_c > 126) continue;
        NU_Glyph* glyph = (NU_Glyph*)Vector_Get(&font->glyphs, word_c - 32);
        float kern = 0.0f;
        if (prev_char) kern = (float)font->kerning_table[prev_char - 32][word_c - 32];
        pen_x += glyph->advance + kern;
        i += 1;
    }
    return pen_x - first_glyph->bearingX - (last_glyph->advance - last_glyph->bearingX - last_glyph->width);
}

void NU_Add_Glyph_Mesh(Vertex_UV_List* vertices, Index_List* indices, NU_Glyph* glyph, float pen_x, float pen_y)
{
    float left = roundf(pen_x + glyph->bearingX);
    float top = roundf(pen_y - glyph->bearingY);
    float bottom = top + glyph->height;
    int vertex_offset = vertices->size;
    vertices->array[vertex_offset].x = left;
    vertices->array[vertex_offset].y = top;
    vertices->array[vertex_offset].u = glyph->uv_x0;
    vertices->array[vertex_offset].v = glyph->uv_y0;
    vertices->array[vertex_offset + 1].x = left + (float)glyph->width;
    vertices->array[vertex_offset + 1].y = top;
    vertices->array[vertex_offset + 1].u = glyph->uv_x1;
    vertices->array[vertex_offset + 1].v = glyph->uv_y0;
    vertices->array[vertex_offset + 2].x = left;
    vertices->array[vertex_offset + 2].y = bottom;
    vertices->array[vertex_offset + 2].u = glyph->uv_x0;
    vertices->array[vertex_offset + 2].v = glyph->uv_y1;
    vertices->array[vertex_offset + 3].x = left + (float)glyph->width;
    vertices->array[vertex_offset + 3].y = bottom;
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

void NU_Render_Text(NU_Font* font, const char* string, float x, float y, float max_width, float screen_width, float screen_height)
{
    if (font->subpixel_rendering) glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    else glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    size_t string_len = strlen(string);
    Vertex_UV_List vertices;
    Index_List indices;
    Vertex_UV_List_Init(&vertices, 4 * string_len);
    Index_List_Init(&indices, 6 * string_len);
    char prev_char = 0;
    float pen_x = x;
    float pen_y = y + font->y_max;
    float word_width = 0.0f;
    NU_Glyph* space_glyph = (NU_Glyph*)Vector_Get(&font->glyphs, 0);
    float space_advance = space_glyph->advance;
    int word_start_i = 0;
    int word_len = 0;
    int k=0;
    while(k < string_len)
    {
        char word_c = string[k];
        if (word_c < 32 || word_c > 126) continue;
        if (word_c == ' ' || k == string_len - 1) { // Word completes
            if (k == string_len - 1 && word_c != ' ') word_len += 1;

            // Loop over all the letters in the word
            if (word_len > 0)
            {
                // Remove tiny extra x space before first glyph and after last glyph in the word
                NU_Glyph* first_glyph_in_word = (NU_Glyph*)Vector_Get(&font->glyphs, string[word_start_i] - 32);
                NU_Glyph* last_glyph_in_word = (NU_Glyph*)Vector_Get(&font->glyphs, string[word_start_i + word_len - 1] - 32);
                word_width -= last_glyph_in_word->advance - last_glyph_in_word->bearingX - last_glyph_in_word->width - first_glyph_in_word->bearingX;

                // Wrap onto new line
                if (pen_x + word_width > max_width) {
                    pen_x = x - first_glyph_in_word->bearingX;
                    pen_y += font->line_height;
                }

                int i=word_start_i;
                while (i < word_start_i + word_len) {
                    char c = string[i];
                    if (c < 32 || c > 126) continue;
                    NU_Glyph* glyph = (NU_Glyph*)Vector_Get(&font->glyphs, c - 32);

                    float kern = 0.0f;
                    if (prev_char) kern = (float)font->kerning_table[prev_char - 32][c - 32];
                    float glyph_advance = glyph->advance + kern;

                    // Add quad mesh 
                    NU_Add_Glyph_Mesh(&vertices, &indices, glyph, pen_x, pen_y);

                    // Move ahead
                    word_width += glyph_advance;
                    pen_x += glyph_advance;
                    prev_char = c;
                    i += 1;
                }
            }

            if (word_c == ' ')
            {
                float kern = (float)font->kerning_table[prev_char - 32][word_c - 32];
                pen_x += space_advance;
                prev_char = word_c;
            }

            word_len = 0;
            word_start_i = k + 1;
            word_width = 0.0f;
        }
        else
        {
            NU_Glyph* glyph = (NU_Glyph*)Vector_Get(&font->glyphs, word_c - 32);
            float kern = 0.0f;
            if (prev_char) kern = (float)font->kerning_table[prev_char - 32][word_c - 32];
            word_width += glyph->advance + kern;
            word_len += 1;
        }
        k += 1;
    }


    // Render
    if (font->subpixel_rendering)
    {
        glUseProgram(Text_Subpixel_Shader_Program);
        glUniform1i(uSubpixelFontTextureLoc, 0);
        glUniform1f(uSubpixelScreenWidthLoc, screen_width);
        glUniform1f(uSubpixelScreenHeightLoc, screen_height);
        glUniform3f(uSubpixelTextColourLoc, 1.0f, 1.0f, 1.0f);
    }
    else
    {
        glUseProgram(Text_Mono_Shader_Program);
        glUniform1i(uMonoFontTextureLoc, 0);
        glUniform1f(uMonoScreenWidthLoc, screen_width);
        glUniform1f(uMonoScreenHeightLoc, screen_height);
        glUniform3f(uMonoTextColourLoc, 1.0f, 1.0f, 1.0f);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font->atlas.handle);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size * sizeof(vertex_uv), vertices.array, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size * sizeof(GLuint), indices.array, GL_DYNAMIC_DRAW);
    glBindVertexArray(text_vao);
    glDrawElements(GL_TRIANGLES, indices.size, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Free
    Vertex_UV_List_Free(&vertices);
    Index_List_Free(&indices);
}

void NU_Render_Font_Atlas(NU_Font_Atlas* atlas, float screen_width, float screen_height, bool subpixel_rendering) 
{
    Vertex_UV_List vertices;
    Index_List indices;
    Vertex_UV_List_Init(&vertices, 4);
    Index_List_Init(&indices, 6);

    vertices.array[0].x = 0.0f;                // TL
    vertices.array[0].y = 0.0f;
    vertices.array[0].u = 0.0f;
    vertices.array[0].v = 0.0f;
    vertices.array[1].x = (float)atlas->width; // TR
    vertices.array[1].y = 0.0f;
    vertices.array[1].u = 1.0f;
    vertices.array[1].v = 0.0f;
    vertices.array[2].x = 0.0f;                // BL
    vertices.array[2].y = (float)atlas->height;
    vertices.array[2].u = 0.0f;
    vertices.array[2].v = 1.0f;
    vertices.array[3].x = (float)atlas->width; // BR
    vertices.array[3].y = (float)atlas->height;
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
        glUniform3f(uSubpixelTextColourLoc, 1.0f, 1.0f, 1.0f);
    }
    else
    {
        glUseProgram(Text_Mono_Shader_Program);
        glUniform1i(uMonoFontTextureLoc, 0);
        glUniform1f(uMonoScreenWidthLoc, screen_width);
        glUniform1f(uMonoScreenHeightLoc, screen_height);
        glUniform3f(uMonoTextColourLoc, 1.0f, 1.0f, 1.0f);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas->handle);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size * sizeof(vertex_uv), vertices.array, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size * sizeof(GLuint), indices.array, GL_DYNAMIC_DRAW);
    glBindVertexArray(text_vao);
    glDrawElements(GL_TRIANGLES, indices.size, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Free
    Vertex_UV_List_Free(&vertices);
    Index_List_Free(&indices);
}