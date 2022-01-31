#include "SceneTransform_inc.hlsl"

//
// Instance
//
cbuffer UniformBlock1 : register(b1) {
    int4 _instance;
    int _numVertices;
    int _numIndices;
    int spareA;
    float _triangleScale;
}

struct PointPosColor {
    float x;
    float y;
    float z;
    uint  color;
};

StructuredBuffer<PointPosColor>  VerticesIn : register(t1);
Buffer<uint>  IndicesIn : register(t2);

struct VertexShaderOutput
{
    float4 Color    : COLOR;
    float3 Normal   : NORMAL;
    float4 Position : SV_Position;
};

VertexShaderOutput main(uint vidx : SV_VertexID) {

    uint tidx = vidx / 3;
    uint tvidx = vidx % 3;

    // Fetch Face indices
    uint3 fvid = uint3(IndicesIn[tidx * 3], IndicesIn[tidx * 3 + 1], IndicesIn[tidx * 3 + 2]);
    uint vid = fvid[tvidx];

    // Fetch Face vertices
    float4 faceVerts[3];
    for (int i = 0; i < 3; i++) {
        uint vi = fvid[i];
        faceVerts[i] = float4(VerticesIn[vi].x, VerticesIn[vi].y, VerticesIn[vi].z, asfloat(VerticesIn[vi].color));
    }

    // Generate normal
    float3 faceEdge0 = faceVerts[1].xyz - faceVerts[0].xyz;
    float3 faceEdge1 = faceVerts[2].xyz - faceVerts[0].xyz;
    float3 normal = normalize(cross(faceEdge0, faceEdge1));
 
    // Barycenter 
    float3 barycenter = (faceVerts[0].xyz + faceVerts[1].xyz + faceVerts[2].xyz) / 3.0f;

    // Transform
    float3 position = faceVerts[tvidx].xyz;
    position += _triangleScale * (barycenter - position);

    Transform _model = node_getWorldTransform(_instance.x);


    position = worldFromObjectSpace(_model, position);
    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    uint color = asuint(faceVerts[tvidx].w);
    const float INT8_TO_NF = 1.0 / 255.0;
    float r = INT8_TO_NF * (float)((color >> 0) & 0xFF);
    float g = INT8_TO_NF * (float)((color >> 8) & 0xFF);
    float b = INT8_TO_NF * (float)((color >> 16) & 0xFF);

    VertexShaderOutput OUT;
    OUT.Position = clipPos;
    OUT.Normal = normal;
    OUT.Color = float4(r, g, b, 1.0f);

    return OUT;
}
