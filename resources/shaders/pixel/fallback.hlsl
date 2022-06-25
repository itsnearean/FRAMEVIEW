// fallback pixel shader - provides basic functionality when other shaders fail
// this shader should always work and provide a safe fallback

#include "../include/types.hlsli"

float4 main(ps_input input) : SV_Target
{
    // basic fallback: output vertex color with texture if available
    float4 color = input.color;
    
    // if texture is bound, sample it
    if (input.uv.x >= 0.0f && input.uv.y >= 0.0f) {
        float4 tex_color = texture0.Sample(sampler0, input.uv);
        color = color * tex_color;
    }
    
    return color;
}
