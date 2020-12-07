#define mat43 float4x3

//
// Transform API
//
struct Transform {
    float4 _right_upX;
    float4 _upYZ_backXY;
    float4 _backZ_ori;

    float3 row_x() { return float3(_right_upX.x, _right_upX.w, _upYZ_backXY.z); }
    float3 row_y() { return float3(_right_upX.y, _upYZ_backXY.x, _upYZ_backXY.w); }
    float3 row_z() { return float3(_right_upX.z, _upYZ_backXY.y, _backZ_ori.x); }

    float3 col_x() { return _right_upX.xyz; }
    float3 col_y() { return float3(_right_upX.w, _upYZ_backXY.xy); }
    float3 col_z() { return float3(_upYZ_backXY.zw, _backZ_ori.x); }
    float3 col_w() { return _backZ_ori.yzw; }
};

float3 rotateFrom(const Transform mat, const float3 d) {
    return float3(dot(mat.row_x(), d), dot(mat.row_y(), d), dot(mat.row_z(), d));
}
float3 rotateTo(const Transform mat, const float3 d) {
    return float3(dot(mat.col_x(), d), dot(mat.col_y(), d), dot(mat.col_z(), d));
}

float3 transformTo(const Transform mat, const float3 p) {
    return rotateTo(mat, p - mat.col_w());
}
float3 transformFrom(const Transform mat, const float3 p) {
    return rotateFrom(mat, p) + mat.col_w();
}

//
// Projection API
// 
struct Projection {
    float4 _aspectRatio_sensorHeight_focal_far;
    float4 _ortho_enabled_height_near_far;

    float aspectRatio() { return (_aspectRatio_sensorHeight_focal_far.x); }
    float sensorHeight() { return (_aspectRatio_sensorHeight_focal_far.y); }
    float focal() { return (_aspectRatio_sensorHeight_focal_far.z); }
    float persFar() { return (_aspectRatio_sensorHeight_focal_far.w); }

    bool isOrtho() { return (_ortho_enabled_height_near_far.x > 0.0); }
    float orthoHeight() { return (_ortho_enabled_height_near_far.y); }
    float orthoNear() { return (_ortho_enabled_height_near_far.z); }
    float orthoFar() { return (_ortho_enabled_height_near_far.w); }
};


float4 clipFromEyeSpace(float aspectRatio, float sensorHeight, float focal, float pfar, float3 eyePos) {
    float ez = -eyePos.z;
    float pnear = focal;
    // Infinite far inverted Z
    // float b = 0.0f; //lim at far  infinite of  pnear / (pnear- pfar);;
    // float a = pnear; // lim at far infinite of - pfar * pnear / (pnear - pfar);
    float4 clipPos;
    clipPos.w = ez;
    clipPos.z = pnear;
    // float depthBuffer = z/w = a * (1/ez) + b; 
    clipPos.x = eyePos.x * pnear * 2.0 / (sensorHeight * aspectRatio);
    clipPos.y = eyePos.y * pnear * 2.0 / (sensorHeight);
    return clipPos;
}

float4 orthoClipFromEyeSpace(float aspectRatio, float sensorHeight, float pnear, float pfar, const float3 eyePos) {
    float4 clipPos;
    clipPos.w = 1.0f;
    clipPos.z = (pfar - (-eyePos.z)) / (pfar - pnear);
    clipPos.x = eyePos.x * 2.0f / (sensorHeight * aspectRatio);
    clipPos.y = eyePos.y * 2.0f / (sensorHeight);
    return clipPos;
}

float4 clipFromEyeSpace(Projection proj, float3 eyePos) {
    if (proj.isOrtho()) {
        return orthoClipFromEyeSpace(proj.aspectRatio(), proj.orthoHeight(), proj.orthoNear(), proj.orthoFar(), eyePos);
    }
    else {
        return clipFromEyeSpace(proj.aspectRatio(), proj.sensorHeight(), proj.focal(), proj.persFar(), eyePos);
    }
}

// Camera buffer
cbuffer UniformBlock0 : register(b0) {
    //float4x3 _view;
    Transform _view;
    Projection _projection;
    float4 _viewport;
};


float3 eyeFromWorldSpace(Transform view, float3 worldPos) {
    return transformTo(view, worldPos);
}

float3 worldFromObjectSpace(Transform model, float3 objPos) {
    return transformFrom(model, objPos);
}

