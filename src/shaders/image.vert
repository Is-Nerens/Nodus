#version 450

layout(location = 0) in vec2 aQuad;
layout(location = 1) in vec3 iPos;
layout(location = 2) in vec2 iSize;
layout(location = 4) in vec4 iUV;
layout(location = 5) in vec4 iScissor;
layout(location = 6) in uint iFilterMode;
layout(location = 7) in uint iRepeatMode;
layout(location = 8) in uint iTextureIndex;

layout(location = 0) out vec2 vUV;
layout(location = 1) out vec2 vScreenPos;
layout(location = 2) out vec4 vScissor;
layout(location = 3) flat out uint vFilterMode;
layout(location = 4) flat out uint vRepeatMode;
layout(location = 5) flat out uint vTextureIndex;

layout(push_constant) uniform PushConstants {
    float uScreenWidth;
    float uScreenHeight;
} pc;

void main()
{
    vec2 worldPos = iPos.xy + aQuad * iSize;
    float ndc_x = (worldPos.x / pc.uScreenWidth) * 2.0 - 1.0;
    float ndc_y = 1.0 - (worldPos.y / pc.uScreenHeight) * 2.0;

    float z_gl = iPos.z * 0.015625;
    float z_vk = z_gl * 0.5 + 0.5;
    gl_Position = vec4(ndc_x, ndc_y, z_vk, 1.0);

    vScreenPos = worldPos;
    vScissor   = iScissor;
    vUV = vec2(
        mix(iUV.x, iUV.z, aQuad.x),
        mix(iUV.y, iUV.w, aQuad.y)
    );
    vFilterMode = iFilterMode;
    vRepeatMode = iRepeatMode;
    vTextureIndex = iTextureIndex;
}
