#pragma once
#include <stdint.h>
#include <string.h>
#include "nu_font.h"

u32 NextUTF8Codepoint(const char* string, int* byteIndex)
{
    unsigned char* p = (unsigned char*)string + *byteIndex;
    u32 cp;

    if (*p == 0) return 0; // end of string

    if ((*p & 0x80) == 0) {
        cp = *p++;
    }
    else if ((*p & 0xE0) == 0xC0 &&
            (p[1] & 0xC0) == 0x80) {
        cp = (*p & 0x1F) << 6 | (p[1] & 0x3F);
        p += 2;
    }
    else if ((*p & 0xF0) == 0xE0 &&
            (p[1] & 0xC0) == 0x80 &&
            (p[2] & 0xC0) == 0x80) {
        cp = (*p & 0x0F) << 12 | (p[1] & 0x3F) << 6 | (p[2] & 0x3F);
        p += 3;
    }
    else if ((*p & 0xF8) == 0xF0 &&
            (p[1] & 0xC0) == 0x80 &&
            (p[2] & 0xC0) == 0x80 &&
            (p[3] & 0xC0) == 0x80) {
        cp = (*p & 0x07) << 18 | (p[1] & 0x3F) << 12 | (p[2] & 0x3F) << 6 | (p[3] & 0x3F);
        p += 4;
    }
    else { // invalid byte
        cp = 0xFFFD;
        p += 1;
    }
    *byteIndex = (int)(p - (unsigned char*)string);
    return cp;
}

u32 NextUTF8CodepointL(const char* string, int* byteIndex, size_t stringLen)
{
    unsigned char* p = (unsigned char*)string + *byteIndex;
    u32 cp;

    if (*byteIndex == stringLen-1) return 0; // end of string

    if ((*p & 0x80) == 0) {
        cp = *p++;
    }
    else if ((*p & 0xE0) == 0xC0 &&
            (p[1] & 0xC0) == 0x80) {
        cp = (*p & 0x1F) << 6 | (p[1] & 0x3F);
        p += 2;
    }
    else if ((*p & 0xF0) == 0xE0 &&
            (p[1] & 0xC0) == 0x80 &&
            (p[2] & 0xC0) == 0x80) {
        cp = (*p & 0x0F) << 12 | (p[1] & 0x3F) << 6 | (p[2] & 0x3F);
        p += 3;
    }
    else if ((*p & 0xF8) == 0xF0 &&
            (p[1] & 0xC0) == 0x80 &&
            (p[2] & 0xC0) == 0x80 &&
            (p[3] & 0xC0) == 0x80) {
        cp = (*p & 0x07) << 18 | (p[1] & 0x3F) << 12 | (p[2] & 0x3F) << 6 | (p[3] & 0x3F);
        p += 4;
    }
    else { // invalid byte
        cp = 0xFFFD;
        p += 1;
    }
    *byteIndex = (int)(p - (unsigned char*)string);
    return cp;
}

float NU_Calculate_Text_Min_Wrap_Width(Font* font, const char* string)
{
    int byteIndex = 0;
    u32 codepoint;
    u32 lastCodepoint = 0;
    u32 wordFirstCodepoint;
    u32 wordLastCodepoint;

    int wordLen = 0;
    float result = 0.0f;
    float wordWidth = 0.0f;

    while ((codepoint = NextUTF8Codepoint(string, &byteIndex)) != 0)
    {
        if (codepoint == '\r') continue;

        int is_space = (codepoint == ' ' || codepoint == '\t');
        if (is_space)
        {
            if (wordLen > 0) {
                Glyph* last  = Font_GetGlyph(font, wordLastCodepoint);

                // Remove space before the first glyph and after the last glyph in the word
                wordWidth -= last->advance;

                // Check if the current word is the longest so far (branchless)
                result += (wordWidth > result) * (wordWidth - result);

                // Reset
                wordWidth = 0.0f;
                wordLen = 0;
            }

            continue;
        }

        Glyph* glyph = Font_GetGlyph(font, codepoint);

        if (wordLen == 0) wordFirstCodepoint = codepoint;

        // Add kerning
        else {
            Glyph* prev = Font_GetGlyph(font, lastCodepoint);
            wordWidth += Font_GetKerning(font, prev->index, glyph->index);
        }

        wordWidth += glyph->advance;
        wordLastCodepoint = codepoint;
        lastCodepoint = codepoint;
        wordLen++;
    }

    // Measure length of final word
    if (wordLen > 0)
    {
        Glyph* first = Font_GetGlyph(font, wordFirstCodepoint);
        Glyph* last  = Font_GetGlyph(font, wordLastCodepoint);

        // Remove space before the first glyph and after the last glyph in the word
        wordWidth -= last->advance;

        // Check if the current word is the longest so far (branchless)
        result += (wordWidth > result) * (wordWidth - result);
    }

    return result;
}