float3 objectFromWorldSpaceDir(Transform model, float3 worldDir) {
    return rotateTo(model, worldDir);
}


//
// Transform Tree
//
StructuredBuffer<Transform>  tree_transforms : register(t0);

Transform node_getTransform(int nodeID) {
    return tree_transforms[2 * nodeID];
}
Transform node_getWorldTransform(int nodeID) {
    return tree_transforms[2 * nodeID + 1];
}

struct Box {
    float3 _center;
    float3 _size; 

    float3 getCorner(int i) {
        return _center + _size * float3( -1.0 + 2.0 * float((i) & 0x01), -1.0 + 2.0 * float((i >> 1) & 0x01), -1.0 + 2.0 * float((i >> 2) & 0x01));
        // 0:  - - - 
        // 1:  + - -
        // 2:  - + -
        // 3:  + + -
        // 4:  - - +
        // 5:  + - +
        // 6:  - + +
        // 7:  + + +
    }

    int2 getEdge(int i) {
        // 0: 0 1
        // 1: 2 3
        // 2: 4 5
        // 3: 6 7

        // 4: 0 2
        // 5: 1 3
        // 6: 4 6
        // 7: 5 7
 
        // 8: 0 4
        // 9: 1 5
        //10: 2 6
        //11: 3 7

        const int2 EDGES[12] = {
            int2(0, 1),
            int2(2, 3),
            int2(4, 5),
            int2(6, 7),

            int2(0, 2),
            int2(1, 3),
            int2(4, 6),
            int2(5, 7),

            int2(0, 4),
            int2(1, 5),
            int2(2, 6),
            int2(3, 7)
        };
        return EDGES[i];
    }
};

Box worldFromObjectSpace(Transform model, Box objBox) {
    Box wb;
    wb._center = transformFrom(model, objBox._center);
    wb._size.x = dot(abs(model.row_x()), objBox._size);
    wb._size.y = dot(abs(model.row_y()), objBox._size);
    wb._size.z = dot(abs(model.row_z()), objBox._size);
    return wb;
}

struct DrawableBound {
    Box _box;
    float spareA;
    float spareB;
};

StructuredBuffer<DrawableBound>  drawable_bounds : register(t1);

Box drawable_getLocalBox(int drawableID) {
    return drawable_bounds[drawableID]._box; //getBox();
}

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

cbuffer UniformBlock1 : register(b1) {
    int   _nodeID;
}


struct VertexPosColor
{
    float3 Position : POSITION;
    //  float3 Normal : NORMAL;
    float4 Color : COLOR;
};

struct VertexShaderOutput
{
    float4 Color    : COLOR;
    float4 Position : SV_Position;
};

VertexShaderOutput main(uint ivid : SV_VertexID)
{
    VertexShaderOutput OUT;

    const int transform_num_edges = 3;
    const int node_num_edges = 1;
    const int box_num_edges = 12;
    const int num_edges = box_num_edges + box_num_edges;

    uint vid = ivid % (2 * num_edges);
    uint itemid = ivid / (2 * num_edges);
    uint svid = vid % 2;
    uint lid = vid / 2;

    float3 position = float3(0.0, 0.0, 0.0);
    float3 color = float3(1.0, 1.0, 1.0);

    ItemInfo _item = item_getInfo(itemid);
    Transform _model = node_getWorldTransform(_item.nodeID);
    Box _box = drawable_getLocalBox(_item.drawableID);

    if (lid < (box_num_edges)) {
        //lid -= 0;
        int2 edge = _box.getEdge(lid);
        position = _box.getCorner(edge[svid]);
        color = float3(float(lid < 4), float(lid >= 4 && lid < 8), float(lid >= 8));
        position = worldFromObjectSpace(_model, position);
    }
    else {
        Box _world_box = worldFromObjectSpace(_model, _box);
    
        lid -= box_num_edges;
        int2 edge = _box.getEdge(lid);
        position = _world_box.getCorner(edge[svid]);
        color = float3(float(lid < 4), float(lid >= 4 && lid < 8), float(lid >= 8));
    }

    float3 eyePosition = eyeFromWorldSpace(_view, position);
    float4 clipPos = clipFromEyeSpace(_projection, eyePosition);

    OUT.Position = clipPos;
    OUT.Color = float4(color, 1.0f);
 
    return OUT;
}

