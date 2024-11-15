#include "Color_inc.hlsl"
#include "Paint_inc.hlsl"
#include "Shading_inc.hlsl"
#include "SceneModel_inc.hlsl"
#include "Surface_inc.hlsl"
#include "Sky_inc.hlsl"

float3 diffuse_brdf(float3 baseColor)
{
    return baseColor / M_PI;
}

float3 light_shading(float3 lightI, float3 l, float3 v, float3 n, float metallic, float roughness, float3 baseColor) {

    float3 shading = baseColor;
  //  baseColor = 1;
    // v; //u_Camera - v_Position);
    // l; //normalize(pointToLight); // Direction from surface point to light
    float3 h = normalize(l + v); // Direction of the vector between l and v, called halfway vector
    float NdotL = clamp(dot(n, l), 0.0, 1.0);
    float NdotV = clamp(dot(n, v), 0.0, 1.0);
    float NdotH = clamp(dot(n, h), 0.0, 1.0);
    float VdotH = clamp(dot(v, h), 0.0, 1.0);
    if (NdotL > 0.0 || NdotV > 0.0)
    {
        float3 intensity = lightI;

        float3 C_diff = lerp(baseColor, 0, metallic);
        float3 f0 = lerp(0.04, baseColor, metallic);
        float alphaRoughness = roughness * roughness;

        float3 fresnel = shading_fresnel(f0, VdotH);

        float3 f_diffuse = (1 - fresnel) * C_diff / M_PI;
        
        float V = V_GGX(NdotL, NdotV, alphaRoughness);
        float D = D_GGX(NdotH, alphaRoughness);
        float3 f_specular = fresnel * V * D;


        //float3 f_diffuse = intensity * NdotL * BRDF_lambertian(f0, f90, C_diff, specularWeight, VdotH);
       // float3 f_specular = intensity * NdotL * BRDF_specularGGX(f0, f90, alphaRoughness, specularWeight, VdotH, NdotL, NdotV, NdotH);
        shading = (f_diffuse + f_specular) * intensity * NdotL;
        // shading = f_diffuse;
        //    shading = f_diffuse;
         //   shading = roughness;)

    }

    //  shading = lightI;
   
    return shading;
}

// Drawgrid
float3 drawGrid(float2 uv, float3 color) {
    float tex_width, tex_height, tex_elements;
    material_textures.GetDimensions(tex_width, tex_height, tex_elements);

    float3 texcoord = float3(uv.x * tex_width, uv.y * tex_height, 1.0f);

    float3 grid = paintStripe(texcoord, tex_width * 0.1f, 1.5f);
    color = lerp(color, float3(1.0, 0.0, 0.0), grid.x);
    color = lerp(color, float3(0.0, 1.0, 0.0), grid.y);

    grid = paintStripe(texcoord, tex_width * 0.01f, 0.5f);
    color = lerp(color, float3(0.8, 0.8, 1.8), max(grid.x, grid.y));

    return color;
}

//
// Main
// 
bool LIGHT_SHADING(int drawMode) { return (drawMode & 0x00000080) != 0; }

int DISPLAYED_COLOR_BITS() { return 0x0000000F; }
int DISPLAYED_COLOR_OFFSET() { return 0; }
int DISPLAYED_COLOR(int drawMode) { return (drawMode & DISPLAYED_COLOR_BITS()) >> DISPLAYED_COLOR_OFFSET(); }

cbuffer UniformBlock1 : register(b1) {
    int _nodeID;
    int _partID;
    int _numNodes;
    int _numParts;
    int _numMaterials;
    int _drawMode;
}

struct PixelShaderInput{
    float3 EyePos : EPOS;
    float3 Normal   : NORMAL;
    float2 Texcoord : TEXCOORD;
    float4 Color : COLOR;
};

float4 main(PixelShaderInput IN) : SV_Target{
    int matIdx = part_array[_partID].material;// < 0.0 ? 0 : floor(IN.Material));
    Material m = material_array[matIdx];
    
    float4 vcolor = IN.Color;

    // Normal and Normal map
    float3 surfNormal = normalize(IN.Normal);
    float3 mapNor = (0.0);
    float3 normal = surfNormal;
    if (m.textures.y != -1) {
        mapNor = float3(material_textures.Sample(materialMapSampler(), float3(IN.Texcoord.xy, m.textures.y)).xyz);
        float3 mapN = (mapNor * 2.0 - 1.0);
        // assume surfNormal, the inter�po�lat�ed ver�tex nor�mal and 
        // V, the view vec�tor (ver�tex to eye)
        //   and then -V => eye to vertex
        float3 mV = -IN.EyePos.xyz;
        float3x3 tbn = computeTangentSpace(surfNormal, mV, IN.Texcoord.xy);
        normal = (mul(mapN, tbn));
        normal = normalize(normal);
        normal = normal.xyz; // i m puzzled by the fact that we don't need to swizzle the result here ? whatever it works
    }
   // normal = surfNormal;

    float4 rmaoMap = float4(1.0, 1.0, 0.0, 0.0);
    if (m.textures.z != -1) {
        rmaoMap = material_textures.Sample(materialMapSampler(), float3(IN.Texcoord.xy, m.textures.z)).gbra;
    }
    float roughness = m.roughness * rmaoMap.x;
    float metallic = m.metallic * rmaoMap.y;
    
    // with albedo from property or from texture
    float3 albedo = m.color.xyz;
    if (m.textures.x != -1) {
        albedo = material_textures.Sample(materialMapSampler(), float3(IN.Texcoord.xy, m.textures.x)).xyz;
    }

    //float3 baseColor = float3(0.7, 0.7, 0.7);
    float3 baseColor = 0.7;
    switch (DISPLAYED_COLOR(_drawMode)) {
    case 0: baseColor = vcolor.xyz; break;
    //case 0: baseColor = albedo; break;
    case 1: baseColor = normal; break;
    case 2: baseColor = surfNormal; break;
    case 3: baseColor = mapNor; break;
    case 4: baseColor = roughness; break;
    case 5: baseColor = metallic; break;
 //   case 5: ; break;
    }

   // baseColor = drawGrid(IN.Texcoord.xy, baseColor);
    float3 shading = baseColor;
    if (LIGHT_SHADING(_drawMode))
    {
        float3 lightD = getSunDir();
        float3 lightI = SkyColor(lightD);

        float3 n = normal;
        
        lightI = sky_evalIrradianceSH(n) * M_PI;
        lightD = n;
        
        float3 v = normalize(IN.EyePos.xyz); //u_Camera - v_Position);
        float3 l = lightD; //normalize(pointToLight); // Direction from surface point to light
        
        shading = light_shading(lightI, lightD, v, n, metallic, roughness, baseColor);
     
    }
    
    float3 emissiveColor = float3 (0.0, 0.0, 0.0);
    if (m.textures.w != -1) {
        emissiveColor = material_textures.Sample(materialMapSampler(), float3(IN.Texcoord.xy, m.textures.w)).xyz;
    }
 
    float3 color = shading + emissiveColor;
    return float4((color), 1.0);
}