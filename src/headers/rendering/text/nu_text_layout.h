#pragma once
#include <stdint.h> 
#include <string.h> 
#include "nu_font.h"
#include <rendering/nu_draw_structures.h>

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