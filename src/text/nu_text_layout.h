#pragma once
#include <stdint.h> 
#include <string.h> 
#include "nu_font.h"
#include <rendering/nu_renderer_structures.h>

u32 NU_GetNextCodepoint(const char* string, int* byteIndex)
{
    unsigned char* p = (unsigned char*)string + *byteIndex;
    u32 cp;

    if (*p == 0) return 0; // end of string

    if ((*p & 0x80) == 0) {
        cp = *p++;
    }
    else if ((*p & 0xE0) == 0xC0 &&
            (p[1] & 0xC0) == 0x80) {
        cp = (*p & 0x1F) << 6 |
            (p[1] & 0x3F);
        p += 2;
    }
    else if ((*p & 0xF0) == 0xE0 &&
            (p[1] & 0xC0) == 0x80 &&
            (p[2] & 0xC0) == 0x80) {
        cp = (*p & 0x0F) << 12 |
            (p[1] & 0x3F) << 6 |
            (p[2] & 0x3F);
        p += 3;
    }
    else if ((*p & 0xF8) == 0xF0 &&
            (p[1] & 0xC0) == 0x80 &&
            (p[2] & 0xC0) == 0x80 &&
            (p[3] & 0xC0) == 0x80) {
        cp = (*p & 0x07) << 18  |
            (p[1] & 0x3F) << 12 |
            (p[2] & 0x3F) << 6  |
            (p[3] & 0x3F);
        p += 4;
    }
    else { // invalid byte
        cp = 0xFFFD;
        p += 1;
    }
    *byteIndex = (int)(p - (unsigned char*)string);
    return cp;
}

float NU_Calculate_Text_Min_Wrap_Width(NU_Font* font, const char* string)
{
    int byteIndex = 0;
    u32 codepoint;
    u32 lastCodepoint = 0;
    u32 wordFirstCodepoint;
    u32 wordLastCodepoint;

    int wordLen = 0;
    float result = 0.0f;
    float wordWidth = 0.0f;

    while ((codepoint = NU_GetNextCodepoint(string, &byteIndex)) != 0)
    {
        int is_space = (codepoint == ' ' || codepoint == '\t');
        if (is_space)
        {
            if (wordLen > 0)
            {
                NU_Glyph* first = NU_Get_Glyph(font, wordFirstCodepoint);
                NU_Glyph* last  = NU_Get_Glyph(font, wordLastCodepoint);

                // Remove space before the first glyph and after the last glyph in the word
                wordWidth -= first->bearingX - last->advance - (last->bearingX + last->width);
                
                // Check if the current word is the longest so far (branchless)
                result += (wordWidth > result) * (wordWidth - result);

                // Reset
                wordWidth = 0.0f;
                wordLen = 0;
            }

            continue;
        }

        NU_Glyph* glyph = NU_Get_Glyph(font, codepoint);

        if (wordLen == 0) wordFirstCodepoint = codepoint;

        // Add kerning
        else {
            NU_Glyph* prev = NU_Get_Glyph(font, lastCodepoint);
            wordWidth += NU_Get_Kerning(font, glyph->index, prev->index);
        }

        wordWidth += glyph->advance;
        wordLastCodepoint = codepoint;
        lastCodepoint = codepoint;
        wordLen++;
    }

    // Measure length of final word
    if (wordLen > 0)
    {
        NU_Glyph* first = NU_Get_Glyph(font, wordFirstCodepoint);
        NU_Glyph* last  = NU_Get_Glyph(font, wordLastCodepoint);

        // Remove space before the first glyph and after the last glyph in the word
        wordWidth -= first->bearingX - last->advance - (last->bearingX + last->width);

        // Check if the current word is the longest so far (branchless)
        result += (wordWidth > result) * (wordWidth - result);
    }

    return result;
}

