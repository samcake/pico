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
    float4 Color  : COLOR;
    float4 Position : SV_Position;
};

Transform evalSkinMatrix(int j, int modelNodeRootIndex) {
    Transform skinBindPose = skin_fetchJoint_invBindPose(j);
    int4 bone = skin_fetchJoint_bone(j);
    Transform jointGlobalTransform = node_getWorldTransform(bone.x + modelNodeRootIndex);
    return mul(jointGlobalTransform, skinBindPose);
   // return mul(skinBindPose, jointGlobalTransform);
}

VertexShaderOutput main(uint vidx : SV_VertexID) {

    // Fetch the part data
    Part p = part_array[_partID];

    uint tidx = vidx / 3;
    uint tvidx = vidx % 3;

    uint3 faceIndices = mesh_fetchFaceIndices(tidx, p.indexOffset);
    FaceVerts faceVerts = mesh_fetchFaceVerts(faceIndices, p.vertexOffset);

    float2 texcoord = mesh_fetchVertexTexcoord(faceIndices[tvidx], p.attribOffset);
    uint2 skinParam = mesh_fetchVertexSkinParam(faceIndices[tvidx], p.attribOffset);
    bool skinned = (skinParam.x != 0);
    int4 skinJoints = 0;
    float4 skinWeights = unpackSkinParamFrom2U(skinParam, skinJoints);
 
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
    float3 skinnedPos = 0;
    int skinJointNodeId = 0;

    Transform rootGlobalTransform = node_getWorldTransform(_nodeID);
    // rootGlobalTransform.identity();
    Transform invGlobalMatrix = transform_inverse(rootGlobalTransform);

    if (skinned) {
        int jointNodeOffset = 2; // 2 + 3 = 5
        Transform sm0 = mul(invGlobalMatrix, evalSkinMatrix(skinJoints.x, jointNodeOffset));
        Transform sm1 = mul(invGlobalMatrix, evalSkinMatrix(skinJoints.y, jointNodeOffset));
        Transform sm2 = mul(invGlobalMatrix, evalSkinMatrix(skinJoints.z, jointNodeOffset));
        Transform sm3 = mul(invGlobalMatrix, evalSkinMatrix(skinJoints.w, jointNodeOffset));

        //Transform skinMatrix = add(
        //    add(scale(mul(invGlobalMatrix, evalSkinMatrix(skinJoints.x, jointNodeOffset)), skinWeights.x),// +
        //        scale(mul(invGlobalMatrix, evalSkinMatrix(skinJoints.y, jointNodeOffset)), skinWeights.y)), // +
        //            add(scale(mul(invGlobalMatrix, evalSkinMatrix(skinJoints.z, jointNodeOffset)), skinWeights.z), // +
        //                scale(mul(invGlobalMatrix, evalSkinMatrix(skinJoints.w, jointNodeOffset)), skinWeights.w))
        //            );
        
        //Transform skinMatrix = add(
        //    add(scale(sm0, skinWeights.x),// +
        //        scale(sm1, skinWeights.y)), // +
        //    add(scale(sm2, skinWeights.z), // +
        //        scale(sm3, skinWeights.w))
        //);

        //skinMatrix = transform_inverse(skinMatrix);
        skinnedPos += skinWeights.x * transformFrom(sm0, position);
        skinnedPos += skinWeights.y * transformFrom(sm1, position);
        skinnedPos += skinWeights.z * transformFrom(sm2, position);
        skinnedPos += skinWeights.w * transformFrom(sm3, position);
        position = skinnedPos.xyz;
    }

  //  position += 0.09f * (barycenter - position);

    Transform _model = node_getWorldTransform(_nodeID);
    Transform _view = cam_view();
    Projection _projection = cam_projection();

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
    OUT.Color = float4((skinWeights.xyz * 1.0f), 1);
   // OUT.Color = float4((skinJoints.xyz * (1.0f / 32.0)), 1);
  //  OUT.Color.xyz = ((skinJointNodeId - 2 ) * 1.0f);
    //OUT.Color.xyz = 0.01;

    return OUT;
}

