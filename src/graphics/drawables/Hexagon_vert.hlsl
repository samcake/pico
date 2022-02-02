#include "SceneTransform_inc.hlsl"

static const float SQRT_3 = sqrt(3.0);
static const float HALF_SQRT_3 = SQRT_3 * 0.5;

static const float2 HEX_VERTS[7] = {
    float2(0, 1),
    float2(-HALF_SQRT_3, 0.5),
    float2(-HALF_SQRT_3, -0.5),
    float2(0,-1),
    float2(HALF_SQRT_3, -0.5),
    float2(HALF_SQRT_3, 0.5),
    float2(0, 0),
};

static const float2x3 HEX_TO_2D = float2x3(
     HALF_SQRT_3, 0,    -HALF_SQRT_3,
     -0.5,        1,    -0.5
);

static const int3 hex_direction_vectors[6] = {
    int3(+1, 0, -1), int3( 0, +1, -1), int3(-1, +1, 0),
    int3(-1, 0, +1), int3( 0, -1, +1), int3(+1, -1, 0)
};

int3 hex_dir(uint dir, int ring = 1) {
    return hex_direction_vectors[uint(dir) % 6] * ring;
}

int3 hex_add(int3 a, int3 b) {
    return a + b;
}

int3 hex_neighbor(int3 h, int dir) {
    return h + hex_dir(dir);
}

uint2 hex_spiral_polar(uint i) {
    i -= 1; // forget about ring 0
    // compute the ring
    uint r = uint(floor(0.5 * (1.0 + sqrt(1.0 + float(i) * 4.0 / 3.0))));
    // sum at ring r aka index of the first index in the ring
    uint r0 = (r * (r - 1) / 2) * 6;
    // index in the ring
    uint ri = (i - r0);
    
    return int2(r, ri);
}

uint3 hex_add_polar(int3 h, uint2 pol) {
    // find which side / direction
    uint d = pol.y / pol.x;
    // and index on the ring side
    uint rsi = pol.y % pol.x;

    h = hex_dir(d, pol.x);
    h += hex_dir(d + 2, rsi);
    return h;
}

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

VertexShaderOutput main_hex(uint ivid : SV_VertexID)
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
        position += delta * 1.0 / 6.0;
    }

    position = normalize(position);
    float3 color = float3(1.0, 1.0, 1.0);
    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    OUT.Position = clipPos;
    OUT.Coords = coords;
 
    return OUT;
}

