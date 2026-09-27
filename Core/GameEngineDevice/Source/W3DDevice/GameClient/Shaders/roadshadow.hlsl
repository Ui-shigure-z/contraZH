// Road passes that also receive the sun's cast shadow, or add dynamic point lights, or both.
//
// Every legacy road mode is the same product, the road texture times each cloud or
// light map times the vertex diffuse, on colour and alpha alike. roadnoise2.nvp
// does it for both maps and the fixed-function two-stage path does it for fewer.
// Roads blend onto terrain by their alpha, so the shadow scales colour only.
//
// NOISE_COUNT (0-2) is how many maps apply and PACKED picks the depth format. With
// two, the second is W3DGroundNoise's texture, read through groundnoise.hlsli, and the
// first is the cloud map or white.
// SHADOWED (default 1) picks whether the shadow map is read at all. The shadow map
// sits on the first stage after the maps, because fixed-function vertex processing
// hands out texcoord sets in stage order.
//
// Blend tiles of three textures draw through these too, laid over the terrain by their alpha,
// which heightblend.hlsli shapes by the top texture's height against a middling one below.
// Roads set its constants to leave their alpha alone.
//
// LIGHTS adds the point lights to the vertex lighting. They need the world position,
// on the stage after the shadow map, and take ps_2_a for their length.
//
// GLINT adds the sun's glint as the terrain has it. The lit builds have it, and GLINT alone
// gives the world position to roads without lights.

#ifndef SHADOWED
#define SHADOWED 1
#endif

#ifndef LIGHTS
#define LIGHTS 0
#endif

#ifndef GLINT
#define GLINT LIGHTS
#endif

#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

#define SHADOW_STAGE (1 + NOISE_COUNT)

#define HEIGHT_BLEND_REGISTER c1
#include "heightblend.hlsli"

sampler2D RoadTexture : register(s0);

#if NOISE_COUNT >= 1
sampler2D Noise1Texture : register(s1);
#endif
#if NOISE_COUNT >= 2
sampler2D Noise2Texture : register(s2);
#endif

#if NOISE_COUNT >= 2
#include "groundnoise.hlsli"
#endif

#if SHADOWED

#if SHADOW_STAGE == 1
sampler2D ShadowMap : register(s1);
#define SHADOW_TEXCOORD TEXCOORD1
#elif SHADOW_STAGE == 2
sampler2D ShadowMap : register(s2);
#define SHADOW_TEXCOORD TEXCOORD2
#else
sampler2D ShadowMap : register(s3);
#define SHADOW_TEXCOORD TEXCOORD3
#endif

#include "shadowreceive.hlsli"

#endif

#if LIGHTS || GLINT

// Stage numbers, spelled out because register names need a literal digit.
#if NOISE_COUNT + SHADOWED == 0
#define POSITION_INDEX 1
#elif NOISE_COUNT + SHADOWED == 1
#define POSITION_INDEX 2
#elif NOISE_COUNT + SHADOWED == 2
#define POSITION_INDEX 3
#else
#define POSITION_INDEX 4
#endif

#endif

#if GLINT
float4 ToSun    : register(c2);   // world space
float4 SunColor : register(c3);   // the sun's diffuse colour in the vertex lighting
#include "terrainglint.hlsli"
#endif

#if LIGHTS

// Eight fill c5 to c22 before the glint's c23 to c25, and fxc needs the rest for literals, so W3DShaderManager::MAX_PIXEL_LIGHTS must match.
#define POINT_LIGHT_REGISTER c5
#define POINT_LIGHT_COUNT 8
#include "pointlights.hlsli"

#endif

struct PsIn
{
    float4 Diffuse   : COLOR0;
    float2 RoadUV    : TEXCOORD0;
#if NOISE_COUNT >= 1
    float2 Noise1UV  : TEXCOORD1;
#endif
#if NOISE_COUNT >= 2
    float2 Noise2UV  : TEXCOORD2;
#endif
#if SHADOWED
    float4 ShadowPos : SHADOW_TEXCOORD;
#endif
#if LIGHTS || GLINT
    float3 WorldPos  : CONCAT(TEXCOORD, POSITION_INDEX);
#endif
};

float4 main(PsIn input) : COLOR
{
    float4 color = tex2D(RoadTexture, input.RoadUV);
    float weight = HeightBlendWeight(input.Diffuse.a, 0.5f, tex2D(HeightAtlas, input.RoadUV).r);
#if GLINT
    float glintStrength = dot(color.rgb, GlintAlbedo.xyz) + GlintAlbedo.w;
#endif

#if LIGHTS
    // Roads lie on the terrain, so their facet's normal is turned up.
    float3 facet = cross(ddx(input.WorldPos), ddy(input.WorldPos));
    facet *= (facet.z < 0.0f) ? -1.0f : 1.0f;
    float3 normal = facet * rsqrt(max(dot(facet, facet), 1e-30f));

    color.rgb *= saturate(input.Diffuse.rgb + PointLighting(input.WorldPos, normal));
#else
    color.rgb *= input.Diffuse.rgb;
#endif
    color.a *= weight;

#if NOISE_COUNT >= 1
    float4 cloud = tex2D(Noise1Texture, input.Noise1UV);
    color *= cloud;
#endif
#if NOISE_COUNT >= 2
    color.rgb *= GroundNoise(Noise2Texture, input.Noise2UV);
#endif

#if SHADOWED
    float lit = ShadowLit(input.ShadowPos);
    color.rgb *= lerp(ShadowColor.rgb, float3(1.0f, 1.0f, 1.0f), lit);
#else
    float lit = 1.0f;
#endif

#if GLINT
    float glint = Glint(input.WorldPos, GlintNormal(input.WorldPos), glintStrength * lit);
#if NOISE_COUNT >= 1
    color.rgb += SunColor.rgb * cloud.rgb * glint;
#else
    color.rgb += SunColor.rgb * glint;
#endif
#endif
    return color;
}
