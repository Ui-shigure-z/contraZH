// Builds one level of a texture's slope map, which the derived bumps read in place of sampling
// the texture's brightness around every pixel.
//
// W3DSlopeMap draws each level at half the size of the texture's matching mip, so the quad's
// own derivatives make the reads come from one mip blurrier than the level, and fine texture
// noise does not sparkle. Red holds the brightness change along u and green along v, softened,
// in units of one texel of the slope map's top level, scaled and centred on a half.

sampler2D Source : register(s0);

float4 Texel  : register(c0);   // xy = one texel of the level drawn, in uv; zw = the level's size over the top level's
float4 Encode : register(c1);   // x = how hard large jumps soften, y = the scale onto the stored range

float Height(float2 uv)
{
    return dot(tex2D(Source, uv).rgb, float3(0.3f, 0.59f, 0.11f));
}

float4 main(float2 uv : TEXCOORD0) : COLOR
{
    float2 change = float2(
        Height(uv + float2(Texel.x, 0.0f)) - Height(uv - float2(Texel.x, 0.0f)),
        Height(uv + float2(0.0f, Texel.y)) - Height(uv - float2(0.0f, Texel.y))) * 0.5f;

    // Paint lines and team colour edges jump far more than surface grain, so large jumps are softened.
    change /= 1.0f + abs(change) * Encode.x;
    return float4(change * Texel.zw * Encode.y + 0.5f, 0.0f, 1.0f);
}
