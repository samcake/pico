#include "SceneTransform_inc.hlsl"
#include "SceneModel_inc.hlsl"


cbuffer UniformBlock1 : register(b1) {
    int _nodeID;
    int _partID;
    int _numNodes;
    int _numParts;
    int _numMaterials;
}

struct VertexShaderOutput
{
    float3 EyePos : EPOS;
    float3 Normal   : NORMAL;
    float2 Texcoord  : TEXCOORD;
    float4 Position : SV_Position;
};

VertexShaderOutput main(uint vidx : SV_VertexID) {

    // Fetch the part data
    Part p = part_array[_partID];

    uint tidx = vidx / 3;
    uint tvidx = vidx % 3;

    uint3 faceIndices = mesh_fetchFaceIndices(tidx, p.indexOffset);
    FaceVerts faceVerts = mesh_fetchFaceVerts(faceIndices, p.vertexOffset);

    float2 texcoord = mesh_fetchVertexTexcoord(faceIndices[tvidx], p.attribOffset);
    

    // Generate normal
    float3 faceEdge0 = faceVerts.v[1].xyz - faceVerts.v[0].xyz;
    float3 faceEdge1 = faceVerts.v[2].xyz - faceVerts.v[0].xyz;
    float3 normal = normalize(cross(faceEdge0, faceEdge1));

    normal = unpackNormalFrom32I(faceVerts.n[tvidx]);
  //  normal = float3(0.0, 1.0, 0.0);

    // Barycenter 
    float3 barycenter = (faceVerts.v[0].xyz + faceVerts.v[1].xyz + faceVerts.v[2].xyz) / 3.0f;

    // Transform
    float3 position = faceVerts.v[tvidx].xyz;
    position += 0.0f * (barycenter - position);

    Transform _model = node_getWorldTransform(_nodeID);

    position = worldFromObjectSpace(_model, position);
    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    normal = worldFromObjectSpaceDir(_model, normal);

  /*  uint color = asuint(p.material);
    const float INT8_TO_NF = 1.0 / 255.0;
    float r = INT8_TO_NF * (float)((color >> 0) & 0xFF);
    float g = INT8_TO_NF * (float)((color >> 8) & 0xFF);
    float b = INT8_TO_NF * (float)((color >> 16) & 0xFF);
*/

  //  float3 camera_pos = worldFromEyeSpace(_view, float3(0, 0, 0));
    float3 camera_pos = _view.col_w();

    VertexShaderOutput OUT;
    OUT.Position = clipPos;
  //  OUT.EyePos = eyePosition;
    OUT.EyePos = camera_pos - position;
    OUT.Normal = normal;
    OUT.Texcoord = texcoord;
    
    return OUT;
}

