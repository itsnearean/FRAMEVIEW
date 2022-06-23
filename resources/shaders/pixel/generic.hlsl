#include "../include/types.hlsli"

float4 main(VS_OUTPUT IN) : SV_TARGET
{
    float4 tex = texture0.Sample(curtex, IN.texcoord0);
    float4 col = IN.color0;
    col.rgb *= tex.rgb;
    col.a *= tex.a;
    return col;
} 