#version 450

layout(location = 0) in vec2 aQuad;
layout(location = 1) in vec3 iPos;
layout(location = 2) in vec2 iSize;
layout(location = 3) in vec4 iUV;
layout(location = 4) in uint iClipTopBottom;   // low 16 = top,  high 16 = bottom
layout(location = 5) in uint iClipLeftRight;   // low 16 = left, high 16 = right
layout(location = 6) in uint iClipRadii;       // bits 0-7=TL, 8-15=TR, 16-23=BL, 24-31=BR
layout(location = 7) in uint iModes;           // bits 0-7=textureIndex, 8-15=filter, 16-23=repeat

layout(location = 0) out vec2 vUV;
layout(location = 1) out vec2 vScreenPos;
layout(location = 2) flat out vec4 vClipRect;   // x=left, y=top, z=right, w=bottom
layout(location = 3) flat out vec4 vClipRadii;  // x=TL, y=TR, z=BL, w=BR
layout(location = 4) flat out uint vTextureIndex;
layout(location = 5) flat out uint vFilterMode;
layout(location = 6) flat out uint vRepeatMode;

layout(push_constant) uniform PushConstants {
    float uScreenWidth;
    float uScreenHeight;
    float uOffsetX;
    float uOffsetY;
} pc;

void main()
{
    vec2 worldPos = (iPos.xy + vec2(pc.uOffsetX, pc.uOffsetY)) + aQuad * iSize;
    float ndc_x = (worldPos.x / pc.uScreenWidth) * 2.0 - 1.0;
    float ndc_y = (worldPos.y / pc.uScreenHeight) * 2.0 - 1.0;

    float z_gl = iPos.z * 0.015625;
    float z_vk = z_gl * 0.5 + 0.5;
    gl_Position = vec4(ndc_x, ndc_y, z_vk, 1.0);

    vScreenPos = worldPos;
    vUV = vec2(
        mix(iUV.x, iUV.z, aQuad.x),
        mix(iUV.y, iUV.w, aQuad.y)
    );

    // Unpack clip data (little-endian)
    float cTop    = float( iClipTopBottom        & 0xFFFFu);
    float cBottom = float((iClipTopBottom >> 16) & 0xFFFFu);
    float cLeft   = float( iClipLeftRight        & 0xFFFFu);
    float cRight  = float((iClipLeftRight >> 16) & 0xFFFFu);
    vClipRect = vec4(cLeft, cTop, cRight, cBottom);

    vClipRadii = vec4(
        float( iClipRadii        & 0xFFu),
        float((iClipRadii >>  8) & 0xFFu),
        float((iClipRadii >> 16) & 0xFFu),
        float((iClipRadii >> 24) & 0xFFu)
    );

    // Unpack modes: textureIndex / filter / repeat from the three low bytes
    vTextureIndex =  iModes        & 0xFFu;
    vFilterMode   = (iModes >>  8) & 0xFFu;
    vRepeatMode   = (iModes >> 16) & 0xFFu;
}
