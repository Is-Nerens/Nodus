#pragma once

typedef struct {
    unsigned char* buffer;
    u16 w, h;
    u16 x, y, shelfHeight;
    int imageResourceIndex;
} AtlasBuilder;

void AtlasBuilder_Init(AtlasBuilder* a, u16 w, u16 h)
{
    a->w = w;
    a->h = h;
    a->x = 0;
    a->y = 0;
    a->shelfHeight = 0;
    a->buffer = (unsigned char*)malloc(w * h * 4);
    memset(a->buffer, 0, w * h * 4);
}

void AtlasBuilder_Free(AtlasBuilder* a)
{
    free(a->buffer);
}

void AtlasBuilder_Clear(AtlasBuilder* a)
{
    a->imageResourceIndex = -1;
    a->x = 0;
    a->y = 0;
    a->shelfHeight = 0;
    memset(a->buffer, 0, a->w * a->h * 4);
}

bool AtlasBuilder_CanFit(AtlasBuilder* a, int w, int h)
{
    if (w > a->w) return 0;
    int y = (a->x + w > a->w) ? a->y + a->shelfHeight : a->y;
    return y + h <= a->h;
}

static void AtlasBuilder_AddImage(
    AtlasBuilder* a,
    const unsigned char* src,
    int imgW, int imgH,
    u16* outX, u16* outY)
{
    // New row if needed
    if (a->x + imgW > a->w) {
        a->y += a->shelfHeight;
        a->x = 0;
        a->shelfHeight = 0;
    }

    // Write position
    int x = a->x;
    int y = a->y;

    // Blit
    for (int row = 0; row < imgH; row++) {
        unsigned char* dst = &a->buffer[((y + row) * a->w + x) * 4];
        const unsigned char* srcRow = &src[row * imgW * 4];
        memcpy(dst, srcRow, imgW * 4);
    }

    // Advance cursor
    a->x += imgW;
    if (imgH > a->shelfHeight) a->shelfHeight = imgH;
    *outX = x;
    *outY = y;
}
