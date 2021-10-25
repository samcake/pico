


struct PixelShaderInput
{
    float4 coords : TEXCOORD;
};


float4 main(PixelShaderInput IN) : SV_Target
{
   float maxCoord = max(IN.coords.x, IN.coords.y);
   if (maxCoord > 1.0) discard;


   return float4(IN.coords.xy, 0, 1);
}