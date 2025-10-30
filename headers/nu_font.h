#pragma once
#include <freetype/freetype.h>
#include <datastructures/vector.h>

FT_Library nu_global_freetype;

typedef struct NU_Glyph {
    int width;
    int height;
    float x_offset;   
    float y_offset;   
    float bearingX;   
    float bearingY;   
    float advance;     
    float uv_x0, uv_y0;    // top-left UV
    float uv_x1, uv_y1;    // bottom-right UV
} NU_Glyph;

typedef struct NU_Font_Atlas {
    unsigned char* buffer;
    int width;
    int height;
    int pen_x;          // For construction only
    int pen_y;          // For construction only
    int tallest_in_row; // For construction only
    uint8_t channels;
    GLuint handle;
} NU_Font_Atlas;

typedef struct NU_Font
{
    Vector glyphs;
    float kerning_table[95][95];
    int height_pixels;
    float y_max;
    float y_min;
    float ascent;
    float descent;
    float line_height;
    NU_Font_Atlas atlas;
    bool subpixel_rendering;
} NU_Font;

void NU_Font_Atlas_Create(NU_Font_Atlas* atlas, int width, int height, uint8_t channels)
{
    atlas->buffer = calloc(width * height, channels);
    atlas->width = width;
    atlas->height = height;
    atlas->pen_x = 0;
    atlas->pen_y = 0;
    atlas->tallest_in_row = 0;
    atlas->channels = channels;
    atlas->handle = 0;
}

void NU_Font_Atlas_Add_Glyph(NU_Font_Atlas* atlas, NU_Glyph* glyph, FT_Bitmap* bmp)
{
    // Wrap onto new row if necessary
    int width_remaining = atlas->width - atlas->pen_x;
    if (width_remaining < glyph->width) {
        atlas->pen_x = 0;
        atlas->pen_y += atlas->tallest_in_row;
        atlas->tallest_in_row = 0;
    }
    atlas->tallest_in_row = MAX(atlas->tallest_in_row, glyph->height);

    // Set glyph UVs
    glyph->uv_x0 = (float)atlas->pen_x / (float)atlas->width;
    glyph->uv_y0 = (float)atlas->pen_y;
    glyph->uv_x1 = (float)(atlas->pen_x + glyph->width) / (float)atlas->width;
    glyph->uv_y1 = (float)(atlas->pen_y + glyph->height);

    // Resize atlas if needed
    if (atlas->pen_y + glyph->height >= atlas->height) {
        size_t old_size = atlas->channels * atlas->width * atlas->height;
        atlas->height += MAX(atlas->height / 2, glyph->height);
        size_t new_size = atlas->channels * atlas->width * atlas->height;
        unsigned char* new_buffer = realloc(atlas->buffer, new_size);
        memset(new_buffer + old_size, 0, new_size - old_size);
        atlas->buffer = new_buffer;
    }

    // Copy row by row
    for (int row = 0; row < glyph->height; row++) {
        unsigned char* src = bmp->buffer + row * bmp->pitch;
        unsigned char* dst = atlas->buffer + ((atlas->pen_y + row) * atlas->width + atlas->pen_x) * atlas->channels;
        memcpy(dst, src, glyph->width * atlas->channels); // copy full row in memory
    }

    atlas->pen_x += glyph->width;
}

void NU_Normalise_Glyph_UVs(NU_Font* font)
{
    for (uint32_t i=0; i<font->glyphs.size; i++)
    {
        NU_Glyph* glyph = (NU_Glyph*)Vector_Get(&font->glyphs, i);
        glyph->uv_y0 /= font->atlas.height;
        glyph->uv_y1 /= font->atlas.height;
    }
}

