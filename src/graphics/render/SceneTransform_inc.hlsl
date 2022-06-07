//
// Scene API
//
#ifndef SCENETRANSFORM_INC
#define SCENETRANSFORM_INC

#include "Camera_inc.hlsl"


//
// Transform Tree API
//
StructuredBuffer<Transform>  tree_transforms : register(t20);


Transform node_getTransform(int nodeID) {
    return tree_transforms[2 * nodeID];
}
Transform node_getWorldTransform(int nodeID) {
    return tree_transforms[2 * nodeID + 1];
}


//
// Drawable API
//
struct DrawableBound {
    Box _box;
    float spareA;
    float spareB;
};

StructuredBuffer<DrawableBound>  drawable_bounds : register(t1);

Box drawable_getLocalBox(int drawableID) {
    return drawable_bounds[drawableID]._box; //getBox();
}


//
// Item API
//
struct ItemInfo {
    uint  nodeID;
    uint  drawableID;
    float spareA;
    float spareB;
};

StructuredBuffer<ItemInfo>  item_infos : register(t2);

ItemInfo item_getInfo(int itemID) {
    return item_infos[itemID];
}

#endif
