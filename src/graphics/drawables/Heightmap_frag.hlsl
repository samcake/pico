
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

float3 getLightDir() {
    // the direction TO the light
    return -normalize(float3(1.f, -1.f, 0.f));
}

struct PixelShaderInput
{
    float4 Color    : COLOR;
    float3 WPos     : WPOS;
    float3 Normal   : SPRITE;
};

float3 SkyColor(const float3 dir) {
    float3 HORIZON_COLOR = float3(1.71, 1.31, 0.83);
    float3 SKY_COLOR = float3(0.4, 0.75, 1);
    float3 SUN_COLOR = float3(1.5, 1, 0.6);
    float3 SUN_DIRECTION = getLightDir();

    float mixWeight = sqrt(abs(dir.z));
    float3 sky = (1 - mixWeight) * HORIZON_COLOR + mixWeight * SKY_COLOR;
    float3 sun = 4 * SUN_COLOR * pow(max(dot(SUN_DIRECTION, dir), 0), 1500);
    return sky + sun;
}

/*
* This is done in eval shading specular diffuse
float3 colorDiffuse(float3 base_color, float metallic) {
    const float3 dielectricSpecular = float3(0.04, 0.04, 0.04);
    const float3 black = float3(0, 0, 0);
    float3 cdiff = lerp(base_color.rgb * (1 - dielectricSpecular.r), black, metallic);
    return cdiff;
}
*/

float3 colorF0(float3 base_color, float metallic) {
    const float3 dielectricSpecular = float3(0.04, 0.04, 0.04);
    float3 cdiff = lerp(dielectricSpecular, base_color.rgb, metallic);
    return cdiff;
}

float3 fresnel(float v_dot_h, float3 f0) {
    float base = (1.0 - v_dot_h);
    return f0 + (float3(1.0, 1.0, 1.0) - f0) * (base *base * base * base * base);
}

float evalSmithInvG1(float roughness4, float ndotd) {
    return ndotd + sqrt(roughness4 + ndotd * ndotd * (1.0 - roughness4));
}

float eval_GeometricOcclusion_NormalDistribution(float n_dot_l, float n_dot_v, float n_dot_h, float roughness2) {
    // See https://www.khronos.org/assets/uploads/developers/library/2017-web3d/glTF-2.0-Launch_Jun17.pdf
    // for details of equations, especially page 20
    float roughness4 = roughness2 * roughness2;

    // Distribution GGX
    float denom = (n_dot_h * n_dot_h * (roughness2 - 1.0) + 1.0);
    denom *= denom;

    // Geometric Occlusion factors G1(n,l) and G1(n,v)
    float smithInvG1NdotV = evalSmithInvG1(roughness4, n_dot_v);
    float smithInvG1NdotL = evalSmithInvG1(roughness4, n_dot_l);
    denom *= smithInvG1NdotV * smithInvG1NdotL;
    // Don't divide by PI as this is part of the light normalization factor
    float power = roughness4 / denom;
    return power;
}


float4 evalShading(float l_dot_h, float v_dot_h, float n_dot_l, float n_dot_v, float n_dot_h, float roughness2, float metallic, float3 f0) {
    // Incident angle attenuation
    float angleAttenuation = n_dot_l;

    // Specular Lighting
    float3 fresnelColor = fresnel(l_dot_h, f0);
    float power = eval_GeometricOcclusion_NormalDistribution(n_dot_l, n_dot_v, n_dot_h, roughness2);
    float3 specular = fresnelColor * power * angleAttenuation;

    // Diffuse Lighting
    // assuming the fresnel(v.h) term is equal to fresnelColor.x
    float diffuse = (1.0 - metallic) * angleAttenuation * (1.0 - fresnelColor.x);

    // We don't divided by PI, as the "normalized" equations state we should, because we decide, as Naty Hoffman, that
    // we wish to have a similar color as raw albedo on a perfectly diffuse surface perpendicularly lit
    // by a white light of intensity 1. But this is an arbitrary normalization of what light intensity "means".
    // (see http://blog.selfshadow.com/publications/s2013-shading-course/hoffman/s2013_pbs_physics_math_notes.pdf
    // page 23 paragraph "Punctual light sources")
    return float4(specular, diffuse);
}

float4 main(PixelShaderInput IN) : SV_Target
{
    float3 L = getLightDir();
//    float3 N = normalize(IN.Normal);
    float3 N = normalize(cross(ddy(IN.WPos), ddx(IN.WPos)));


    float3 E2F = IN.WPos - _view.col_w();
    float E2Flen = length(E2F);
    float3 V = -normalize(E2F);
    float3 H = normalize(V + L);


    float3 diff_color = SkyColor(N);
    float3 spec_color = SkyColor(reflect(V, N));


    float metallic = 0.0;
    float roughness = 0.01;
    float roughness2 = roughness * roughness;
    float3 base_color = float3(1.0, 1.0, 1.0);
   // float3 base_color = float3(0.02, 0.02, 0.02); // water

    float3 F0 = colorF0(base_color, metallic);


    float n_dot_l = dot(N, L);
    float n_dot_h = dot(N, H);
    float n_dot_v = dot(N, V);
    float v_dot_h = dot(V, H);
    float l_dot_h = dot(L, H);

    float4 spec_diff_lighting = evalShading(l_dot_h, v_dot_h, n_dot_l, n_dot_h, n_dot_v, roughness2, metallic, F0);

    float3 color = spec_diff_lighting.w * base_color + spec_diff_lighting.xyz * 0.0;
//    color *= 0.8 * base_color;

//    float fog = (1.0f - max(0.f, pow(1 - E2Flen / 100.f, 1.2f)));
 //   color = fog * color;

    return float4(pow(color,  2.2), 1.0);
    return float4(N, 1.0);
}