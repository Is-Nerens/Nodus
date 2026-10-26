#version 450

layout(location = 0) in vec2 vLocalPos;
layout(location = 1) in vec2 vScreenPos;
layout(location = 2) in vec2 vSize;
layout(location = 3) in vec4 vRadius;
layout(location = 4) in vec4 vBorder;
layout(location = 5) in vec4 vScissor;
layout(location = 6) in vec4 vBgColor;
layout(location = 7) in vec4 vBorderColor;

layout(location = 0) out vec4 FragColor;

float sdRoundRect(vec2 p, vec2 b, float r)
{
    vec2 q = abs(p) - b + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

void main()
{
    // ---- SCISSOR (world/screen space) ----
    if (vScreenPos.x < vScissor.z ||
        vScreenPos.y < vScissor.x ||
        vScreenPos.x > vScissor.w ||
        vScreenPos.y > vScissor.y)
    {
        discard;
    }

    // ---- SDF (local space, centered at 0) ----
    vec2 p = vLocalPos - vSize * 0.5;
    vec2 halfSize = vSize * 0.5;

    // ---- PER-CORNER OUTER RADIUS ----
    float outerR = p.x > 0.0 ? (p.y > 0.0 ? vRadius.w : vRadius.y)
                             : (p.y > 0.0 ? vRadius.z : vRadius.x);

    // ---- PER-CORNER INNER RADIUS (reduced by adjacent border thickness) ----
    float innerR = p.x > 0.0 ? (p.y > 0.0 ? max(vRadius.w - max(vBorder.y, vBorder.w), 0.0)
                                           : max(vRadius.y - max(vBorder.x, vBorder.w), 0.0))
                             : (p.y > 0.0 ? max(vRadius.z - max(vBorder.y, vBorder.z), 0.0)
                                          : max(vRadius.x - max(vBorder.x, vBorder.z), 0.0));

    // ---- OUTER SHAPE ----
    float outer = sdRoundRect(p, halfSize, outerR);

    // ---- INNER SHAPE (border inset) ----
    vec2 innerOffset = vec2(vBorder.z - vBorder.w, vBorder.x - vBorder.y) * 0.5;
    vec2 innerHalf = halfSize - vec2(vBorder.z + vBorder.w, vBorder.x + vBorder.y) * 0.5;
    innerHalf = max(innerHalf, vec2(0.0));
    float inner = sdRoundRect(p - innerOffset, innerHalf, innerR);

    // ---- MASKS ----
    float outerMask = smoothstep(0.5, -0.5, outer);
    float innerMask = (innerHalf.x > 0.0 && innerHalf.y > 0.0) ? smoothstep(0.5, -0.5, inner) : 1.0;

    float fillMask   = innerMask * vBgColor.a;
    float borderMask = clamp(outerMask - innerMask, 0.0, 1.0);

    // ---- COLOR ----
    vec3 color = (borderMask > fillMask) ? vBorderColor.rgb : vBgColor.rgb;
    float alpha = clamp(fillMask + borderMask, 0.0, 1.0);
    if (alpha <= 0.0) discard;
    FragColor = vec4(color, alpha);
}