float NU_Calculate_FreeText_Height_From_Wrap_Width(Font* font, const char* string, float width)
{
    if (string[0] == '\0') return 0.0f;


    float penX = 0.0f;
    int wraps = 0;
    int byteIndex = 0;
    u32 codepoint;
    u32 lastCodepoint = 0;

    while ((codepoint = NextUTF8Codepoint(string, &byteIndex)) != 0)
    {
        if (codepoint == '\r') continue;

        int is_space = (codepoint == ' ' || codepoint == '\t');
        if (is_space)
        {
            // Calculate space advancement
            Glyph* spaceGlyph = Font_GetGlyph(font, codepoint);
            float spaceAdvance = spaceGlyph->advance;
            if (lastCodepoint != 0) {
                Glyph* preSpaceGlyph = Font_GetGlyph(font, lastCodepoint);
                spaceAdvance += Font_GetKerning(font, preSpaceGlyph->index, spaceGlyph->index);
            }

            // Calculate width of next word
            float nextWordWidth = 0.0f;
            int j = byteIndex;
            u32 jCodepoint;
            u32 jPrevCodepoint = 0;
            while ((jCodepoint = NextUTF8Codepoint(string, &j)) != 0 && jCodepoint != ' ' && jCodepoint != '\t')
            {
                Glyph* glyph = Font_GetGlyph(font, jCodepoint);
                nextWordWidth += glyph->advance;

                // Add kerning
                if (jPrevCodepoint != 0) {
                    Glyph* prevGlyph = Font_GetGlyph(font, jPrevCodepoint);
                    nextWordWidth += Font_GetKerning(font, prevGlyph->index, glyph->index);
                }

                jPrevCodepoint = jCodepoint;
            }
            if (jPrevCodepoint != 0) { // Subtract space after last glyph in next word
                Glyph* jLastGlyph = Font_GetGlyph(font, jPrevCodepoint);
                nextWordWidth -= jLastGlyph->advance;
            }

            // If next word overflows width -> wrap onto new line
            if (penX + spaceAdvance + nextWordWidth > width)
            {
                penX = 0.0f;
                lastCodepoint = 0;
                wraps++;
            }
            else
            {
                penX += spaceAdvance;
            }
        }
        else if (codepoint == '\n') {
            penX = 0.0f;
            lastCodepoint = 0;
            wraps++;
        }
        // Inside a word
        else
        {
            Glyph* glyph = Font_GetGlyph(font, codepoint);
            penX += glyph->advance;

            // Add kerning
            if (lastCodepoint != 0) {
                Glyph* prevGlyph = Font_GetGlyph(font, lastCodepoint);
                penX += Font_GetKerning(font, prevGlyph->index, glyph->index);
            }
        }

        lastCodepoint = codepoint;
    }

    return (wraps + 1) * font->line_height;
}

