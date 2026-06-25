






cbuffer ConstantBuffer : register(b0)  {
    matrix matWorld;
};


struct VS_INPUT
{
    float3 pos : POSITION;
    float4 color : COLOR;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};