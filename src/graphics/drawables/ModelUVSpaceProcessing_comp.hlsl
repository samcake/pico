

//
// Model
//
struct Part {
    uint numIndices;
    uint indexOffset;
    uint vertexOffset;
    uint attribOffset;
    uint material;
    uint spareA;
    uint spareB;
    uint spareC;
};

StructuredBuffer<Part>  part_array : register(t1);

Buffer<uint>  index_array : register(t2);

struct Vertex {
    float x;
    float y;
    float z;
    uint n;
};

StructuredBuffer<Vertex>  vertex_array : register(t3);

struct VertexAttrib {
    float x;
    float y;
    float z;
    float w;
};

StructuredBuffer<VertexAttrib>  vertex_attrib_array : register(t4);

uint3 fetchFaceIndices(int indexOffset, int faceNum) {
    // Fetch Face indices
    int faceIndexBase = indexOffset + faceNum * 3;
    return uint3(index_array[faceIndexBase], index_array[faceIndexBase + 1], index_array[faceIndexBase + 2]);
}

struct Face {
    float3 v[3];
    uint3 n;
};

Face fetchFaceVerts(uint3 fvid, int vertexOffset) {
    // Fetch Face vertices
    Face face;
    for (int i = 0; i < 3; i++) {
        uint vi = vertexOffset + fvid[i];
        Vertex v = vertex_array[vi];
        face.v[i] = float3(v.x, v.y, v.z);
        face.n[i] = v.n;
    }

    return face;
}

Face fetchFaceAttribs(uint3 fvid, int attribOffset) {
    // Fetch Face vertices
    Face face;
    for (int i = 0; i < 3; i++) {
        uint vi = attribOffset + fvid[i];
        face.v[i] = float4(vertex_attrib_array[vi].x, vertex_attrib_array[vi].y, vertex_attrib_array[vi].z, vertex_attrib_array[vi].w);
    }

    return face;
}

struct Edge {
    uint p;
    uint2 i;
    uint d;
};

StructuredBuffer<Edge>  edge_array : register(t5);

//
// Material API
// 
struct Material {
    float4 color;
    float metallic;
    float roughness;
    float spareA;
    float spareB;
    float4 emissive;
    uint4 textures;
};

StructuredBuffer<Material>  material_array : register(t9);

Texture2DArray uTex0 : register(t10);
Texture2D uTex1 : register(t11);
SamplerState uSampler0 : register(s0);


//
// Kernel 
// 

cbuffer UniformBlock1 : register(b1) {
    int   _nodeID;
    int  _map_width;
    int  _map_height;
    float _map_spacing;
    int  _mesh_width;
    int  _mesh_height;
    float _mesh_spacing;
}


// Read previous state from 
RWTexture2D<float4> out_buffer : register(u0);

[numthreads(32, 32, 1)]
void main_blur(uint3 DTid : SV_DispatchThreadID)
{
    int2 pixelCoord = DTid.xy;

    int matIdx = part_array[0].material;
    Material m = material_array[matIdx];
    uint mapId = m.textures.x;
    if (mapId == -1) {
        return;
    }

    float3 color = float3(0,0,0);

    float tex_width, tex_height, tex_elements;
    uTex0.GetDimensions(tex_width, tex_height, tex_elements);

    int radius = 5;
    float numSamples = 0;
    int2 coord = pixelCoord;
    int minX = max(0, pixelCoord.x - radius);
    int maxX = min(int(tex_width) - 1, pixelCoord.x + radius);

    int minY = max(0, pixelCoord.y - radius);
    int maxY = min(int(tex_height) - 1, pixelCoord.y + radius);

    for (int i = minX; i <= maxX; ++i) {
        for (int j = minY; j <= maxY; ++j) {
            float mask = uTex1[uint2(i, j)].w;
            float3 texel = uTex0[uint3(i, j, mapId)].xyz;
            texel = lerp(texel, float3(1.0, 1.0, 1.0), mask);
            color += texel;
            numSamples += 1.0;
        }
    }
    
    out_buffer[pixelCoord] = float4(color / numSamples, 1);
}

