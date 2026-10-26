#version 450

// Vertex input attributes (matching the same layout locations as the GL version)
layout(location = 0) in vec2 aQuad;
layout(location = 1) in vec3 iPos;
layout(location = 2) in vec2 iSize;
layout(location = 3) in uint iBgColor;
layout(location = 4) in uint iBorderColor;
layout(location = 5) in vec4 iRadius;
layout(location = 6) in vec4 iBorder;
layout(location = 7) in vec4 iScissor;

layout(location = 0) out vec2 vLocalPos;
layout(location = 1) out vec2 vScreenPos;
layout(location = 2) out vec2 vSize;
layout(location = 3) out vec4 vRadius;
layout(location = 4) out vec4 vBorder;
layout(location = 5) out vec4 vScissor;
layout(location = 6) out vec4 vBgColor;
layout(location = 7) out vec4 vBorderColor;

layout(push_constant) uniform PushConstants {
    float uScreenWidth;
    float uScreenHeight;
} pc;

vec4 unpackRGBA(uint c)
{
    return vec4(
        float((c >> 0) & 255u) / 255.0,
        float((c >> 8) & 255u) / 255.0,
        float((c >> 16) & 255u) / 255.0,
        float((c >> 24) & 255u) / 255.0
    );
}

void main()
{
    vScreenPos = iPos.xy + aQuad * iSize;
    float ndc_x = (vScreenPos.x / pc.uScreenWidth) * 2.0 - 1.0;
    float ndc_y = 1.0 - (vScreenPos.y / pc.uScreenHeight) * 2.0;

    // Vulkan NDC depth range is [0, 1] not [-1, 1].
    // GL version wrote iPos.z * 0.015625 into clip.z with w=1, which lands in
    // [-1, 1]. Remap to [0, 1] so it matches:  z_vk = (z_gl + 1) * 0.5
    // If you previously relied on z == 0 -> middle of the depth range, this maps
    // it to 0.5 which is the same physical depth. Adjust if you want a fixed depth.
    float z_gl = iPos.z * 0.015625;
    float z_vk = z_gl * 0.5 + 0.5;

    // Y-flip: if you render to a Vulkan target that shares GL's convention
    // (top-left origin), keep the ndc_y as written above. If your render target
    // was authored for GL and you're now presenting in Vulkan, you may want to
    // negate ndc_y. Keep consistent with your other Vulkan pipelines.
    gl_Position = vec4(ndc_x, ndc_y, z_vk, 1.0);

    vLocalPos    = aQuad * iSize;
    vSize        = iSize;
    vRadius      = iRadius;
    vBorder      = iBorder;
    vScissor     = iScissor;
    vBgColor     = unpackRGBA(iBgColor);
    vBorderColor = unpackRGBA(iBorderColor);
}
