

cbuffer UniformBlock1 : register(b1) {
    int   _nodeID;
    int  _map_width;
    int  _map_height;
    float _map_spacing;
    int  _mesh_width;
    int  _mesh_height;
    float _mesh_spacing;
}


// Write up to 4 mip map levels.
RWStructuredBuffer<float> out_buffer : register(u0);

[numthreads(32, 32, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float t = float(_nodeID) / 60.0;
    float2 p = float2(DTid.x, DTid.y) * _map_spacing;
    float2 d = float2(0.5, 0.1);
    float2 a = float2(40.0, 10.0);
    out_buffer[DTid.x + _map_width * DTid.y] = a.x * sin(p.x * 0.05+ t * d.x + p.y * 0.03 + t * d.y);
}