float NU_Calculate_LText_Height_From_Wrap_Width(Font* font, const char* string, size_t stringLen, float width)
{
    if (string[0] == '\0') return 0.0f;


    float penX = 0.0f;
    int wraps = 0;
    int byteIndex = 0;
    u32 codepoint;
    u32 lastCodepoint = 0;

    while ((codepoint = NextUTF8CodepointL(string, &byteIndex, stringLen)) != 0)
    {
        if (codepoint == '\r') continue;

        int is_space = (codepoint == ' ' || codepoint == '\t');
        if (is_space)
        {
            // Calculate space advancement
            Glyph* spaceGlyph = Font_GetGlyph(font, codepoint);
            float spaceAdvance = spaceGlyph->advance;
            if (lastCodepoint != 0) {
                Glyph* preSpaceGlyph = Font_GetGlyph(font, lastCodepoint);
                spaceAdvance += Font_GetKerning(font, preSpaceGlyph->index, spaceGlyph->index);
            }

            // Calculate width of next word
            float nextWordWidth = 0.0f;
            int j = byteIndex;
            u32 jCodepoint;
            u32 jPrevCodepoint = 0;
            while ((jCodepoint = NextUTF8Codepoint(string, &j)) != 0 && jCodepoint != ' ' && jCodepoint != '\t')
            {
                Glyph* glyph = Font_GetGlyph(font, jCodepoint);
                nextWordWidth += glyph->advance;

                // Add kerning
                if (jPrevCodepoint != 0) {
                    Glyph* prevGlyph = Font_GetGlyph(font, jPrevCodepoint);
                    nextWordWidth += Font_GetKerning(font, prevGlyph->index, glyph->index);
                }

                jPrevCodepoint = jCodepoint;
            }
            if (jPrevCodepoint != 0) { // Subtract space after last glyph in next word
                Glyph* jLastGlyph = Font_GetGlyph(font, jPrevCodepoint);
                nextWordWidth -= jLastGlyph->advance;
            }

            // If next word overflows width -> wrap onto new line
            if (penX + spaceAdvance + nextWordWidth > width)
            {
                penX = 0.0f;
                lastCodepoint = 0;
                wraps++;
            }
            else
            {
                penX += spaceAdvance;
            }
        }
        else if (codepoint == '\n') {
            penX = 0.0f;
            lastCodepoint = 0;
            wraps++;
        }
        // Inside a word
        else
        {
            Glyph* glyph = Font_GetGlyph(font, codepoint);
            penX += glyph->advance;

            // Add kerning
            if (lastCodepoint != 0) {
                Glyph* prevGlyph = Font_GetGlyph(font, lastCodepoint);
                penX += Font_GetKerning(font, prevGlyph->index, glyph->index);
            }
        }

        lastCodepoint = codepoint;
    }

    return (wraps + 1) * font->line_height;
}

float NU_Calculate_Text_Unwrapped_Width(Font* font, const char* string)
{
    if (!string || string[0] == '\0') return 0.0f;

    int byteIndex = 0;
    u32 codepoint;
    u32 lastCodepoint = 0;

    float width = 0.0f;

    while ((codepoint = NextUTF8Codepoint(string, &byteIndex)) != 0)
    {
        if (codepoint == '\r' || codepoint == '\n') continue;

        Glyph* glyph = Font_GetGlyph(font, codepoint);

        if (lastCodepoint != 0) {
            Glyph* prev = Font_GetGlyph(font, lastCodepoint);
            width += Font_GetKerning(font, prev->index, glyph->index);
        }

        width += glyph->advance;
        lastCodepoint = codepoint;
    }

    return width;
}

float NU_Calculate_LText_Unwrapped_Width(Font* font, const char* string, size_t stringLen)
{
    if (!string || stringLen == 0) return 0.0f;

    int byteIndex = 0;
    u32 codepoint;
    u32 lastCodepoint = 0;

    float width = 0.0f;

    while ((codepoint = NextUTF8CodepointL(string, &byteIndex, stringLen)) != 0)
    {
        if (codepoint == '\r' || codepoint == '\n') continue;

        Glyph* glyph = Font_GetGlyph(font, codepoint);

        if (lastCodepoint != 0) {
            Glyph* prev = Font_GetGlyph(font, lastCodepoint);
            width += Font_GetKerning(font, prev->index, glyph->index);
        }

        width += glyph->advance;
        lastCodepoint = codepoint;
    }

    return width;
}

