#ifndef CAMERA_INC
#define CAMERA_INC

#include "Transform_inc.hlsl"
#include "Projection_inc.hlsl"


// Camera buffer
/*cbuffer UniformBlock0 : register(b10) {
    //float4x3 _view;
    Transform _view;
    Projection _projection;
    float4 _viewport;
};*/


struct Camera {
  //float4x3 _view;
    Transform _view;
    Projection _projection;
    float4 _viewport;
};

StructuredBuffer<Camera> camera_array : register(t19);

Transform cam_view() {
    return camera_array[0]._view;
}

Projection cam_projection() {
    return camera_array[0]._projection;
}

float4 cam_viewport() {
    return camera_array[0]._viewport;
}
    
float cam_pixelSizeAt(float eyeDepth) {
    Projection _projection = cam_projection();
    float4 _viewport = cam_viewport();
    
    float pixelSize = _projection.sensorHeight() / _viewport.w;
    return abs(eyeDepth) * pixelSize / _projection.focal();
}

#endif
