// The sun's glint on the ground, where it mirrors the sun towards the camera.
//
// The normal is the one the vertex lighting takes, from the blue and alpha of the water's height
// texture. Its texels sit on the terrain's vertices, so filtering spreads it across a cell as smoothly
// as the vertex lighting. A facet's own normal would show every triangle in the highlight.
//
// The including shader declares ToSun and SunColor and has the world position. GlintAlbedo all 0 turns
// the glint off. The literals are ones the heaviest terrain shaders already hold.

sampler2D GlintNormals : register(s11);

float4 GlintEye    : register(c23);   // camera position in world space, w = gloss, or for terrain the materials' gloss scale
float4 GlintAlbedo : register(c24);   // weighs the albedo's rgb and 1 into the glint's strength
float4 GlintMap    : register(c25);   // world xy to normal texcoords: xy scale, zw offset

float3 GlintNormal(float3 world)
{
    float2 xy = tex2D(GlintNormals, world.xy * GlintMap.xy + GlintMap.zw).ba * 2.0f - 1.0f;
    return float3(xy, sqrt(saturate(1.0f - dot(xy, xy))));
}

// The share of the sun's colour to add, lost before the edge of surfaces turning from the sun. The gloss must be at least 1, since pow(0, 0) is NaN.
float Glint(float3 world, float3 normal, float strength, float gloss)
{
    float3 halfway = normalize(normalize(GlintEye.xyz - world) + ToSun.xyz);
    return pow(saturate(dot(normal, halfway)), gloss) * saturate(dot(normal, ToSun.xyz) * gloss) * strength;
}
