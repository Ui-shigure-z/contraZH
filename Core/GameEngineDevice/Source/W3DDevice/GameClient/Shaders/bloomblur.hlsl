// Summing in float keeps each tap off the target's 8 bits; a tap is (u offset, v offset, weight, unused)

sampler2D Source : register(s0);
float4 Taps[5] : register(c0);

float4 main(float2 uv : TEXCOORD0) : COLOR
{
    float4 sum = 0.0f;
    for (int i = 0; i < 5; ++i)
    {
        sum += tex2D(Source, uv + Taps[i].xy) * Taps[i].z;
    }
    return sum;
}
