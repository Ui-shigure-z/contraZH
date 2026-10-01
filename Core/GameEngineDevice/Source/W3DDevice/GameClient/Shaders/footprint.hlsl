// Draws an object's collision footprint on the ground as a glowing outline. The shape is a rounded box,
// which is a circle when the corner radius equals the half size.
//
// TEXCOORD0 spans the decal from 0 to 1 along the object's axes. COLOR0 carries the tint and opacity.

float4 Extent : register(c0);   // xy = decal size, zw = footprint half size inside its rounded corners
float4 Shape  : register(c1);   // x = corner radius plus the snap-in offset, y = line half width, z = 1 / inner glow reach, w = 1 / outer glow reach
float4 Motion : register(c2);   // xy = sweep direction, z = flash from being selected, w = glow strength

struct PsIn
{
    float4 Tint : COLOR0;
    float2 UV   : TEXCOORD0;
};

float4 main(PsIn input) : COLOR
{
    // The signed distance to the outline, negative inside the footprint.
    float2 position = (input.UV - 0.5f) * Extent.xy;
    float2 corner = abs(position) - Extent.zw;
    float edge = length(max(corner, 0.0f)) + min(max(corner.x, corner.y), 0.0f) - Shape.x;

    // The line keeps a pixel and a half of width when the camera pulls back.
    float pixel = max(fwidth(edge), 0.0001f);
    float halfWidth = max(Shape.y, pixel * 0.75f);
    float outline = saturate((halfWidth - abs(edge)) / pixel + 0.5f);

    float glow = (edge < 0.0f) ? exp(edge * Shape.z) : exp(-edge * Shape.w);

    // Two highlights travel around the outline, opposite each other.
    float along = dot(normalize(position + 0.0001f), Motion.xy);
    float hot = saturate(pow(abs(along), 12.0f) + Motion.z);

    float alpha = saturate(outline * (0.65f + 0.35f * hot) + glow * Motion.w * (1.0f + hot));
    float3 color = lerp(input.Tint.rgb, 1.0f, outline * hot * 0.6f);
    return float4(color, alpha * input.Tint.a);
}
