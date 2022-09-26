#ifndef CAMERA_INC
#define CAMERA_INC

#include "Transform_inc.hlsl"
#include "Projection_inc.hlsl"


// Camera buffer
cbuffer UniformBlock0 : register(b10) {
    //float4x3 _view;
    Transform _view;
    Projection _projection;
    float4 _viewport;
};

float cam_pixelSizeAt(float eyeDepth)
{
    float pixelSize = _projection.sensorHeight() / _viewport.w;
    return abs(eyeDepth) * pixelSize / _projection.focal();
}

#endif
