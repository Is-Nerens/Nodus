#version 450

layout(location = 0) in vec4 vColor;
layout(location = 1) in vec2 vSDF;

layout(location = 0) out vec4 FragColor;

void main()
{
    if (vSDF.x < -1e8)
    {
        FragColor = vColor;
        return;
    }

    float d;
    float aa;

    if (vSDF.y > 1e8)
    {
        // Line body: signed distance to the nearest long edge.
        d = vSDF.x;

        // Pixel-footprint-based antialiasing.
        aa = max(0.5 * fwidth(d), 0.0001);
    }
    else
    {
        // Round cap: normalized circle coordinates.
        if (vSDF.x < 0.0) discard;

        d = length(vSDF) - 1.0;

        // Derivative-based AA in normalized cap coordinates.
        aa = max(fwidth(d), 0.0001);
    }

    float coverage = 1.0 - smoothstep(-aa, aa, d);

    if (coverage <= 0.0) discard;

    FragColor = vec4(vColor.rgb, vColor.a * coverage);
}
