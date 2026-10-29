#version 450

layout(location = 0) in vec2 aQuad;
layout(location = 1) in vec3 iPos;
layout(location = 2) in vec2 iSize;
layout(location = 3) in vec4 iUV;
layout(location = 4) in uint iColor;
layout(location = 5) in uint iTextureIndex;   // 0..7
layout(location = 6) in uint iClipTopBottom;
layout(location = 7) in uint iClipLeftRight;
layout(location = 8) in uint iClipRadii;

layout(location = 0) out vec2 vUV;
layout(location = 1) out vec2 vScreenPos;
layout(location = 2) out vec4 vColor;

layout(location = 4) flat out vec4 vClipRect;   // x=left, y=top, z=right, w=bottom
layout(location = 5) flat out vec4 vClipRadii;  // x=TL, y=TR, z=BL, w=BR

layout(push_constant) uniform PushConstants {
    float uScreenWidth;
    float uScreenHeight;
    float uOffsetX;
    float uOffsetY;
} pc;

vec4 unpackRGBA(uint c)
{
    return vec4(
        float((c >> 0)  & 255u) / 255.0,
        float((c >> 8)  & 255u) / 255.0,
        float((c >> 16) & 255u) / 255.0,
        float((c >> 24) & 255u) / 255.0
    );
}

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
    vColor        = unpackRGBA(iColor);

    // Unpack clip data (little-endian)
    float cTop    = float( iClipTopBottom        & 0xFFFFu);
    float cBottom = float((iClipTopBottom >> 16) & 0xFFFFu);
    float cLeft   = float( iClipLeftRight        & 0xFFFFu);
    float cRight  = float((iClipLeftRight >> 16) & 0xFFFFu);
    vClipRect = vec4(cLeft, cTop, cRight, cBottom);
    float rTL = float( iClipRadii        & 0xFFu);
    float rTR = float((iClipRadii >>  8) & 0xFFu);
    float rBL = float((iClipRadii >> 16) & 0xFFu);
    float rBR = float((iClipRadii >> 24) & 0xFFu);
    vClipRadii = vec4(rTL, rTR, rBL, rBR);
}
