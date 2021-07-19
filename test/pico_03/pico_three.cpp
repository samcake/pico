// pico_three.cpp 
//
// Sam Gateau - January 2020
// 
// MIT License
//
// Copyright (c) 2020 Sam Gateau
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <chrono>

#include <core/api.h>

#include <graphics/gpu/Device.h>
#include <graphics/gpu/Resource.h>
#include <graphics/gpu/Shader.h>
#include <graphics/gpu/Descriptor.h>
#include <graphics/gpu/Pipeline.h>
#include <graphics/gpu/Batch.h>
#include <graphics/gpu/Swapchain.h>

#include <graphics/render/Renderer.h>
#include <graphics/render/Mesh.h>

#include <document/PointCloud.h>

#include <uix/Window.h>

#include <vector>

//--------------------------------------------------------------------------------------
// pico 3: Load and render a Pointcloud with different view/projection transform
// introducing:
// gpu::Buffer as Uniform Buffer
// gpu::Descriptor and DescriptorSetLayout
// gpu::Buffer as Vertex & Index buffer
// document::Pointcloud
// render::Mesh


//--------------------------------------------------------------------------------------
const std::string vertexShaderSource = std::string(R"HLSL(
/*struct ModelViewProjection
{
    matrix MVP;
};
*/
//ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

cbuffer UniformBlock0 : register(b0)
{
    //graphics::vec3 _eye{ 0.0f };                float _focal { 0.036f };
    //graphics::vec3 _right { 1.f, 0.f, 0.f};     float _sensorHeight { 0.056f };
    //graphics::vec3 _up { 0.f, 1.f, 0.f };       float _aspectRatio { 16.f / 9.f };
    //graphics::vec3 _back { 0.f, 0.f, -1.f };    float _far { 10.0f };

    float4 eye_focal;
    float4 right_sensorHeight;
    float4 up_aspectRatio;
    float4 back_far;
};

float3 eyeFromClipSpace(float focal, float sensorHeight, float aspectRatio, float2 clipPos) {
	return float3(clipPos.x*aspectRatio*sensorHeight * 0.5, clipPos.y * sensorHeight * 0.5, -focal);
}
float3 worldFromEyeSpaceDir(float3 right, float3 up, float3 eyeDir) {
	return eyeDir * eyeDir.x + up * eyeDir.y + cross(right, up) * eyeDir.z;
}
float3 worldFromEyeSpace(float3 eye, float3 right, float3 up, float3 eyePos) {
	return worldFromEyeSpaceDir(right, up, eyePos) + eye;
}
/*
float3 worldFromEyeSpaceDir(float3 right, float3 up, float3 eyeDir) {
	return eyeDir * eyePos.x + up * eyeDir.y + cross(right, up) * eyeDir.z;
}*/
float3 eyeFromWorldSpace(float3 eye, float3 right, float3 up, float3 worldPos) {
	float3 eyeCenteredPos = worldPos - eye;
    return float3( 
        dot(right, eyeCenteredPos),
        dot(up, eyeCenteredPos),
        dot(cross(right, up), eyeCenteredPos)
    );
}

float4 clipFromEyeSpace(float focal, float sensorHeight, float aspectRatio, float far, float3 eyePos) {
    
   // float foc = focal* (eyePos.x < 0.0 ? 1.0 : 1.5);
    float foc = focal;
    float w = foc - eyePos.z;
    float z = (-eyePos.z) *far / (far - foc);
    float x = eyePos.x * foc / (0.5 * sensorHeight * aspectRatio);
    float y = eyePos.y * foc / (0.5 * sensorHeight);
    return float4(x, y, z, w);
}


struct VertexPosColor
{
    float3 Position : POSITION;
    float4 Color : COLOR;
};

struct VertexShaderOutput
{
    float4 Color    : COLOR;
    float4 Position : SV_Position;
};

VertexShaderOutput mainVertex(VertexPosColor IN)
{
    VertexShaderOutput OUT;

    float3 position = IN.Position;
    float3 eyePosition = eyeFromWorldSpace(eye_focal.xyz, right_sensorHeight.xyz, up_aspectRatio.xyz, position);
  
    float4 clipPos = clipFromEyeSpace(eye_focal.w, right_sensorHeight.w, up_aspectRatio.w, back_far.w, eyePosition);
    OUT.Position = clipPos;
    OUT.Color = float4(IN.Color.xyz, 1.0f);

    return OUT;
}

)HLSL");

