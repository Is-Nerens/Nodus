#version 450

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec2 vScreenPos;
layout(location = 2) in vec4 vColor;
layout(location = 3) flat in uint vTextureIndex;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 0) uniform sampler2D uFontTextures[8];

void main()
{
    float alpha = texture(uFontTextures[vTextureIndex], vUV).r;
    FragColor = vec4(vColor.rgb, vColor.a * alpha);
}
