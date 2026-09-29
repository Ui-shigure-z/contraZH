// Lights the ground near a laser beam, as a light shaped like the beam itself. It draws on a mesh that
// follows the terrain and blends as dest * (1 + light), which lights the ground's own colour a second
// time. The light arrives divided by the scene's light, so a dim night lights up as much as a bright day.
//
// BEAMS lights the ground with that many overlapping beams in one pass. Each channel takes the root of
// the sum of the beams' squared light, so a lone beam lights as before and overlaps never compound.
//
// COLOR0 carries the terrain normal's x and y, TEXCOORD0 the world x and y, and TEXCOORD1.x the world z.

sampler2D NoiseTexture : register(s0);

#if defined(BEAMS)
float4 Shape     : register(c0);   // x = falloff power, y = light on ground facing away, from 0 to 1
float4 Beams[BEAMS * 4] : register(c1);   // per beam, the four registers the single beam reads from c0
#else
float4 BeamStart : register(c0);   // xyz = beam start, w = its along coordinate in noise tiles
float4 BeamSpan  : register(c1);   // xyz = start to end, w = 1 / length squared
float4 Glow      : register(c2);   // rgb = light over the scene's light, w = 1 / reach
float4 Pulse     : register(c3);   // x = pulse travel so far, y = beam length in noise tiles, z = pulse swing
float4 Shape     : register(c4);   // x = falloff power, y = light on ground facing away, from 0 to 1
#endif

struct PsIn
{
    float4 Normal  : COLOR0;
    float2 WorldXY : TEXCOORD0;
    float2 WorldZ  : TEXCOORD1;
};

float3 BeamLight(float3 world, float3 normal, float4 beamStart, float4 beamSpan, float4 glow, float4 pulses)
{
    // The nearest point on the beam lights this spot, so a beam high in the air lights little.
    float t = saturate(dot(world - beamStart.xyz, beamSpan.xyz) * beamSpan.w);
    float3 toBeam = beamStart.xyz + beamSpan.xyz * t - world;
    float distance = length(toBeam);
    float falloff = pow(saturate(1.0f - distance * glow.w), Shape.x);

    // Wrapped, so ground facing away from the beam keeps some light, and slopes facing it light most.
    float facing = lerp(saturate(dot(normal, toBeam) / max(distance, 0.001f)), 1.0f, Shape.y);

    // The laser shader's pulses light the ground as they pass.
    float along = beamStart.w + t * pulses.y;
    float4 noiseA = tex2D(NoiseTexture, float2(along - pulses.x, 0.25f));
    float4 noiseB = tex2D(NoiseTexture, float2(along * 2.3f - pulses.x * 1.5f, 0.75f));
    float pulse = 1.0f + pulses.z * (noiseA.r + noiseB.g - 1.0f);

    return glow.rgb * (falloff * facing * pulse);
}

float4 main(PsIn input) : COLOR
{
    float3 world = float3(input.WorldXY, input.WorldZ.x);
    float2 slope = input.Normal.xy * 2.0f - 1.0f;
    float3 normal = float3(slope, sqrt(saturate(1.0f - dot(slope, slope))));

#if defined(BEAMS)
    float3 sum = 0.0f;
    [unroll] for (int i = 0; i < BEAMS; i++)
    {
        float3 light = BeamLight(world, normal, Beams[i * 4], Beams[i * 4 + 1], Beams[i * 4 + 2], Beams[i * 4 + 3]);
        sum += light * light;
    }
    return float4(sqrt(sum), 1.0f);
#else
    return float4(BeamLight(world, normal, BeamStart, BeamSpan, Glow, Pulse), 1.0f);
#endif
}
