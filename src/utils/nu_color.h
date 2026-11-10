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
