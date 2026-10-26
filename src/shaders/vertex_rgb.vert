#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

layout(push_constant) uniform Push {
    float uScreenWidth;
    float uScreenHeight;
    float uOffsetX;
    float uOffsetY;
} pc;

layout(location = 0) out vec3 vColor;

void main() {
    float ndc_x = ((aPos.x + pc.uOffsetX) / pc.uScreenWidth) * 2.0 - 1.0;
    float ndc_y = 1.0 - ((aPos.y + pc.uOffsetY) / pc.uScreenHeight) * 2.0;
    gl_Position = vec4(ndc_x, ndc_y, aPos.z * 0.015625, 1.0);
    vColor = aColor;
}
