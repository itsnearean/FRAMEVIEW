// fallback vertex shader - provides basic functionality when other shaders fail
// this shader should always work and provide a safe fallback

#include "../include/types.hlsli"

vs_output main(vs_input input)
{
    vs_output output;
    
    // basic transformation: just pass through position and color
    output.position = float4(input.position.xy, 0.0f, 1.0f);
    output.color = input.color;
    output.uv = input.uv;
    
    return output;
}
