#ifndef SKIN_INC
#define SKIN_INC

#include "Transform_inc.hlsl"


struct SkinJointBinding {
    Transform invBindPose;
    int4 bone;
};

//StructuredBuffer<SkinJointBindRow>  skin_joint_bind_array : register(t5);
StructuredBuffer<float4>  skin_joint_bind_array : register(t5);

Transform skin_fetchJoint_invBindPose(uint j) {
    return transform_makeFrom3Vec4(
        skin_joint_bind_array[j * 4],
        skin_joint_bind_array[j * 4 + 1],
        skin_joint_bind_array[j * 4 + 2]);
}

int4 skin_fetchJoint_bone(uint j) {
    return asint(skin_joint_bind_array[j * 4 + 3]);
}


#endif