float NU_Calculate_FreeText_Height_From_Wrap_Width(NU_Font* font, const char* string, float width)
{
    if (string[0] == '\0') return 0.0f;

    int tempI = 0; 
    u32 firstCodepointOnLine = NU_GetNextCodepoint(string, &tempI);
    float penX = -NU_Get_Glyph(font, firstCodepointOnLine)->bearingX;
    
    int wraps = 0;
    int byteIndex = 0;
    u32 codepoint;
    u32 lastCodepoint = 0;

    while ((codepoint = NU_GetNextCodepoint(string, &byteIndex)) != 0)
    {
        int is_space = (codepoint == ' ' || codepoint == '\t');
        if (is_space)
        {
            // Calculate space advancement
            NU_Glyph* spaceGlyph = NU_Get_Glyph(font, codepoint);
            float spaceAdvance = spaceGlyph->advance;
            if (lastCodepoint != 0) {
                NU_Glyph* preSpaceGlyph = NU_Get_Glyph(font, lastCodepoint);
                spaceAdvance += NU_Get_Kerning(font, preSpaceGlyph->index, spaceGlyph->index);
            }

            // Calculate width of next word
            float nextWordWidth = 0.0f;
            int j = byteIndex; 
            u32 jCodepoint;
            u32 jPrevCodepoint = 0;
            while ((jCodepoint = NU_GetNextCodepoint(string, &j)) != 0 && jCodepoint != ' ' && jCodepoint != '\t')
            {
                NU_Glyph* glyph = NU_Get_Glyph(font, jCodepoint);
                nextWordWidth += glyph->advance;

                // Add kerning
                if (jPrevCodepoint != 0) {
                    NU_Glyph* prevGlyph = NU_Get_Glyph(font, jPrevCodepoint);
                    nextWordWidth += NU_Get_Kerning(font, prevGlyph->index, glyph->index);
                }

                jPrevCodepoint = jCodepoint;
            }
            if (jPrevCodepoint != 0) { // Subtract space after last glyph in next word
                NU_Glyph* jLastGlyph = NU_Get_Glyph(font, jPrevCodepoint);
                nextWordWidth -= jLastGlyph->advance - (jLastGlyph->bearingX + jLastGlyph->width);
            }

            // If next word overflows width -> wrap onto new line
            if (penX + spaceAdvance + nextWordWidth > width + 1.0f)
            {
                tempI = byteIndex; 
                firstCodepointOnLine = NU_GetNextCodepoint(string, &tempI);
                penX = -NU_Get_Glyph(font, firstCodepointOnLine)->bearingX;
                wraps++;
            }
            else
            {
                penX += spaceAdvance;
            }
        }
        // Inside a word
        else
        {
            NU_Glyph* glyph = NU_Get_Glyph(font, codepoint);
            penX += glyph->advance;

            // Add kerning
            if (lastCodepoint != 0) {
                NU_Glyph* prevGlyph = NU_Get_Glyph(font, lastCodepoint);
                penX += NU_Get_Kerning(font, prevGlyph->index, glyph->index);
            }
        }

        lastCodepoint = codepoint;
    }

    return (wraps + 1) * font->line_height;
}

float NU_Calculate_Text_Unwrapped_Width(NU_Font* font, const char* string)
{
    if (string[0] == '\0') return 0.0f;

    u32 codepoint;
    u32 lastCodepoint = 0;
    float width = 0.0f;
    int byteIndex = 0;
    while((codepoint = NU_GetNextCodepoint(string, &byteIndex)) != 0)
    {
        NU_Glyph* glyph = NU_Get_Glyph(font, codepoint);
        width += glyph->advance;
        if (lastCodepoint != 0) {
           NU_Glyph* prevGlyph = NU_Get_Glyph(font, lastCodepoint); 
           width += NU_Get_Kerning(font, prevGlyph->index, glyph->index);
        }
        lastCodepoint = codepoint;
    }

    // Subtract spacing before first glyph and after last glyph;
    int tempI = 0;
    u32 firstCodepoint = NU_GetNextCodepoint(string, &tempI);
    width -= NU_Get_Glyph(font, firstCodepoint)->bearingX;
    if (lastCodepoint != 0) {
        NU_Glyph* lastGlyph = NU_Get_Glyph(font, lastCodepoint);
        width -= lastGlyph->advance - (lastGlyph->bearingX + lastGlyph->width);
    }

    return width;
}

