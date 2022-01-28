#include "Camera_inc.hlsl"


//
// Scene Transform Tree API
//
StructuredBuffer<Transform>  tree_transforms : register(t20);


Transform node_getTransform(int nodeID) {
    return tree_transforms[2 * nodeID];
}
Transform node_getWorldTransform(int nodeID) {
    return tree_transforms[2 * nodeID + 1];
}

