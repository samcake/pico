/*struct ModelViewProjection
{
    matrix MVP;
};
*/
//ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

struct VertexPosColor
{
    float3 Position : POSITION;
};

struct VertexShaderOutput
{
    float4 Color    : COLOR;
    float4 Position : SV_Position;
};

VertexShaderOutput mainVertex(VertexPosColor IN)
{
    VertexShaderOutput OUT;

   // OUT.Position = mul(ModelViewProjectionCB.MVP, float4(IN.Position, 1.0f));
    OUT.Position = float4(IN.Position, 1.0f);
    OUT.Color = float4(1.0f, 0.0f, 0.0f, 1.0f);

    return OUT;
}
