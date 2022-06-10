//
// Hexagon API
//
#ifndef HEXAGON_INC
#define HEXAGON_INC

static const int NUM_HEX_DIR = 6;
static const int3 hex_direction_vectors[NUM_HEX_DIR] = {
    int3(+1, 0, -1), int3(0, +1, -1), int3(-1, +1, 0),
    int3(-1, 0, +1), int3(0, -1, +1), int3(+1, -1, 0)
};

int3 hex_dir(uint dir, int ring = 1) {
    return hex_direction_vectors[uint(dir) % 6] * ring;
}

int3 hex_add(int3 a, int3 b) {
    return a + b;
}

int3 hex_sub(int3 a, int3 b) {
    return a - b;
}

int hex_dist(int3 a, int3 b) {
    int3 d = abs(a - b);
    return (d.x + d.y + d.z) >> 1;
}

int3 hex_round(float3 h)
{
    float3 rh = round(h);
    float check = rh.x + rh.y + rh.z;
    if (check != 0)
    {
        float3 hc = abs(h - rh);
        if ((hc.x > hc.y) && (hc.x > hc.z))
            rh.x = -rh.y - rh.z;
        else if (hc.y > hc.z)
            rh.y = -rh.x - rh.z;
        else
            rh.z = -rh.x - rh.y;
    }
    return int3(rh);
}

int3 hex_neighbor(int3 h, int dir)
{
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


static const float SQRT_3 = sqrt(3.0);
static const float HALF_SQRT_3 = SQRT_3 * 0.5;
static const float THIRD_SQRT_3 = SQRT_3 / 3.0;

static const int HEX_NUM_VERTS = 7;
static const float2 HEX_VERTS[HEX_NUM_VERTS + 1] = {
    float2(0, 0), // center of hex
    float2(0, 1),  // then the 6 corners of the hex
    float2(-HALF_SQRT_3, 0.5),
    float2(-HALF_SQRT_3, -0.5),
    float2(0,-1),
    float2(HALF_SQRT_3, -0.5),
    float2(HALF_SQRT_3, 0.5),
    float2(0, 1), // repeat the first corner for easy indexing
};

static const uint HEX_NUM_TRIANGLES = 6;
static const uint HEX_NUM_INDICES = 3 * HEX_NUM_TRIANGLES;

uint hex_index_to_vertex(uint idx, out uint tid, out uint tvid) {
    tid = idx / 3;
    tvid = idx - tid * 3;
    return (tvid ? tvid + tid : 0);
}


static const float2x3 HEX_TO_2D = float2x3(
    HALF_SQRT_3, 0, -HALF_SQRT_3,
    -0.5, 1, -0.5
    );
    
    
static const float2x3 _2D_TO_HEX = float2x3(
    HALF_SQRT_3, 0, -HALF_SQRT_3,
    -0.5, 1, -0.5
    );


float2 hex_cube_to_ortho(int3 p, float radius = 1.0) {
    return mul(HEX_TO_2D, float3(p)) * radius;
}



#endif
