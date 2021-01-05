
/*
cbuffer UniformBlock1 : register(b1) {
    int   _nodeID;
    int  _map_width;
    int  _map_height;
    float _map_spacing;
    int  _mesh_width;
    int  _mesh_height;
    float _mesh_spacing;
}
*/

// Write up to 4 mip map levels.
RWStructuredBuffer<float> out_buffer : register(u0);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    out_buffer[DTid.x] = DTid.x;
}

