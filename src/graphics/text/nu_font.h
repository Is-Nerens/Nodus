#pragma once
#include "vulkan/vulkan_core.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H
#include "nu_default_font.h"
#include "nu_font_gpu_image.h"

FT_Library nu_global_freetype;

enum FontAtlasState {
    FONT_ATLAS_STATE_MUST_UPLOAD,
    FONT_ATLAS_STATE_UPDATED,
    FONT_ATLAS_STATE_MUST_PATCH,
    FONT_ATLAS_STATE_MUST_RESIZE,
};

typedef struct {
    FT_UInt index;
    u16 width;
    u16 height;
    float bearingX;
    float bearingY;
    float advance;
    float u, v;    // top-left UV (not normalised)
} Glyph;

typedef struct {
    unsigned char* buffer;
    int width;
    int height;
    float invWidth;
    float invHeight;
    int penX;           // For construction only
    int penY;           // For construction only
    int tallestInRow;   // For construction only;
    FontImageGPU fontImageGPU;
    enum FontAtlasState updateState;
    bool subpixelRendering;
} FontAtlas;

typedef struct
{
    FT_Face face;
    FontAtlas atlas;
    Array Ascii_Glyphs;
    Hashmap UTF8_Glyphs;
    int heightPixels;
    int fontWeight;
    float y_max;
    float y_min;
    float ascent;
    float descent;
    float line_height;
    FT_Int32 loadFlags;
    FT_Int32 renderFlags;
    bool subpixelRendering;
} Font;

void FontAtlas_Create(FontAtlas* atlas, int width, int height, bool subpixelRendering)
{
    int channels = 1; if (subpixelRendering) channels = 4;
    atlas->buffer = calloc(width * height, channels);
    atlas->width = width;
    atlas->height = height;
    atlas->invWidth = 1.0f / (float)(atlas->width);
    atlas->invHeight = 1.0f / (float)(atlas->height);
    atlas->penX = 0;
    atlas->penY = 0;
    atlas->tallestInRow = 0;
    atlas->subpixelRendering = subpixelRendering;
    atlas->updateState = FONT_ATLAS_STATE_MUST_UPLOAD;
}

void FontAtlas_AddGlyph(FontAtlas* atlas, Glyph* glyph, FT_Bitmap* bmp)
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

    int channels = 1; if (atlas->subpixelRendering) channels = 4;

    // Resize atlas if needed
    if (atlas->penY + glyph->height >= atlas->height) {
        size_t old_size = channels * atlas->width * atlas->height;
        atlas->height += max(atlas->height / 2, glyph->height);
        size_t new_size = channels * atlas->width * atlas->height;
        unsigned char* new_buffer = realloc(atlas->buffer, new_size);
        memset(new_buffer + old_size, 0, new_size - old_size);
        atlas->invWidth = 1.0f / (float)(atlas->width);
        atlas->invHeight = 1.0f / (float)(atlas->height);
        atlas->buffer = new_buffer;
        if (atlas->updateState != FONT_ATLAS_STATE_MUST_UPLOAD) atlas->updateState = FONT_ATLAS_STATE_MUST_RESIZE;
    }
    else {
        if (atlas->updateState != FONT_ATLAS_STATE_MUST_UPLOAD) atlas->updateState = FONT_ATLAS_STATE_MUST_PATCH;
    }

    // Copy row by row
    for (int row = 0; row < glyph->height; row++) {
        unsigned char* src = bmp->buffer + row * bmp->pitch;
        unsigned char* dst = atlas->buffer + ((atlas->penY + row) * atlas->width + atlas->penX) * channels;

        if (atlas->subpixelRendering) {
            // FreeType LCD bitmap is 3 bytes/px; atlas is 4 bytes/px (RGBA)
            for (int x = 0; x < glyph->width; x++) {
                dst[x * 4 + 0] = src[x * 3 + 0];
                dst[x * 4 + 1] = src[x * 3 + 1];
                dst[x * 4 + 2] = src[x * 3 + 2];
                dst[x * 4 + 3] = 255;
            }
        }
        else {
            memcpy(dst, src, glyph->width);
        }
    }

    atlas->penX += glyph->width;
}

