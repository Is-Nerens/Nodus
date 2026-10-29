#version 450

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec4 vColor;

layout(location = 0, index = 0) out vec4 FragColor;
layout(location = 0, index = 1) out vec4 FragColor1;

layout(set = 0, binding = 0) uniform sampler2D uFontTexture;

void main()
{
    vec3 lcd = texture(uFontTexture, vUV).rgb;
    FragColor  = vec4(vColor.rgb, 1.0);
    FragColor1 = vec4(lcd, 1.0);
}