void NU_Font_Atlas_To_GPU(NU_Font_Atlas* atlas)
{
    if (!atlas || !atlas->buffer) return;

    // Generate a texture handle
    glGenTextures(1, &atlas->handle);
    glBindTexture(GL_TEXTURE_2D, atlas->handle);

    // Set texture parameters (wrap and filter)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Determine OpenGL format based on channels
    GLenum format;
    if (atlas->channels == 1) {
        format = GL_RED;
    } 
    else if (atlas->channels == 3) {
        format = GL_RGB;
    } 
    else 
    {
        fprintf(stderr, "NU_Error: unsupported number of channels: %d\n", atlas->channels);
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

    // Upload the texture to the GPU
    glTexImage2D(GL_TEXTURE_2D, 0, format, atlas->width, atlas->height, 0, format, GL_UNSIGNED_BYTE, atlas->buffer);

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
}

int NU_Font_Create(NU_Font* font, const char* filepath, int height_pixels, bool subpixel_rendering)
{
    height_pixels = min(height_pixels, 256);
    font->subpixel_rendering = subpixel_rendering;

    uint8_t channels = 1;
    FT_Int32 load_flags = FT_LOAD_DEFAULT | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_LIGHT;
    FT_Int32 render_flags = FT_RENDER_MODE_NORMAL;
    if (font->subpixel_rendering) {
        channels = 3;
        load_flags |= FT_LOAD_TARGET_LCD;
        render_flags = FT_RENDER_MODE_LCD;
    }

    FT_Face face;
    if (FT_New_Face(nu_global_freetype, filepath, 0, &face)) {
        printf("Error! Could not find font: %s\n", filepath);
        return 0;
    }

    // Update height pixels
    FT_Set_Pixel_Sizes(face, 0, (FT_UInt)height_pixels);
    FT_Size_Metrics* metrics = &face->size->metrics;
    font->height_pixels = metrics->height >> 6;                      // convert from 1/64th pixels to integer pixels
    font->y_max         = (float)(face->bbox.yMax >> 6);
    font->y_min         = (float)(face->bbox.yMin >> 6);
    font->ascent        = (float)(face->size->metrics.ascender >> 6);
    font->descent       = (float)(face->size->metrics.descender >> 6);
    font->line_height   = (float)(face->size->metrics.height >> 6);


    // Init font storage
    Vector_Reserve(&font->glyphs, sizeof(NU_Glyph), 95);
    NU_Font_Atlas_Create(&font->atlas, 512, 128, channels);

    // Render and save each ASCII glyph 32..126
    for (char glyph_char = 32; glyph_char <= 126; glyph_char++)
    {
        FT_ULong character = glyph_char;
        FT_UInt glyph_index = FT_Get_Char_Index(face, character);
        if (FT_Load_Glyph(face, glyph_index, load_flags)) {
            fprintf(stderr, "Failed to load glyph: %c\n", glyph_char);
            continue;
        }
        if (FT_Render_Glyph(face->glyph, render_flags)) {
            fprintf(stderr, "Failed to render glyph: %c\n", glyph_char);
            continue;
        }
        FT_Bitmap* bmp = &face->glyph->bitmap;
        
        // Store glyph metrics
        NU_Glyph glyph;
        glyph.width    = (int)bmp->width / channels; 
        glyph.height   = (int)bmp->rows;
        glyph.bearingX = (float)(face->glyph->metrics.horiBearingX >> 6);
        glyph.bearingY = (float)(face->glyph->metrics.horiBearingY >> 6);
        glyph.advance  = (float)(face->glyph->advance.x >> 6); // FreeType uses 1/64th pixel units
        glyph.x_offset = (float)(glyph.advance - (float)(glyph.width)) * 0.5f;
        glyph.y_offset = (float)(face->bbox.yMax >> 6) - glyph.bearingY;
        Vector_Push(&font->glyphs, &glyph);
        NU_Glyph* stored_glyph = Vector_Get(&font->glyphs, glyph_char - 32);

        // Store bitmap in font atlas
        NU_Font_Atlas_Add_Glyph(&font->atlas, stored_glyph, bmp);
    }

    NU_Normalise_Glyph_UVs(font);
    NU_Font_Atlas_To_GPU(&font->atlas);

    // Precompute kerning table for all ASCII 32..126 pairs
    for (char left_char = 32; left_char <= 126; left_char++) {
        FT_UInt left_index = FT_Get_Char_Index(face, left_char);
        for (char right_char = 32; right_char <= 126; right_char++) {
            FT_UInt right_index = FT_Get_Char_Index(face, right_char);
            FT_Vector kern;
            if (FT_Get_Kerning(face, left_index, right_index, FT_KERNING_UNSCALED, &kern) != 0) {
                kern.x = 0; // fallback on error
            }

            // Convert 26.6 fixed point to pixels
            float kern_pixels = kern.x >> 6;
            font->kerning_table[left_char - 32][right_char - 32] = kern_pixels;

            if (kern_pixels != 0) {
                printf("kerning: %f\n", kern_pixels);
                printf("kerning stored: %f\n", font->kerning_table[left_char - 32][right_char - 32]);
            }
        }
    }


    FT_Done_Face(face);
    return 1; // Success
}

void NU_Font_Free(NU_Font* font)
{
    free(font->atlas.buffer);
    Vector_Free(&font->glyphs);
}