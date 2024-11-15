//
// Transform API
// 
// Define Transform struct and functions
//
#ifndef TRANSFORM_INC
#define TRANSFORM_INC

#define mat43 float4x3

// evaluate an orthonormal base from a direction D
// D is injected as the Z axis and teh function produce the X and Y axis
// Algorithm by Jeppe Revall Frisvad
// https://backend.orbit.dtu.dk/ws/portalfiles/portal/126824972/onb_frisvad_jgt2012_v2.pdf
void transform_evalOrthonormalBase(in float3 D, out float3 X, out float3 Y) {
    // Handle the singularity
    if (D.z < -0.9999999f) {
        X = float3(0.0f, -1.0f, 0.0f);
        Y = float3(-1.0f, 0.0f, 0.0f);
        return;
    }
    
    float a = 1.0f * rcp(1.0f + D.z);
    float b = -D.x * D.y * a;
    X = float3(1.0f - D.x * D.x * a, b, -D.x);
    Y = float3(b, 1.0f - D.y * D.y * a, -D.y);
     
    // Naive implementation
    // Y = (abs(dir.y) < 0.95f ? float3(0, 1, 0) : float3(1, 0, 0));
    // X = normalize(cross(Y, D));
    // Y = cross(D, X); 
}

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

    float4 r0() { return _right_upX; }
    float4 r1() { return _upYZ_backXY; }
    float4 r2() { return _backZ_ori; }

    float4x4 toMat44() { return float4x4(float4(col_x(), 0), float4(col_y(), 0), float4(col_z(), 0), float4(col_w(), 1.0)); }
    float4x4 toInverseMat44() { return float4x4( float4(row_x(), 0), float4(row_y(), 0), float4(row_z(), 0), float4(col_w(), 1.0)); }
    
    void identity() {
        _right_upX   = float4(1, 0, 0, 0);
        _upYZ_backXY = float4(1, 0, 0, 0);
        _backZ_ori   = float4(1, 0, 0, 0);
    }

    void from4Columns(float3 c0, float3 c1, float3 c2, float3 c3) {
        _right_upX = float4(c0, c1.x);
        _upYZ_backXY = float4(c1.yz, c2.xy);
        _backZ_ori = float4(c2.z, c3);
    }
    void from3Vec4(float4 r0, float4 r1, float4 r2) {
        _right_upX = r0;
        _upYZ_backXY = r1;
        _backZ_ori = r2;
    }

};

Transform add(Transform a, Transform b) {
    Transform r;
    r.from3Vec4(a.r0() + b.r0(), a.r1() + b.r1(), a.r2() + b.r2());
    return r;
}

Transform transform_makeFrom3Vec4(float4 r0, float4 r1, float4 r2) {
    Transform t;
    t.from3Vec4(r0, r1, r2);
    return t;
}

Transform transform_inverse(Transform m) {
    Transform t;
    t.from4Columns(m.row_x(), m.row_y(), m.row_z(), -m.col_w());
    return t;
}

Transform scale(Transform m, float s) {
    Transform t;
    t.from3Vec4(m._right_upX * s, m._upYZ_backXY * s, m._backZ_ori * s);
    return t;
}
Transform scale(float s, Transform m) {
    return scale(m, s);
}

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

Transform mul(Transform a, Transform b) {
    float3 a_row_0 = a.row_x();
    float3 a_row_1 = a.row_y();
    float3 a_row_2 = a.row_z();

    float3 b_col_x = b.col_x();
    float3 c_col_x = float3(
                        dot(a_row_0, b_col_x),
                        dot(a_row_1, b_col_x),
                        dot(a_row_2, b_col_x) );
    float3 b_col_y = b.col_y();
    float3 c_col_y = float3(
                        dot(a_row_0, b_col_y),
                        dot(a_row_1, b_col_y),
                        dot(a_row_2, b_col_y) );
    float3 b_col_z = b.col_z();
    float3 c_col_z = float3(
                        dot(a_row_0, b_col_z),
                        dot(a_row_1, b_col_z),
                        dot(a_row_2, b_col_z) );
    float3 b_col_w = b.col_w();
    float3 a_col_w = a.col_w();
    float3 c_col_w = float3(
                        dot(a_row_0, b_col_w) + a_col_w.x,
                        dot(a_row_1, b_col_w) + a_col_w.y,
                        dot(a_row_2, b_col_w) + a_col_w.z );
    Transform c;
    c.from4Columns(c_col_x, c_col_y, c_col_z, c_col_w);
    return c;
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