#pragma once
#include <freetype/freetype.h>
#include "nu_default_font.h"

FT_Library nu_global_freetype;

typedef struct NU_Glyph {
    FT_UInt index;
    u16 width;
    u16 height; 
    float bearingX;   
    float bearingY;   
    float advance;     
    float u, v;    // top-left UV (not normalised)
} NU_Glyph;

typedef struct NU_Font_Atlas {
    unsigned char* buffer;
    int width;
    int height;
    float invWidth;
    float invHeight;
    int penX;           // For construction only
    int penY;           // For construction only
    int tallestInRow;   // For construction only
    int channels;
    GLuint handle;
    bool modifiedOnCPU;
    bool resizedOnCPU;
} NU_Font_Atlas;

typedef struct NU_Font
{
    FT_Face face;
    Array Ascii_Glyphs;
    Hashmap UTF8_Glyphs;
    int height_pixels;
    int fontWeight;
    float y_max;
    float y_min;
    float ascent;
    float descent;
    float line_height;
    FT_Int32 loadFlags;
    FT_Int32 renderFlags;
    NU_Font_Atlas atlas;
    bool subpixel_rendering;
} NU_Font;

void NU_Font_Atlas_Create(NU_Font_Atlas* atlas, int width, int height, int channels)
{
    atlas->buffer = calloc(width * height, channels);
    atlas->width = width;
    atlas->height = height;
    atlas->invWidth = 1.0f / (float)(atlas->width);
    atlas->invHeight = 1.0f / (float)(atlas->height);
    atlas->penX = 0;
    atlas->penY = 0;
    atlas->tallestInRow = 0;
    atlas->channels = channels;
    atlas->handle = 0;
}

void NU_Font_Atlas_Add_Glyph(NU_Font_Atlas* atlas, NU_Glyph* glyph, FT_Bitmap* bmp)
{
    // Wrap onto new row if necessary
    int width_remaining = atlas->width - atlas->penX;
    if (width_remaining < glyph->width) {
        atlas->penX = 0;
        atlas->penY += atlas->tallestInRow;
        atlas->tallestInRow = 0;
    }
    atlas->tallestInRow = MAX(atlas->tallestInRow, glyph->height);

    // Set glyph UVs
    glyph->u = (float)atlas->penX;
    glyph->v = (float)atlas->penY;

    // Resize atlas if needed
    if (atlas->penY + glyph->height >= atlas->height) {
        size_t old_size = atlas->channels * atlas->width * atlas->height;
        atlas->height += MAX(atlas->height / 2, glyph->height);
        size_t new_size = atlas->channels * atlas->width * atlas->height;
        unsigned char* new_buffer = realloc(atlas->buffer, new_size);
        memset(new_buffer + old_size, 0, new_size - old_size);
        atlas->invWidth = 1.0f / (float)(atlas->width);
        atlas->invHeight = 1.0f / (float)(atlas->height);
        atlas->buffer = new_buffer;
        atlas->resizedOnCPU = true;
    }

    // Copy row by row
    for (int row = 0; row < glyph->height; row++) {
        unsigned char* src = bmp->buffer + row * bmp->pitch;
        unsigned char* dst = atlas->buffer + ((atlas->penY + row) * atlas->width + atlas->penX) * atlas->channels;
        memcpy(dst, src, glyph->width * atlas->channels); // copy full row in memory
    }

    atlas->penX += glyph->width;
    atlas->modifiedOnCPU = true;
}

void NU_Font_Atlas_Upload_Or_Modify_GPU(NU_Font_Atlas* atlas)
{
    if (!atlas || !atlas->buffer) return;

    GLenum format;
    if (atlas->channels == 1) format = GL_RED;
    else if (atlas->channels == 3) format = GL_RGB;
    else return;

    // First upload
    if (atlas->handle == 0) {

        // Generate a texture handle
        glGenTextures(1, &atlas->handle);
        glBindTexture(GL_TEXTURE_2D, atlas->handle);

        // Set texture parameters (wrap and filter)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Upload the texture to the GPU
        glTexImage2D(GL_TEXTURE_2D, 0, format, atlas->width, atlas->height, 0, format, GL_UNSIGNED_BYTE, atlas->buffer);
    }
    // Patch update
    else if (atlas->modifiedOnCPU && !atlas->resizedOnCPU) {
        glBindTexture(GL_TEXTURE_2D, atlas->handle);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, atlas->width, atlas->height, format, GL_UNSIGNED_BYTE, atlas->buffer);
        atlas->modifiedOnCPU = false;
    }
    // Resize update
    else if (atlas->resizedOnCPU) {
        glBindTexture(GL_TEXTURE_2D, atlas->handle);
        glTexImage2D(GL_TEXTURE_2D, 0, format, atlas->width, atlas->height, 0, format, GL_UNSIGNED_BYTE, atlas->buffer);
        atlas->modifiedOnCPU = false;
        atlas->resizedOnCPU = false;
    }

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
}

