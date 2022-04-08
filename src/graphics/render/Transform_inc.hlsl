//
// Transform API
// 
// Define Transform struct and functions
//
#ifndef Transform_inc
#define Transform_inc

#define mat43 float4x3

float3 transform_rotateFrom(const float3 axisX, const float3 axisY, const float3 axisZ, const float3 d) {
    return float3(
        dot(float3(axisX.x, axisY.x, axisZ.x), d),
        dot(float3(axisX.y, axisY.y, axisZ.y), d),
        dot(float3(axisX.z, axisY.z, axisZ.z), d));
}

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
// Model / World / Eye Space transforms from Model and View
//

// Eye <= World
float3 eyeFromWorldSpace(Transform view, float3 worldPos) {
    return transformTo(view, worldPos);
}

// Eye <= World Dir
float3 eyeFromWorldSpaceDir(Transform view, float3 worldDir) {
    return rotateTo(view, worldDir);
}

// World <= Eye
float3 worldFromEyeSpace(Transform view, float3 eyePos) {
    return transformFrom(view, eyePos);
}

// World <= Eye dir
float3 worldFromEyeSpaceDir(Transform view, float3 eyeDir) {
    return rotateFrom(view, eyeDir);
}

// World <= Object
float3 worldFromObjectSpace(Transform model, float3 objPos) {
    return transformFrom(model, objPos);
}

// World <= Object  dir
float3 worldFromObjectSpaceDir(Transform model, float3 objDir) {
    return rotateFrom(model, objDir);
}

// Object <= World dir
float3 objectFromWorldSpaceDir(Transform model, float3 worldDir) {
    return rotateTo(model, worldDir);
}










//
// Box API
//

struct Box {
    float3 _center;
    float3 _size;

    float3 getCorner(int i) {
        return _center + _size * float3(-1.0 + 2.0 * float((i) & 0x01), -1.0 + 2.0 * float((i >> 1) & 0x01), -1.0 + 2.0 * float((i >> 2) & 0x01));
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

    int3 getTriangle(int i) {
        // 0: 2 0 6
        // 1: 6 0 4
        // 2: 1 3 5
        // 3: 5 3 7

        // 4: 0 1 4
        // 5: 4 1 5
        // 6: 2 6 3 
        // 7: 3 6 7

        // 8: 0 2 1
        // 9: 1 2 3
        //10: 4 5 6
        //11: 6 5 7

        const int3 TRIS[12] = {
            int3(2, 0, 6),
            int3(6, 0, 4),
            int3(1, 3, 5),
            int3(5, 3, 7),

            int3(0, 1, 4),
            int3(4, 1, 5),
            int3(2, 6, 3),
            int3(3, 6, 7),

            int3(0, 2, 1),
            int3(1, 2, 3),
            int3(4, 5, 6),
            int3(6, 5, 7)
        };
        return TRIS[i];
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

#endif