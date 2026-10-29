#version 450

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec2 vScreenPos;
layout(location = 2) in vec4 vColor;

layout(location = 4) flat in vec4 vClipRect;   // left, top, right, bottom
layout(location = 5) flat in vec4 vClipRadii;  // TL, TR, BL, BR

layout(location = 0, index = 0) out vec4 FragColor;
layout(location = 0, index = 1) out vec4 FragColor1;

layout(set = 0, binding = 0) uniform sampler2D uFontTexture;

// Returns 1.0 if inside the (optionally rounded) clip rect, else 0.0.
float insideRoundedRect(vec2 p, vec4 rect, vec4 r)
{
    if (p.x < rect.x || p.x > rect.z || p.y < rect.y || p.y > rect.w) return 0.0;

    float dx = 0.0, dy = 0.0, rad = 0.0;

    if (p.x < rect.x + r.x && p.y < rect.y + r.x) {        // top-left
        dx = p.x - (rect.x + r.x);
        dy = p.y - (rect.y + r.x);
        rad = r.x;
    } else if (p.x > rect.z - r.y && p.y < rect.y + r.y) { // top-right
        dx = p.x - (rect.z - r.y);
        dy = p.y - (rect.y + r.y);
        rad = r.y;
    } else if (p.x < rect.x + r.z && p.y > rect.w - r.z) { // bottom-left
        dx = p.x - (rect.x + r.z);
        dy = p.y - (rect.w - r.z);
        rad = r.z;
    } else if (p.x > rect.z - r.w && p.y > rect.w - r.w) { // bottom-right
        dx = p.x - (rect.z - r.w);
        dy = p.y - (rect.w - r.w);
        rad = r.w;
    } else {
        return 1.0;
    }

    if (rad <= 0.0) return 1.0;
    return (dx*dx + dy*dy <= rad*rad) ? 1.0 : 0.0;
}

void main()
{
    if (vScreenPos.x < vClipRect.x || vScreenPos.x > vClipRect.z ||
        vScreenPos.y < vClipRect.y || vScreenPos.y > vClipRect.w) {
        discard;
    }

    if (insideRoundedRect(vScreenPos, vClipRect, vClipRadii) == 0.0) {
        discard;
    }

    vec3 lcd = texture(uFontTexture, vUV).rgb;
    FragColor  = vec4(vColor.rgb, 1.0);
    FragColor1 = vec4(lcd, 1.0);
}
