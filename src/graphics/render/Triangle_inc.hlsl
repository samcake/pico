
//
// Triangle API
// 

// Triangle ray intersection by IQ https://iquilezles.org/
// triangle defined by vertices v0, v1 and  v2
float3 triangle_intersect(in float3 ro, in float3 rd, in float3 v0, in float3 v1, in float3 v2) {
    float3 v1v0 = v1 - v0;
    float3 v2v0 = v2 - v0;
    float3 rov0 = ro - v0;
    float3  n = cross(v1v0, v2v0);
    float3  q = cross(rov0, rd);
    float d = 1.0 / dot(rd, n);
    float u = d * dot(-q, v2v0);
    float v = d * dot(q, v1v0);
    float t = d * dot(-n, rov0);
    if (u < 0.0 || u>1.0 || v < 0.0 || (u + v)>1.0) t = -1.0;
    return float3(t, u, v);
}
