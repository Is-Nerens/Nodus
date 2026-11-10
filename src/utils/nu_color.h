#pragma once

typedef struct {
    uint8 r, g, b, a;
} NuColor;

static inline int32 HexValue(char c)
{
    if ((unsigned)(c - '0') <= 9) return c - '0';
    c |= 0x20; // Convert uppercase ASCII to lowercase
    if ((unsigned)(c - 'a') <= 5) return c - 'a' + 10;
    return -1;
}

static inline int32 PackColorIntoInt32(NuColor color)
{
    return (int32)(
        ((uint32)color.r << 24) |
        ((uint32)color.g << 16) |
        ((uint32)color.b <<  8) |
        ((uint32)color.a)
    );
}

static inline uint32 PackColorIntoUint32(NuColor color)
{
    return ((uint32)color.r << 24) |
    ((uint32)color.g << 16) |
    ((uint32)color.b <<  8) |
    ((uint32)color.a);
}

static inline float32 PackColorIntoFloat32(NuColor color)
{
    union {
        uint32 u;
        float32 f;
    } value;
    value.u = ((uint32)color.r << 24) |
              ((uint32)color.g << 16) |
              ((uint32)color.b << 8) |
              (uint32)color.a;
    return value.f;
}

static inline NuColor UnpackInt32IntoColor(int32 i)
{
    uint32 value = (uint32)i;

    NuColor color = {
        (uint8)(value >> 24),
        (uint8)(value >> 16),
        (uint8)(value >> 8),
        (uint8)value
    };

    return color;
}

static inline NuColor UnpackFloat32IntoColor(float32 f)
{
    union {
        uint32 u;
        float32 f;
    } value;
    value.f = f;
    uint32 bits = value.u;
    NuColor color = {
        (uint8)(bits >> 24),
        (uint8)(bits >> 16),
        (uint8)(bits >> 8),
        (uint8)bits
    };
    return color;
}

NuColor ColourFromHexcode(const char* hexcode)
{
    NuColor black = { 0, 0, 0, 255 };
    if (!hexcode) return black;
    uint32 offset = (hexcode[0] == '#') ? 1 : 0;
    hexcode += offset;
    if (hexcode[0] == '\0' || hexcode[1] == '\0' ||
        hexcode[2] == '\0' || hexcode[3] == '\0' ||
        hexcode[4] == '\0' || hexcode[5] == '\0') return black;
    int32 r1 = HexValue(hexcode[0]);
    int32 r2 = HexValue(hexcode[1]);
    int32 g1 = HexValue(hexcode[2]);
    int32 g2 = HexValue(hexcode[3]);
    int32 b1 = HexValue(hexcode[4]);
    int32 b2 = HexValue(hexcode[5]);
    if (r1 < 0 || r2 < 0 ||
        g1 < 0 || g2 < 0 ||
        b1 < 0 || b2 < 0) {
        return black;
    }
    uint8 alpha = 255;
    if (hexcode[6] != '\0') {
        if (hexcode[7] == '\0' || hexcode[8] != '\0') return black;
        int32 a1 = HexValue(hexcode[6]);
        int32 a2 = HexValue(hexcode[7]);
        if (a1 < 0 || a2 < 0) return black;
        alpha = (uint8)((a1 << 4) | a2);
    }
    NuColor color = {
        (uint8)((r1 << 4) | r2),
        (uint8)((g1 << 4) | g2),
        (uint8)((b1 << 4) | b2),
        alpha
    };
    return color;
}
