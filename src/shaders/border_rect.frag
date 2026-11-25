#version 450

layout(location = 0) in vec2 vLocalPos;
layout(location = 1) in vec2 vScreenPos;
layout(location = 2) in vec2 vSize;
layout(location = 3) in vec4 vRadius;
layout(location = 4) in vec4 vBorder;
layout(location = 5) flat in vec4 vClipRect;   // left, top, right, bottom
layout(location = 6) flat in vec4 vClipRadii;  // TL, TR, BL, BR
layout(location = 7) in vec4 vBgColor;
layout(location = 8) in vec4 vBorderColor;

layout(location = 0) out vec4 FragColor;

float sdRoundRect(vec2 p, vec2 b, float r)
{
    vec2 q = abs(p) - b + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

float insideClipping(vec2 p, vec4 rect, vec4 r)
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
    // Clipping
    if (insideClipping(vScreenPos, vClipRect, vClipRadii) == 0.0) {
        discard;
    }

    // SDF (local space, centered at 0)
    vec2 p = vLocalPos - vSize * 0.5;
    vec2 halfSize = vSize * 0.5;

    // Corder outer radii
    float outerR = p.x > 0.0 ? (p.y > 0.0 ? vRadius.w : vRadius.y)
                             : (p.y > 0.0 ? vRadius.z : vRadius.x);

    // Corder inner radii (reduced by adjacent border thickness)
    float innerR = p.x > 0.0 ? (p.y > 0.0 ? max(vRadius.w - max(vBorder.y, vBorder.w), 0.0)
                                           : max(vRadius.y - max(vBorder.x, vBorder.w), 0.0))
                             : (p.y > 0.0 ? max(vRadius.z - max(vBorder.y, vBorder.z), 0.0)
                                          : max(vRadius.x - max(vBorder.x, vBorder.z), 0.0));

    // Outer shape
    float outer = sdRoundRect(p, halfSize, outerR);

    // Inner shape (border inset)
    vec2 innerOffset = vec2(vBorder.z - vBorder.w, vBorder.x - vBorder.y) * 0.5;
    vec2 innerHalf = halfSize - vec2(vBorder.z + vBorder.w, vBorder.x + vBorder.y) * 0.5;
    innerHalf = max(innerHalf, vec2(0.0));
    float inner = sdRoundRect(p - innerOffset, innerHalf, innerR);

    // Masks
    float outerMask = smoothstep(0.5, -0.5, outer);
    float innerMask = (innerHalf.x > 0.0 && innerHalf.y > 0.0) ? smoothstep(0.5, -0.5, inner) : 1.0;
    float fillMask   = innerMask * vBgColor.a;
    float borderMask = clamp(outerMask - innerMask, 0.0, 1.0);

    // Final colour
    vec3 color = (borderMask > fillMask) ? vBorderColor.rgb : vBgColor.rgb;
    float alpha = clamp(fillMask + borderMask, 0.0, 1.0);
    if (alpha <= 0.0) discard;
    FragColor = vec4(color, alpha);
}