void NU_Generate_Text_Renderdata(
    Array* glyphRenderData,
    Font* font, const char* string,
    float x, float y, float z,
    u8 r, u8 g, u8 b,
    float maxWidth)
{
    if (string[0] == '\0') return;
    if (maxWidth <= 0.0f) maxWidth = 1e20f;

    // Init penX, penY
    int tempI = 0;
    float penX = x;
    float penY = y + font->ascent;

    int byteIndex = 0;
    u32 codepoint;
    u32 lastCodepoint = 0;

    while ((codepoint = NextUTF8Codepoint(string, &byteIndex)) != 0)
    {
        if (codepoint == '\r') continue;

        int is_space = (codepoint == ' ' || codepoint == '\t');
        if (is_space)
        {
            // Calculate space advancement
            Glyph* spaceGlyph = Font_GetGlyph(font, codepoint);
            float spaceAdvance = spaceGlyph->advance;
            if (lastCodepoint != 0) {
                Glyph* preSpaceGlyph = Font_GetGlyph(font, lastCodepoint);
                spaceAdvance += Font_GetKerning(font, preSpaceGlyph->index, spaceGlyph->index);
            }

            // Calculate width of next word
            float nextWordWidth = 0.0f;
            int j = byteIndex;
            u32 jCodepoint;
            u32 jPrevCodepoint = 0;
            while ((jCodepoint = NextUTF8Codepoint(string, &j)) != 0 && jCodepoint != ' ' && jCodepoint != '\t')
            {
                Glyph* glyph = Font_GetGlyph(font, jCodepoint);
                nextWordWidth += glyph->advance;

                // Add kerning
                if (jPrevCodepoint != 0) {
                    Glyph* prevGlyph = Font_GetGlyph(font, jPrevCodepoint);
                    nextWordWidth += Font_GetKerning(font, prevGlyph->index, glyph->index);
                }

                jPrevCodepoint = jCodepoint;
            }
            if (jPrevCodepoint != 0) { // Subtract space after last glyph in next word
                Glyph* jLastGlyph = Font_GetGlyph(font, jPrevCodepoint);
                nextWordWidth -= jLastGlyph->advance;
            }

            // If next word overflows width -> wrap onto new line
            if (penX - x + spaceAdvance + nextWordWidth > maxWidth)
            {
                penX = x;
                penY += font->line_height;
                lastCodepoint = 0;
            }
            else
            {
                penX += spaceAdvance;
            }
        }
        else if (codepoint == '\n') {
            penX = x;
            penY += font->line_height;
            lastCodepoint = 0;
        }
        else
        {
            Glyph* glyph = Font_GetGlyph(font, codepoint);
            if (lastCodepoint != 0) {
                Glyph* prevGlyph = Font_GetGlyph(font, lastCodepoint);
                penX += Font_GetKerning(font, prevGlyph->index, glyph->index); // Kerning
            }

            // Add glyph render data
            GlyphRenderData* renderData = Array_PushEmpty(glyphRenderData);
            renderData->x = penX + glyph->bearingX;
            renderData->y = penY - glyph->bearingY;
            renderData->z = z;
            renderData->w = (float)glyph->width;
            renderData->h = (float)glyph->height;
            renderData->u0 = glyph->u * font->atlas.invWidth;
            renderData->v0 = glyph->v * font->atlas.invHeight;
            renderData->u1 = (glyph->u + glyph->width) * font->atlas.invWidth;
            renderData->v1 = (glyph->v + glyph->height) * font->atlas.invHeight;
            renderData->colourRGBA = PackRGBA(r, g, b, 255);

            penX += glyph->advance;
        }

        lastCodepoint = codepoint;
    }
}