int NU_Create_Font_From_Face(NU_Font* font, FT_Face face, int height_pixels, bool subpixel_rendering)
{
    height_pixels = min(height_pixels, 256);
    font->subpixel_rendering = subpixel_rendering;

    int channels = 1;
    font->loadFlags = FT_LOAD_DEFAULT | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_LIGHT;
    font->renderFlags = FT_RENDER_MODE_NORMAL;
    if (font->subpixel_rendering) {
        channels = 3;
        font->loadFlags |= FT_LOAD_TARGET_LCD;
        font->renderFlags = FT_RENDER_MODE_LCD;
    }

    // Update height pixels
    FT_Set_Pixel_Sizes(face, 0, (FT_UInt)height_pixels);
    FT_Size_Metrics* metrics = &face->size->metrics;
    font->height_pixels = metrics->height >> 6;
    font->y_max         = (float)(face->bbox.yMax >> 6);
    font->y_min         = (float)(face->bbox.yMin >> 6);
    font->ascent        = (float)(face->size->metrics.ascender >> 6);
    font->descent       = (float)(face->size->metrics.descender >> 6);
    font->line_height   = (float)(face->size->metrics.height >> 6);

    // Init font storage
    Array_Init(&font->Ascii_Glyphs, sizeof(NU_Glyph), 128);
    Hashmap_Init(&font->UTF8_Glyphs, sizeof(u32), sizeof(NU_Glyph), 256);
    NU_Font_Atlas_Create(&font->atlas, 512, 512, channels);

    // Render and save each ASCII glyph 32..126
    for (char glyph_char = 32; glyph_char < 127; glyph_char++)
    {
        FT_ULong character = glyph_char;
        FT_UInt glyph_index = FT_Get_Char_Index(face, character);
        if (FT_Load_Glyph(face, glyph_index, font->loadFlags)) continue;
        if (FT_Render_Glyph(face->glyph, font->renderFlags)) continue;
        FT_Bitmap* bmp = &face->glyph->bitmap;
        
        // Store glyph metrics
        NU_Glyph glyph;
        glyph.index    = glyph_index;
        glyph.width    = (u16)bmp->width / channels;
        glyph.height   = (u16)bmp->rows;
        glyph.bearingX = face->glyph->bitmap_left;
        glyph.bearingY = face->glyph->bitmap_top;
        glyph.advance  = (float)(face->glyph->advance.x >> 6);
        Array_Push(&font->Ascii_Glyphs, &glyph);
        NU_Glyph* stored_glyph = Array_Get(&font->Ascii_Glyphs, glyph_char - 32);

        // Store bitmap in font atlas
        NU_Font_Atlas_Add_Glyph(&font->atlas, stored_glyph, bmp);
    }

    NU_Font_Atlas_Upload_Or_Modify_GPU(&font->atlas);

    font->face = face;
    return 1; // Success
}

NU_Glyph* NU_Add_Uncached_Glyph(NU_Font* font, u32 codepoint)
{
    // Render glyph
    FT_UInt glyph_index = FT_Get_Char_Index(font->face, codepoint);
    if (FT_Load_Glyph(font->face, glyph_index, font->loadFlags)) return Array_Get(&font->Ascii_Glyphs, 63);
    if (FT_Render_Glyph(font->face->glyph, font->renderFlags)) return Array_Get(&font->Ascii_Glyphs, 63);
    FT_Bitmap* bmp = &font->face->glyph->bitmap;

    int channels = 1;
    if (font->subpixel_rendering) channels = 3;

    // Store glyph metrics
    NU_Glyph glyph;
    glyph.index    = glyph_index;
    glyph.width    = (u16)bmp->width / channels;
    glyph.height   = (u16)bmp->rows;
    glyph.bearingX = font->face->glyph->bitmap_left;
    glyph.bearingY = font->face->glyph->bitmap_top;
    glyph.advance  = (float)(font->face->glyph->advance.x >> 6);
    Hashmap_Set(&font->UTF8_Glyphs, &codepoint, &glyph);
    NU_Glyph* stored_glyph = Hashmap_Get(&font->UTF8_Glyphs, &codepoint);

    // Store bitmap in font atlas
    NU_Font_Atlas_Add_Glyph(&font->atlas, stored_glyph, bmp);
    return stored_glyph;
}

int NU_Font_Create(NU_Font* font, const char* filepath, int height_pixels, bool subpixel_rendering)
{
    FT_Face face;
    FT_Error error = FT_New_Face(nu_global_freetype, filepath, 0, &face);
    if (error) {
        return 0;
    }

    return NU_Create_Font_From_Face(font, face, height_pixels, subpixel_rendering);
}

int NU_Font_Create_Default(NU_Font* font, int height_pixels, bool subpixel_rendering)
{
    FT_Face face;
    FT_Error error = FT_New_Memory_Face(nu_global_freetype, (unsigned char*)nu_default_ttf, nu_default_ttf_len, 0, &face);
    if (error) {
        return 0;
    }
    
    return NU_Create_Font_From_Face(font, face, height_pixels, subpixel_rendering);
}

void NU_Font_Free(NU_Font* font)
{
    FT_Done_Face(font->face);
    Array_Free(&font->Ascii_Glyphs);
    Hashmap_Free(&font->UTF8_Glyphs);
    free(font->atlas.buffer);
}

NU_Glyph* NU_Get_Glyph(NU_Font* font, u32 codepoint)
{
    // Ascii hot path
    if (codepoint < 128) {
        // Fallback to '?' for ascii control characters
        if ((unsigned)(codepoint - 32) < 95) {
            return Array_Get(&font->Ascii_Glyphs, codepoint - 32);
        }
        return Array_Get(&font->Ascii_Glyphs, '?' - 32);
    }

    // Slower non-ascii lookup
    NU_Glyph* glyph = (NU_Glyph*)Hashmap_Get(&font->UTF8_Glyphs, &codepoint);
    if (glyph) return glyph;

    // Really slow non-ascii non-cached glyph
    return NU_Add_Uncached_Glyph(font, codepoint);
}

inline float NU_Get_Kerning(NU_Font* font, FT_UInt leftIndex, FT_UInt rightIndex)
{
    FT_Vector kern; kern.x = 0.0f;
    FT_Get_Kerning(font->face, leftIndex, rightIndex, FT_KERNING_UNSCALED, &kern);
    return kern.x >> 6;
}