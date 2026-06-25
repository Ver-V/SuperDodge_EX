#include "struct.hlsli"


PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    

    output.pos = mul(float4(input.pos, 1.0f), matWorld);
    output.color = input.color;
    output.color.a = 0.5f;
    return output;
}