void NU_Generate_LText_Renderdata(
    Array* glyphRenderData,
    Font* font,
    const char* string,
    size_t stringLen,
    float x, float y, float z,
    u8 r, u8 g, u8 b, float maxWidth)
{
    if (stringLen == 0) return;
    if (maxWidth <= 0.0f) maxWidth = 1e20f;

    // Init penX, penY
    int tempI = 0;
    float penX = x;
    float penY = y + font->ascent;

    int byteIndex = 0;
    u32 codepoint;
    u32 lastCodepoint = 0;

    while ((codepoint = NextUTF8CodepointL(string, &byteIndex, stringLen)) != 0)
    {
        if (codepoint == '\r') continue;

        int is_space = (codepoint == ' ' || codepoint == '\t');
        if (is_space)
        {
            // Calculate space advancement
            Glyph* spaceGlyph = Font_GetGlyph(font, codepoint);
            float spaceAdvance = spaceGlyph->advance;
            if (lastCodepoint != 0) {
                Glyph* preSpaceGlyph = Font_GetGlyph(font, lastCodepoint);
                spaceAdvance += Font_GetKerning(font, preSpaceGlyph->index, spaceGlyph->index);
            }

            // Calculate width of next word
            float nextWordWidth = 0.0f;
            int j = byteIndex;
            u32 jCodepoint;
            u32 jPrevCodepoint = 0;
            while ((jCodepoint = NextUTF8Codepoint(string, &j)) != 0 && jCodepoint != ' ' && jCodepoint != '\t')
            {
                Glyph* glyph = Font_GetGlyph(font, jCodepoint);
                nextWordWidth += glyph->advance;

                // Add kerning
                if (jPrevCodepoint != 0) {
                    Glyph* prevGlyph = Font_GetGlyph(font, jPrevCodepoint);
                    nextWordWidth += Font_GetKerning(font, prevGlyph->index, glyph->index);
                }

                jPrevCodepoint = jCodepoint;
            }
            if (jPrevCodepoint != 0) { // Subtract space after last glyph in next word
                Glyph* jLastGlyph = Font_GetGlyph(font, jPrevCodepoint);
                nextWordWidth -= jLastGlyph->advance;
            }

            // If next word overflows width -> wrap onto new line
            if (penX - x + spaceAdvance + nextWordWidth > maxWidth)
            {
                penX = x;
                penY += font->line_height;
                lastCodepoint = 0;
            }
            else
            {
                penX += spaceAdvance;
            }
        }
        else if (codepoint == '\n') {
            penX = x;
            penY += font->line_height;
            lastCodepoint = 0;
        }
        else
        {
            Glyph* glyph = Font_GetGlyph(font, codepoint);
            if (lastCodepoint != 0) {
                Glyph* prevGlyph = Font_GetGlyph(font, lastCodepoint);
                penX += Font_GetKerning(font, prevGlyph->index, glyph->index); // Kerning
            }

            // Add glyph render data
            GlyphRenderData* renderData = Array_PushEmpty(glyphRenderData);
            renderData->x = penX + glyph->bearingX;
            renderData->y = penY - glyph->bearingY;
            renderData->z = z;
            renderData->w = (float)glyph->width;
            renderData->h = (float)glyph->height;
            renderData->u0 = glyph->u * font->atlas.invWidth;
            renderData->v0 = glyph->v * font->atlas.invHeight;
            renderData->u1 = (glyph->u + glyph->width) * font->atlas.invWidth;
            renderData->v1 = (glyph->v + glyph->height) * font->atlas.invHeight;
            renderData->colourRGBA = PackRGBA(r, g, b, 255);

            penX += glyph->advance;
        }

        lastCodepoint = codepoint;
    }
}

u32 NU_Calculate_Unwrapped_Text_Cursorbytes(Font* font, const char* string, float cursorX)
{
    if (!string || !*string) return 0;

    u32 cursorBytes = 0;
    float width = 0.0f;
    int byteIndex = 0;
    u32 prevCp = 0;

    while (1)
    {
        int currentByteIndex = byteIndex;
        u32 cp = NextUTF8Codepoint(string, &byteIndex);
        if (cp == 0) break; // end of string
        if (cp == '\r') continue;

        Glyph* g = Font_GetGlyph(font, cp);

        // Add kerning from previous glyph
        if (prevCp) {
            Glyph* prevG = Font_GetGlyph(font, prevCp);
            width += Font_GetKerning(font, prevG->index, g->index);
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
