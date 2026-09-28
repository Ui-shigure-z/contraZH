// The HQ sky's cloud shadows, drawn by W3DSkyClouds into a map over the ground around the camera.
//
// Every surface that reads the cloud map multiplies its colour by this map's rgb, and alpha stays 1
// because the legacy terrain shaders multiply alpha too.
//
// The noise tiles, with two cloud shapes in red and green and a warp vector in blue and alpha.
// Each read goes through its own turn, scale and drift, and the scales differ by irrational ratios,
// so the clouds never repeat. The layers drift at different speeds, so the clouds change shape too.

sampler2D Noise : register(s0);

float4 Window  : register(c0);   // x = world units across the map, y = world units the warp bends, z = 1 / edge width
float4 Warp    : register(c1);   // each read's noise texcoords per world unit: xy along world x, zw along world y
float4 ShapeA  : register(c2);
float4 ShapeB  : register(c3);
float4 Detail  : register(c4);
float4 DriftWA : register(c5);   // each read's offset: xy for the warp, zw for shape A
float4 DriftBD : register(c6);   // xy for shape B, zw for the detail
float4 Weights : register(c7);   // shape A, shape B, detail, and the offset that puts the coverage threshold at 0
float4 Shade   : register(c8);   // rgb = the ground's colour under a thick cloud

// A mul and a mad per read, since dot products would cost more.
float2 NoiseUV(float2 p, float4 axes, float2 drift)
{
    return p.x * axes.xy + (p.y * axes.zw + drift);
}

float4 main(float2 uv : TEXCOORD0) : COLOR
{
    float2 p = uv * Window.x;
    p += (tex2D(Noise, NoiseUV(p, Warp, DriftWA.xy)).ba - 0.5f) * Window.y;

    float a = tex2D(Noise, NoiseUV(p, ShapeA, DriftWA.zw)).r;
    float b = tex2D(Noise, NoiseUV(p, ShapeB, DriftBD.xy)).g;
    float d = tex2D(Noise, NoiseUV(p, Detail, DriftBD.zw)).r;

    // Standard deviations above the coverage threshold. Thick middles shade fully, thin edges less.
    float depth = a * Weights.x + b * Weights.y + d * Weights.z + Weights.w;
    float edge = saturate(depth * Window.z + 0.5f);
    float cover = edge * edge * (3.0f - 2.0f * edge) * saturate(0.75f + 0.25f * depth);
    return float4(lerp(float3(1.0f, 1.0f, 1.0f), Shade.rgb, cover), 1.0f);
}