const std::string pixelShaderSource = std::string(R"HLSL(
struct PixelShaderInput
{
    float4 Color : COLOR;
};

float4 mainPixel(PixelShaderInput IN) : SV_Target
{
    return IN.Color;
}
)HLSL");

graphics::PipelineStatePointer createPipelineState(const graphics::DevicePointer& device, graphics::StreamLayout vertexLayout, const graphics::DescriptorSetLayoutPointer& descriptorSetLayout) {

    graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "mainVertex", "", vertexShaderSource };
    graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);


    graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "mainPixel", "", pixelShaderSource };
    graphics::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

    graphics::ProgramInit programInit{ vertexShader, pixelShader };
    graphics::ShaderPointer programShader = device->createProgram(programInit);

    graphics::GraphicsPipelineStateInit pipelineInit{
        programShader,
        vertexLayout,
        graphics::PrimitiveTopology::POINT,
        descriptorSetLayout
    };
    graphics::PipelineStatePointer pipeline = device->createGraphicsPipelineState(pipelineInit);

    return pipeline;
}
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    // Watch for command line argument to pick a different default file
    std::string cloudPointFile("../asset/pointcloud/AsianElephantPointcloud.ply");
    if (argc > 1) {
        cloudPointFile = std::string(argv[argc - 1]);
    }

    // Create the pico api
    core::ApiInit pico_init{ };
    auto result = core::api::create(pico_init);

    if (!result) {
        std::clog << "Pico api failed to create ?" << std::endl;
        return 1;
    }

    // First a device, aka the gpu api used by pico
    graphics::DeviceInit deviceInit {};
    auto gpuDevice = graphics::Device::createDevice(deviceInit);

    // Content creation

    core::vec4 viewportRect{ 0.0f, 0.0f, 1280.0f, 720.f };

    // Some content, why not a pointcloud ?
    auto pointCloud = document::PointCloud::createFromPLY(cloudPointFile);

    // Create a Mesh from the point cloud data

    // Declare the vertex format == PointCloud::Point
    graphics::AttribArray<2> attribs{ {{ graphics::AttribSemantic::A, graphics::AttribFormat::VEC3, 0 }, {graphics::AttribSemantic::C, graphics::AttribFormat::CVEC4, 0 }} };
    graphics::AttribBufferViewArray<1> bufferViews{ {0} };
    auto vertexFormat = graphics::StreamLayout::build(attribs, bufferViews);

    // Create the Mesh for real
    graphics::MeshPointer mesh = graphics::Mesh::createFromPointArray(vertexFormat, (uint32_t)pointCloud->_points.size(), (const uint8_t*)pointCloud->_points.data());
    
    // get the bounding sphere of the mesh
    core::vec4 sceneSphere = mesh->_bounds.toSphere();
    sceneSphere.w *= 2.0f;

    // Let's allocate buffer to hold the point cloud mesh
    graphics::BufferInit vertexBufferInit{};
    vertexBufferInit.usage = graphics::ResourceUsage::VERTEX_BUFFER;
    vertexBufferInit.hostVisible = true;
    vertexBufferInit.bufferSize = mesh->_vertexStream._buffers[0]->getSize();
    vertexBufferInit.vertexStride = mesh->_vertexStream._streamLayout.evalBufferViewByteStride(0);

    auto vertexBuffer = gpuDevice->createBuffer(vertexBufferInit);
    memcpy(vertexBuffer->_cpuMappedAddress, mesh->_vertexStream._buffers[0]->_data.data(), vertexBufferInit.bufferSize);

    auto numVertices = mesh->getNumVertices();

    // Let's describe the pipeline Descriptors layout
    graphics::DescriptorLayouts descriptorLayouts {
        { graphics::DescriptorType::UNIFORM_BUFFER, graphics::ShaderStage::VERTEX, 0, 1}
    };

    graphics::DescriptorSetLayoutInit descriptorSetLayoutInit{ descriptorLayouts };
    auto descriptorSetLayout = gpuDevice->createDescriptorSetLayout(descriptorSetLayoutInit);

    // And a Pipeline
    graphics::PipelineStatePointer pipeline = createPipelineState(gpuDevice, mesh->_vertexStream._streamLayout, descriptorSetLayout);


    // It s time to create a descriptorSet that matches the expected pipeline descriptor set
    // then we will assign a uniform buffer in it
    graphics::DescriptorSetInit descriptorSetInit{
        descriptorSetLayout
    };
    auto descriptorSet = gpuDevice->createDescriptorSet(descriptorSetInit);

    // Let s create a uniform buffer
    float aspectRatio = (viewportRect.z / viewportRect.w);
    struct CameraUB{
        core::vec3 _eye{ 0.5f, 1.0f, 2.04f };                float _focal { 0.056f };
        core::vec3 _right { 1.f, 0.f, 0.f};     float _sensorHeight { 0.056f };
        core::vec3 _up { 0.f, 1.f, 0.f };       float _aspectRatio {16.0f / 9.0f };
        core::vec3 _back { 0.f, 0.f, 1.f };    float _far { 10.f };
        core::vec4 _stuff{ 1.0f, 0.0f, 0.0f, 0.0f };
    };
    CameraUB cameraData;
    cameraData._aspectRatio = aspectRatio;

    // Far is a few times the scene size
    cameraData._far = sceneSphere.w * 10.0f;

    // look toward -z, y up and x right
    // move the eye the s size of the scene backward
    cameraData._eye = core::vec3(sceneSphere.xyz()) + cameraData._back * sceneSphere.w;

    graphics::BufferInit uboInit;
    uboInit.usage = graphics::ResourceUsage::UNIFORM_BUFFER;
    uboInit.bufferSize = sizeof(CameraUB);
    uboInit.hostVisible = true;
    auto cameraUBO = gpuDevice->createBuffer(uboInit);
    memcpy(cameraUBO->_cpuMappedAddress, &cameraData, sizeof(CameraUB));

    // Assign the UBO just created as the resource of the descriptorSet
    // auto descriptorObjects = descriptorSet->buildDescriptorObjects();

    graphics::DescriptorObject uboDescriptorObject;
    uboDescriptorObject._uniformBuffers.push_back( cameraUBO );
    graphics::DescriptorObjects descriptorObjects = {
        uboDescriptorObject,
    };
    gpuDevice->updateDescriptorSet(descriptorSet, descriptorObjects);
    
    // And now a render callback where we describe the rendering sequence
    graphics::RenderCallback renderCallback = [&](const graphics::CameraPointer& camera, const graphics::SwapchainPointer& swapchain, const graphics::DevicePointer& device, const graphics::BatchPointer& batch) {
        static float time = 0.0f;
        time += 1.0f / 60.0f;
        float intPart;
        float timeNorm = modf(time, &intPart);

        auto currentIndex = swapchain->currentIndex();

        if (cameraData._stuff.x) {
            cameraData._focal = 0.15f + 0.1f * sinf(0.1f * time);
        }

        memcpy(cameraUBO->_cpuMappedAddress, &cameraData, sizeof(CameraUB));

        batch->begin(currentIndex);

        batch->resourceBarrierTransition(
            graphics::ResourceBarrierFlag::NONE,
            graphics::ResourceState::PRESENT,
            graphics::ResourceState::RENDER_TARGET,
            swapchain, currentIndex, -1);

        core::vec4 clearColor(14.f/255.f, 14.f / 255.f, 14.f / 255.f, 1.f);
        batch->clear(swapchain, currentIndex, clearColor);

        batch->beginPass(swapchain, currentIndex);

        batch->bindPipeline(pipeline);

        batch->bindVertexBuffers(1, &vertexBuffer);

        batch->setViewport(viewportRect);
        batch->setScissor(viewportRect);

        batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet);

        batch->draw(numVertices, 0);

        batch->endPass();

        batch->resourceBarrierTransition(
            graphics::ResourceBarrierFlag::NONE,
            graphics::ResourceState::RENDER_TARGET,
            graphics::ResourceState::PRESENT,
            swapchain, currentIndex, -1);

        batch->end();

        device->executeBatch(batch);

        device->presentSwapchain(swapchain);
    };


    // Next, a renderer built on this device
    auto renderer = std::make_shared<graphics::Renderer>(gpuDevice, renderCallback);


    // Presentation creation

    // We need a window where to present, let s use the graphics::Window for convenience
    // This could be any window, we just need the os handle to create the swapchain next.
    auto windowHandler = new uix::WindowHandlerDelegate();
    uix::WindowInit windowInit { windowHandler };
    auto window = uix::Window::createWindow(windowInit);

    graphics::SwapchainInit swapchainInit { (HWND)window->nativeWindow(), (uint32_t) viewportRect.z, (uint32_t) viewportRect.w, true };
    auto swapchain = gpuDevice->createSwapchain(swapchainInit);

    //Now that we have created all the elements, 
    // We configure the windowHandler onPaint delegate of the window to do real rendering!
    windowHandler->_onPaintDelegate = ([swapchain, renderer](const uix::PaintEvent& e) {
        // Measuring framerate
        static uint64_t frameCounter = 0;
        static double elapsedSeconds = 0.0;
        static std::chrono::high_resolution_clock clock;
        static auto t0 = clock.now();

        frameCounter++;
        auto t1 = clock.now();
        auto deltaTime = t1 - t0;
        t0 = t1;

        elapsedSeconds += deltaTime.count() * 1e-9;
        if (elapsedSeconds > 1.0) {
            char buffer[500];
            auto fps = frameCounter / elapsedSeconds;
            sprintf_s(buffer, 500, "FPS: %f\n", fps);
            OutputDebugString(buffer);
            frameCounter = 0;
            elapsedSeconds = 0.0;
        }

        // Render!
        renderer->render(nullptr, swapchain);
    });

    // Let's react to keyboard events and change the camera views
    windowHandler->_onKeyboardDelegate = [&](const uix::KeyboardEvent& e) {
        if (e.state && e.key == uix::KEY_SPACE) {
            cameraData._stuff.x = (cameraData._stuff.x == 0.f ? 1.0f : 0.0f);
        }
        if (e.state && e.key == uix::KEY_3) {
            // look down
            cameraData._right = { 1.f, 0.f, 0.f };
            cameraData._up = { 0.f, 0.f, -1.f };
            cameraData._back = core::cross(cameraData._right, cameraData._up);
            cameraData._eye = core::vec3(sceneSphere.xyz()) + cameraData._back * sceneSphere.w;
        }
        if (e.state && e.key == uix::KEY_1) {
            // look side
            cameraData._right = { 1.f, 0.f, 0.f };
            cameraData._up = core::normalize({ 0.f, 1.f, 0.0f });
            cameraData._back = core::cross(cameraData._right, cameraData._up);
            cameraData._eye = core::vec3(sceneSphere.xyz()) + cameraData._back * sceneSphere.w;
        }
        if (e.state && e.key == uix::KEY_2) {
            // look lateral
            cameraData._right = { 0.f, 0.f, -1.f };
            cameraData._up = core::normalize({ 0.f, 1.f, 0.0f });
            cameraData._back = core::cross(cameraData._right, cameraData._up);
            cameraData._eye = core::vec3(sceneSphere.xyz()) + cameraData._back * sceneSphere.w;
        }
        if (e.state && e.key == uix::KEY_4) {
            // look 3/4 down
            cameraData._right = core::normalize({ 1.f, 0.f, -1.f });
            cameraData._up = core::normalize({ 0.f, 1.f, -1.f });
            cameraData._back = core::normalize(core::cross(cameraData._right, cameraData._up));
            cameraData._eye = core::vec3(sceneSphere.xyz()) + cameraData._back * sceneSphere.w;
        }
    };

    // Render Loop 
    bool keepOnGoing = true;
    while (keepOnGoing) {
        keepOnGoing = window->messagePump();
    }

    core::api::destroy();
    std::clog << "Pico api destroyed" << std::endl;

     return 0;
}