void FontAtlas_UpdateGPU(
    FontAtlas* atlas,
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue)
{
    if (!atlas || !atlas->buffer) return;

    // First upload
    if (atlas->updateState == FONT_ATLAS_STATE_MUST_UPLOAD) {
        FontImageGPU_Create(&atlas->fontImageGPU, device, physicalDevice, commandPool, graphicsQueue, atlas->width, atlas->height, atlas->buffer, !atlas->subpixelRendering);
    }
    // Patch update
    else if (atlas->updateState == FONT_ATLAS_STATE_MUST_PATCH) {
        FontImageGPU_Patch(&atlas->fontImageGPU, device, physicalDevice, commandPool, graphicsQueue, 0, 0, atlas->width, atlas->height, atlas->buffer);
    }
    // Resize update
    else if (atlas->updateState == FONT_ATLAS_STATE_MUST_RESIZE) {
        FontImageGPU_Resize(&atlas->fontImageGPU, device, physicalDevice, commandPool, graphicsQueue, atlas->width, atlas->height, atlas->buffer);
    }

    // Set update state
    atlas->updateState = FONT_ATLAS_STATE_UPDATED;
}

int Font_CreateFromFace(Font* font, FT_Face face, int heightPixels, int fontWeight, bool subpixelRendering)
{
    heightPixels = min(heightPixels, 256);
    font->subpixelRendering = subpixelRendering;

    int channels = 1;
    font->loadFlags = FT_LOAD_DEFAULT | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_LIGHT;
    font->renderFlags = FT_RENDER_MODE_NORMAL;
    if (font->subpixelRendering) {
        channels = 4;
        font->loadFlags |= FT_LOAD_TARGET_LCD;
        font->renderFlags = FT_RENDER_MODE_LCD;
    }

    // Set font height
    FT_Set_Pixel_Sizes(face, 0, (FT_UInt)heightPixels);

    // Set font weight (if variable font)
    if (face->face_flags & FT_FACE_FLAG_MULTIPLE_MASTERS) {
        FT_MM_Var* mm = NULL;
        if (FT_Get_MM_Var(face, &mm) == 0) {
            // Allocate coords for ALL axes
            FT_Fixed* coords = (FT_Fixed*)malloc(mm->num_axis * sizeof(FT_Fixed));

            // Get current design coordinates first
            FT_Get_Var_Design_Coordinates(face, mm->num_axis, coords);

            for (int i = 0; i < mm->num_axis; i++) {
                if (mm->axis[i].tag == FT_MAKE_TAG('w','g','h','t')) {
                    FT_Var_Axis* axis = &mm->axis[i];
                    float t = (fontWeight - 100.0f) / 800.0f;
                    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
                    coords[i] = axis->minimum + (FT_Fixed)((axis->maximum - axis->minimum) * t);
                    break;
                }
            }

            FT_Set_Var_Design_Coordinates(face, mm->num_axis, coords);
            free(coords);
            FT_Done_MM_Var(face->glyph->library, mm);
        }
    }

    FT_Size_Metrics* metrics = &face->size->metrics;
    font->heightPixels = metrics->height >> 6;
    font->y_max         = (float)(face->bbox.yMax >> 6);
    font->y_min         = (float)(face->bbox.yMin >> 6);
    font->ascent        = (float)(face->size->metrics.ascender >> 6);
    font->descent       = (float)(face->size->metrics.descender >> 6);
    font->line_height   = (float)(face->size->metrics.height >> 6);

    // Init font storage and datastructures
    Array_Init(&font->Ascii_Glyphs, sizeof(Glyph), 128);
    Hashmap_Init(&font->UTF8_Glyphs, sizeof(u32), sizeof(Glyph), 256);
    FontAtlas_Create(&font->atlas, 512, 512, subpixelRendering);

    // Render and save each ASCII glyph 32..126
    for (char glyph_char = 32; glyph_char < 127; glyph_char++)
    {
        FT_ULong character = glyph_char;
        FT_UInt glyph_index = FT_Get_Char_Index(face, character);
        if (FT_Load_Glyph(face, glyph_index, font->loadFlags)) continue;
        if (FT_Render_Glyph(face->glyph, font->renderFlags)) continue;
        FT_Bitmap* bmp = &face->glyph->bitmap;

        // Store glyph metrics
        Glyph glyph;
        glyph.index    = glyph_index;
        glyph.width    = (u16)bmp->width;
        glyph.height   = (u16)bmp->rows;
        glyph.bearingX = face->glyph->bitmap_left;
        glyph.bearingY = face->glyph->bitmap_top;
        glyph.advance  = (float)(face->glyph->advance.x >> 6);
        Array_Push(&font->Ascii_Glyphs, &glyph);
        Glyph* stored_glyph = Array_Get(&font->Ascii_Glyphs, glyph_char - 32);

        // Store bitmap in font atlas
        FontAtlas_AddGlyph(&font->atlas, stored_glyph, bmp);
    }

    font->face = face;
    return 1; // Success
}

