#version 450

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec2 vScreenPos;
layout(location = 2) in vec4 vScissor;
layout(location = 3) flat in uint vFilterMode;
layout(location = 4) flat in uint vRepeatMode;
layout(location = 5) flat in uint vTextureIndex;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 0) uniform sampler2D uTexturesLinear[8];
layout(set = 0, binding = 1) uniform sampler2D uTexturesNearest[8];

void main()
{
    if (vScreenPos.x < vScissor.z ||
        vScreenPos.y < vScissor.x ||
        vScreenPos.x > vScissor.w ||
        vScreenPos.y > vScissor.y)
    {
        discard;
    }

    vec2 uv = vUV;
    if ((vRepeatMode & 1u) != 0u) uv.x = fract(uv.x);
    if ((vRepeatMode & 2u) != 0u) uv.y = fract(uv.y);

    if (vFilterMode == 0u) FragColor = texture(uTexturesLinear[vTextureIndex], uv);
    else FragColor = texture(uTexturesNearest[vTextureIndex], uv);
}