void NU_Add_Glyph_Mesh(Vertex_RGB_UV_List* vertices, Index_List* indices, NU_Glyph* glyph, NU_Font_Atlas* atlas, float penX, float penY, float z, float r, float g, float b)
{
    if (vertices->size + 4 > vertices->capacity) Vertex_RGB_UV_List_Grow(vertices, 4);
    if (indices->size + 6 > indices->capacity) Index_List_Grow(indices, 6);
    float left = penX + glyph->bearingX;
    float top = penY - glyph->bearingY;
    float bottom = top + glyph->height;
    int vertex_offset = vertices->size;
    vertices->array[vertex_offset].x = left;
    vertices->array[vertex_offset].y = top;
    vertices->array[vertex_offset].z = z;
    vertices->array[vertex_offset].r = r;
    vertices->array[vertex_offset].g = g;
    vertices->array[vertex_offset].b = b;
    vertices->array[vertex_offset].u = glyph->u * atlas->invWidth;
    vertices->array[vertex_offset].v = glyph->v * atlas->invHeight;
    vertices->array[vertex_offset + 1].x = left + (float)glyph->width;
    vertices->array[vertex_offset + 1].y = top;
    vertices->array[vertex_offset + 1].z = z;
    vertices->array[vertex_offset + 1].r = r;
    vertices->array[vertex_offset + 1].g = g;
    vertices->array[vertex_offset + 1].b = b;
    vertices->array[vertex_offset + 1].u = (glyph->u + glyph->width) * atlas->invWidth;
    vertices->array[vertex_offset + 1].v = glyph->v * atlas->invHeight;
    vertices->array[vertex_offset + 2].x = left;
    vertices->array[vertex_offset + 2].y = bottom;
    vertices->array[vertex_offset + 2].z = z;
    vertices->array[vertex_offset + 2].r = r;
    vertices->array[vertex_offset + 2].g = g;
    vertices->array[vertex_offset + 2].b = b;
    vertices->array[vertex_offset + 2].u = glyph->u * atlas->invWidth;
    vertices->array[vertex_offset + 2].v = (glyph->v + glyph->height) * atlas->invHeight;
    vertices->array[vertex_offset + 3].x = left + (float)glyph->width;
    vertices->array[vertex_offset + 3].y = bottom;
    vertices->array[vertex_offset + 3].z = z;
    vertices->array[vertex_offset + 3].r = r;
    vertices->array[vertex_offset + 3].g = g;
    vertices->array[vertex_offset + 3].b = b;
    vertices->array[vertex_offset + 3].u = (glyph->u + glyph->width) * atlas->invWidth;
    vertices->array[vertex_offset + 3].v = (glyph->v + glyph->height) * atlas->invHeight;
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

void NU_Generate_Text_Mesh(Vertex_RGB_UV_List* vertices, Index_List* indices, NU_Font* font, const char* string, float x, float y, float z, float r, float g, float b, float maxWidth)
{
    if (string[0] == '\0') return;
    if (maxWidth <= 0.0f) maxWidth = 1e20f;

    // Init penX, penY
    int tempI = 0; 
    u32 firstCodepointOnLine = NU_GetNextCodepoint(string, &tempI);
    float penX = x - NU_Get_Glyph(font, firstCodepointOnLine)->bearingX;
    float penY = y + font->ascent;

    int byteIndex = 0;
    u32 codepoint;
    u32 lastCodepoint = 0;

    while ((codepoint = NU_GetNextCodepoint(string, &byteIndex)) != 0)
    {
        int is_space = (codepoint == ' ' || codepoint == '\t');
        if (is_space)
        {
            // Calculate space advancement
            NU_Glyph* spaceGlyph = NU_Get_Glyph(font, codepoint);
            float spaceAdvance = spaceGlyph->advance;
            if (lastCodepoint != 0) {
                NU_Glyph* preSpaceGlyph = NU_Get_Glyph(font, lastCodepoint);
                spaceAdvance += NU_Get_Kerning(font, preSpaceGlyph->index, spaceGlyph->index);
            }

            // Calculate width of next word
            float nextWordWidth = 0.0f;
            int j = byteIndex; 
            u32 jCodepoint;
            u32 jPrevCodepoint = 0;
            while ((jCodepoint = NU_GetNextCodepoint(string, &j)) != 0 && jCodepoint != ' ' && jCodepoint != '\t')
            {
                NU_Glyph* glyph = NU_Get_Glyph(font, jCodepoint);
                nextWordWidth += glyph->advance;

                // Add kerning
                if (jPrevCodepoint != 0) {
                    NU_Glyph* prevGlyph = NU_Get_Glyph(font, jPrevCodepoint);
                    nextWordWidth += NU_Get_Kerning(font, prevGlyph->index, glyph->index);
                }

                jPrevCodepoint = jCodepoint;
            }
            if (jPrevCodepoint != 0) { // Subtract space after last glyph in next word
                NU_Glyph* jLastGlyph = NU_Get_Glyph(font, jPrevCodepoint);
                nextWordWidth -= jLastGlyph->advance - (jLastGlyph->bearingX + jLastGlyph->width);
            }

            // If next word overflows width -> wrap onto new line
            if (penX - x + spaceAdvance + nextWordWidth > maxWidth)
            {
                tempI = byteIndex; 
                firstCodepointOnLine = NU_GetNextCodepoint(string, &tempI);
                penX = x - NU_Get_Glyph(font, firstCodepointOnLine)->bearingX;
                penY += font->line_height;
                lastCodepoint = 0;
            }
            else
            {
                penX += spaceAdvance;
            }
        }
        else
        {
            NU_Glyph* glyph = NU_Get_Glyph(font, codepoint);
            if (lastCodepoint != 0) {
                NU_Glyph* prevGlyph = NU_Get_Glyph(font, lastCodepoint);
                penX += NU_Get_Kerning(font, prevGlyph->index, glyph->index); // Kerning
            }
            NU_Add_Glyph_Mesh(vertices, indices, glyph, &font->atlas, penX, penY, z, r, g, b);
            penX += glyph->advance;
        }

        lastCodepoint = codepoint;
    }
}


u32 NU_Calculate_Unwrapped_Text_Cursorbytes(NU_Font* font, const char* string, float cursorX)
{
    if (!string || !*string) return 0;

    u32 cursorBytes = 0;
    float width = 0.0f;
    int byteIndex = 0;
    u32 prevCp = 0;

    while (1)
    {
        int currentByteIndex = byteIndex;
        u32 cp = NU_GetNextCodepoint(string, &byteIndex);
        if (cp == 0) break; // end of string

        // Lookup glyph (assumes font->glyphs starts at ASCII 32)
        NU_Glyph* g = NU_Get_Glyph(font, cp - 32);
        if (!g) continue; // skip missing glyphs

        // Add kerning from previous glyph
        if (prevCp) {
            NU_Glyph* prevG = NU_Get_Glyph(font, prevCp - 32);
            width += NU_Get_Kerning(font, prevG->index, g->index);
        }

        float glyphStart = width;
        float glyphEnd = width + g->advance;
        float glyphMid = glyphStart + g->advance * 0.5f;

        // Snap cursor to nearest glyph boundary
        if (cursorX < glyphMid)
        {
            // Cursor is closer to this glyph's left edge
            cursorBytes = currentByteIndex;
            break;
        }
        else
        {
            // Cursor is closer to this glyph's right edge
            cursorBytes = byteIndex;
        }

        width = glyphEnd;
        prevCp = cp;
    }

    return cursorBytes;
}