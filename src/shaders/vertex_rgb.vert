#version 450

layout(location = 0) in vec3 iPos;
layout(location = 1) in float iColor;

layout(push_constant) uniform Push {
    float uScreenWidth;
    float uScreenHeight;
    float uOffsetX;
    float uOffsetY;
} pc;

layout(location = 0) out vec4 vColor;

vec4 unpackRGBA(float c)
{
    uint value = floatBitsToUint(c);
    return vec4(
        float((value >> 24) & 255u) / 255.0,
        float((value >> 16) & 255u) / 255.0,
        float((value >> 8)  & 255u) / 255.0,
        float((value >> 0)  & 255u) / 255.0
    );
}

void main() {
    float ndc_x = ((iPos.x + pc.uOffsetX) / pc.uScreenWidth) * 2.0 - 1.0;
    float ndc_y = ((iPos.y + pc.uOffsetY) / pc.uScreenHeight) * 2.0 - 1.0;

    gl_Position = vec4(ndc_x, ndc_y, iPos.z, 1.0);

    vColor = unpackRGBA(iColor);
}