Glyph* Font_AddUncachedGlyph(Font* font, u32 codepoint)
{
    // Render glyph
    FT_UInt glyph_index = FT_Get_Char_Index(font->face, codepoint);
    if (FT_Load_Glyph(font->face, glyph_index, font->loadFlags)) return Array_Get(&font->Ascii_Glyphs, 63);
    if (FT_Render_Glyph(font->face->glyph, font->renderFlags)) return Array_Get(&font->Ascii_Glyphs, 63);
    FT_Bitmap* bmp = &font->face->glyph->bitmap;

    int channels = 1;
    if (font->subpixelRendering) channels = 4;

    // Store glyph metrics
    Glyph glyph;
    glyph.index    = glyph_index;
    glyph.width    = (u16)bmp->width;
    glyph.height   = (u16)bmp->rows;
    glyph.bearingX = font->face->glyph->bitmap_left;
    glyph.bearingY = font->face->glyph->bitmap_top;
    glyph.advance  = (float)(font->face->glyph->advance.x >> 6);
    Hashmap_Set(&font->UTF8_Glyphs, &codepoint, &glyph);
    Glyph* stored_glyph = Hashmap_Get(&font->UTF8_Glyphs, &codepoint);

    // Store bitmap in font atlas
    FontAtlas_AddGlyph(&font->atlas, stored_glyph, bmp);
    return stored_glyph;
}

int Font_Create(Font* font, const char* filepath, int heightPixels, int fontWeight, bool subpixelRendering)
{
    FT_Face face;
    FT_Error error = FT_New_Face(nu_global_freetype, filepath, 0, &face);
    if (error) return 0;

    return Font_CreateFromFace(font, face, heightPixels, fontWeight, subpixelRendering);
}

int Font_CreateDefault(Font* font, int heightPixels, int fontWeight, bool subpixelRendering)
{
    FT_Face face;
    FT_Error error = FT_New_Memory_Face(nu_global_freetype, (unsigned char*)nu_default_ttf, nu_default_ttf_len, 0, &face);
    if (error) {
        return 0;
    }

    return Font_CreateFromFace(font, face, heightPixels, fontWeight, subpixelRendering);
}

void Font_Free(Font* font, VkDevice device)
{
    FT_Done_Face(font->face);
    Array_Free(&font->Ascii_Glyphs);
    Hashmap_Free(&font->UTF8_Glyphs);
    free(font->atlas.buffer);
    FontImageGPU_Destroy(&font->atlas.fontImageGPU, device);
}

Glyph* Font_GetGlyph(Font* font, u32 codepoint)
{
    // Ascii hot path
    if (codepoint < 128) {
        // Fallback to '?' for ascii control characters
        if ((unsigned)(codepoint - 32) < 95) return Array_Get(&font->Ascii_Glyphs, codepoint - 32);
        return Array_Get(&font->Ascii_Glyphs, '?' - 32);
    }

    // Slower non-ascii lookup
    Glyph* glyph = (Glyph*)Hashmap_Get(&font->UTF8_Glyphs, &codepoint);
    if (glyph) return glyph;

    // Really slow non-ascii non-cached glyph
    return Font_AddUncachedGlyph(font, codepoint);
}

inline float Font_GetKerning(Font* font, FT_UInt leftIndex, FT_UInt rightIndex)
{
    FT_Vector kern; kern.x = 0.0f;
    FT_Get_Kerning(font->face, leftIndex, rightIndex, FT_KERNING_UNSCALED, &kern);
    return kern.x >> 6;
}
