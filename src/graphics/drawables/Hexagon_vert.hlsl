#include "SceneTransform_inc.hlsl"

#include "Hexagon_inc.hlsl"

static const float PHI = 0.5 * (1.0 + sqrt(5.0));

static const float3 icosahedron_verts[12] = {
    float3(0, +1, +PHI),
    float3(0, +1, -PHI),
    float3(0, -1, +PHI),
    float3(0, -1, -PHI),
    float3(+1, +PHI, 0),
    float3(+1, -PHI, 0),
    float3(-1, +PHI, 0),
    float3(-1, -PHI, 0),
    float3(+PHI, 0, +1),
    float3(-PHI, 0, +1),
    float3(+PHI, 0, -1),
    float3(-PHI, 0, -1)
};

static const uint3 icosahedron_indices[20] = {
    uint3(0, 2, 8),
    uint3(0, 8, 4),
    uint3(0, 4, 6),
    uint3(0, 6, 9),
    uint3(0, 9, 2),

    uint3(3, 10, 5),
    uint3(3, 5, 7),
    uint3(3, 7, 11),
    uint3(3, 11, 1),
    uint3(3, 1, 10),

    uint3(2, 5, 8),
    uint3(8, 5, 10),
    uint3(8, 10, 4),
    uint3(4, 10, 1),
    uint3(4, 1, 6),
    
    uint3(6, 1, 11),
    uint3(6, 11, 9),
    uint3(9, 11, 7),
    uint3(7, 9, 2),
    uint3(7, 2, 5)
};


struct VertexShaderOutput
{
    float4 Coords : TEXCOORD;
    float4 Position : SV_Position;
};

VertexShaderOutput main_hex_ico(uint ivid : SV_VertexID)
{
    VertexShaderOutput OUT;

    const int num_pents = 12;
    const int num_ivid_for_pents = num_pents * 3 * 5;

    int is_summit = ivid < num_ivid_for_pents;


    int num_tris = is_summit ? 3 : 6; // Only 3 summit triangles per face, 6 triangles per hexagons ottherwise
    uint vid = ivid % (3 * num_tris);
    uint tvid = vid % 3;
    uint tid = vid / 3;

    uint instance = (ivid - (1 - is_summit) * num_ivid_for_pents) / (3 * num_tris);

        const int num_faces = 20;
    int face_id = instance % num_faces;

    uint hex_vertIdx = (tvid ? (tvid + tid - 1) % 6 : 6);
    float3 position = float3(HEX_VERTS[hex_vertIdx], 0.0);
    float4 coords = float4(position.xy, face_id, 20);
 
    int3 h = 0;

    int i = instance / num_faces;
    if ((i > 0)) //  && (i < 19))
    {
        uint2 pol = hex_spiral_polar(i);
        h = hex_add_polar(h, pol);
        coords.zw = float2(pol);
        position.xy += mul(HEX_TO_2D, float3(h));
    }    
    position.xy *= 2.0 / 3.0 / 2.0;
    position = position.xzy;

   // position *= 0.5;
    
    
    //position.y = face_id * 2.0;
    
    
    // Build face base to transform hex
    uint3 faceIdxs = icosahedron_indices[face_id];
    float3 faceOri = (icosahedron_verts[faceIdxs.x] + icosahedron_verts[faceIdxs.y] + icosahedron_verts[faceIdxs.z]) /3.0;
  //  faceOri = icosahedron_verts[face_id];
    
    float3 faceNor = normalize(faceOri);
    float3 faceUp = normalize(icosahedron_verts[faceIdxs.x] - faceOri);
    float3 faceTan = normalize(icosahedron_verts[faceIdxs.z] - icosahedron_verts[faceIdxs.y]);
    
    position = faceOri * 1.0 + faceTan * position.z + faceUp * position.x;
    
    if (is_summit) {
        position = icosahedron_verts[faceIdxs[tid]];
        float3 delta = icosahedron_verts[faceIdxs[(tid + tvid) % 3]] - icosahedron_verts[faceIdxs[tid]];
        position += delta  / 6.0;
    }

    position = normalize(position);
    float3 color = float3(1.0, 1.0, 1.0);
    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    OUT.Position = clipPos;
    OUT.Coords = coords;
 
    return OUT;
}


VertexShaderOutput main_hex(uint ivid : SV_VertexID)
{
    VertexShaderOutput OUT;

    uint hid = ivid / HEX_NUM_INDICES;
    uint hvid = ivid - hid * HEX_NUM_INDICES;
    uint tid = 0;
    uint tvid = 0;
    uint hex_vid = hex_index_to_vertex(hvid, tid, tvid);
    
    float3 position = float3(HEX_VERTS[hex_vid], 0.0);
    float4 coords = float4(position.xy, hid, 30);

    int3 h = 0;

    int i = hid;
    if ((i > 0))
    {
        uint2 pol = hex_spiral_polar(i);
        h = hex_add_polar(h, pol);
        coords.zw = float2(pol);
        position.xy += mul(HEX_TO_2D, float3(h));
    }
  //  position.xy *= 2.0 / 3.0 / 2.0;
    position = position.xzy;



    float3 color = float3(1.0, 1.0, 1.0);
    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    OUT.Position = clipPos;
    OUT.Coords = coords;

    return OUT;
}



struct FUllScreenViewportOutput
{
    float4 Coords : TEXCOORD;
    float4 Position : SV_Position;
};

FUllScreenViewportOutput main_fsv(uint ivid : SV_VertexID)
{
    FUllScreenViewportOutput OUT;

    const int num_tris = 1;
    uint vid = ivid % (3 * num_tris);
    uint instance = ivid / (3 * num_tris);
    uint tvid = vid % 3;
    uint tid = vid / 3;

    float3 position = float3(0.0, 0.0, 0.0);
    float3 color = float3(1.0, 1.0, 1.0);

    position.xz = (float2(-1, -1) + float2(((tvid == 1) ? 2.0 : 0.0), ((tvid == 2) ? 2.0 : 0.0))) * 100;
    position.y = 0;
    float4 coords = float4(position, 1);

    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    OUT.Position = clipPos;
    OUT.Coords = coords;
    return OUT;
}
