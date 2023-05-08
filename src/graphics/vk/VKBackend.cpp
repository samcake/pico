// D3D12Backend.cpp
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
#include "VKBackend.h"

#include <chrono>

#ifdef PICO_VULKAN

using namespace graphics;

void VKBackend::VkCheck(const char* file, int line, const char* functionName, VkResult result)
{
    if (result != VK_SUCCESS) {
        picoLog("VKBackend FAILED !!!");
        switch (result)
        {
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            ::core::Log::_log(file, line, functionName, "VK_ERROR_OUT_OF_HOST_MEMORY");
            break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            ::core::Log::_log(file, line, functionName, "VK_ERROR_OUT_OF_DEVICE_MEMORY");
            break;
        case VK_ERROR_INITIALIZATION_FAILED:
            ::core::Log::_log(file, line, functionName, "VK_ERROR_INITIALIZATION_FAILED");
            break;
        case VK_ERROR_LAYER_NOT_PRESENT:
            ::core::Log::_log(file, line, functionName, "VK_ERROR_LAYER_NOT_PRESENT");
            break;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            ::core::Log::_log(file, line, functionName, "VK_ERROR_EXTENSION_NOT_PRESENT");
            break;
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            ::core::Log::_log(file, line, functionName, "VK_ERROR_INCOMPATIBLE_DRIVER");
            break;
        }
    }
}

const VkFormat VKBackend::Format[] = {
    VK_FORMAT_UNDEFINED,
    VK_FORMAT_R32G32B32A32_UINT, // Typeless
    VK_FORMAT_R32G32B32A32_SFLOAT,
    VK_FORMAT_R32G32B32A32_UINT,
    VK_FORMAT_R32G32B32A32_SINT,
    VK_FORMAT_R32G32B32_UINT, //VK_FORMAT_R32G32B32_TYPELESS,
    VK_FORMAT_R32G32B32_SFLOAT,
    VK_FORMAT_R32G32B32_UINT,
    VK_FORMAT_R32G32B32_SINT,
    VK_FORMAT_R16G16B16A16_UINT, //VK_FORMAT_R16G16B16A16_TYPELESS,
    VK_FORMAT_R16G16B16A16_SFLOAT,
    VK_FORMAT_R16G16B16A16_UNORM,
    VK_FORMAT_R16G16B16A16_UINT,
    VK_FORMAT_R16G16B16A16_SNORM,
    VK_FORMAT_R16G16B16A16_SINT,
    VK_FORMAT_R32G32_UINT, //VK_FORMAT_R32G32_TYPELESS,
    VK_FORMAT_R32G32_SFLOAT,
    VK_FORMAT_R32G32_UINT,
    VK_FORMAT_R32G32_SINT,
    VK_FORMAT_R8G8B8A8_UNORM, //VK_FORMAT_R32G8X24_TYPELESS,
    VK_FORMAT_R8G8B8A8_UNORM, //VK_FORMAT_D32_FLOAT_S8X24_UINT,
    VK_FORMAT_R8G8B8A8_UNORM, //VK_FORMAT_R32_FLOAT_X8X24_TYPELESS,
    VK_FORMAT_R8G8B8A8_UNORM, //VK_FORMAT_X32_TYPELESS_G8X24_UINT,
    VK_FORMAT_R8G8B8A8_UNORM, //VK_FORMAT_R10G10B10A2_TYPELESS,
    VK_FORMAT_R8G8B8A8_UNORM, //VK_FORMAT_R10G10B10A2_UNORM,
    VK_FORMAT_R8G8B8A8_UNORM, //VK_FORMAT_R10G10B10A2_UINT,
    VK_FORMAT_R8G8B8A8_UNORM, //VK_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
    VK_FORMAT_R8G8B8A8_UNORM, //VK_FORMAT_R11G11B10_FLOAT,
    VK_FORMAT_R8G8B8A8_UNORM, //VK_FORMAT_R8G8B8A8_TYPELESS,
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_R8G8B8A8_SRGB, //VK_FORMAT_R8G8B8A8_UNORM_SRGB,
    VK_FORMAT_R8G8B8A8_UINT,
    VK_FORMAT_R8G8B8A8_SNORM,
    VK_FORMAT_R8G8B8A8_SINT,
    VK_FORMAT_R16G16_UINT, //VK_FORMAT_R16G16_TYPELESS,
    VK_FORMAT_R16G16_SFLOAT,
    VK_FORMAT_R16G16_UNORM,
    VK_FORMAT_R16G16_UINT,
    VK_FORMAT_R16G16_SNORM,
    VK_FORMAT_R16G16_SINT,
    VK_FORMAT_R32_UINT, //VK_FORMAT_R32_TYPELESS,
    VK_FORMAT_D32_SFLOAT,
    VK_FORMAT_R32_SFLOAT,
    VK_FORMAT_R32_UINT,
    VK_FORMAT_R32_SINT,
    VK_FORMAT_D24_UNORM_S8_UINT, //VK_FORMAT_R24G8_TYPELESS,
    VK_FORMAT_D24_UNORM_S8_UINT,
    VK_FORMAT_D24_UNORM_S8_UINT, //VK_FORMAT_R24_UNORM_X8_TYPELESS,
    VK_FORMAT_D24_UNORM_S8_UINT, //VK_FORMAT_X24_TYPELESS_G8_UINT,
    VK_FORMAT_R8G8_UINT, //VK_FORMAT_R8G8_TYPELESS,
    VK_FORMAT_R8G8_UNORM,
    VK_FORMAT_R8G8_UINT,
    VK_FORMAT_R8G8_SNORM,
    VK_FORMAT_R8G8_SINT,
    VK_FORMAT_R16_UINT, //VK_FORMAT_R16_TYPELESS,
    VK_FORMAT_R16_UINT, //VK_FORMAT_R16_FLOAT,
    VK_FORMAT_D16_UNORM,
    VK_FORMAT_R16_UNORM,
    VK_FORMAT_R16_UINT,
    VK_FORMAT_R16_SNORM,
    VK_FORMAT_R16_SINT,
    VK_FORMAT_R8_UINT, //VK_FORMAT_R8_TYPELESS,
    VK_FORMAT_R8_UNORM,
    VK_FORMAT_R8_UINT,
    VK_FORMAT_R8_SNORM,
    VK_FORMAT_R8_SINT,
};

const VkFormat VKBackend::FormatBGRA[] = {
    VK_FORMAT_UNDEFINED,
    VK_FORMAT_R32G32B32A32_UINT, // Typeless
    VK_FORMAT_R32G32B32A32_SFLOAT,
    VK_FORMAT_R32G32B32A32_UINT,
    VK_FORMAT_R32G32B32A32_SINT,
    VK_FORMAT_R32G32B32_UINT, //VK_FORMAT_R32G32B32_TYPELESS,
    VK_FORMAT_R32G32B32_SFLOAT,
    VK_FORMAT_R32G32B32_UINT,
    VK_FORMAT_R32G32B32_SINT,
    VK_FORMAT_R16G16B16A16_UINT, //VK_FORMAT_R16G16B16A16_TYPELESS,
    VK_FORMAT_R16G16B16A16_SFLOAT,
    VK_FORMAT_R16G16B16A16_UNORM,
    VK_FORMAT_R16G16B16A16_UINT,
    VK_FORMAT_R16G16B16A16_SNORM,
    VK_FORMAT_R16G16B16A16_SINT,
    VK_FORMAT_R32G32_UINT, //VK_FORMAT_R32G32_TYPELESS,
    VK_FORMAT_R32G32_SFLOAT,
    VK_FORMAT_R32G32_UINT,
    VK_FORMAT_R32G32_SINT,
    VK_FORMAT_B8G8R8A8_UNORM, //VK_FORMAT_R32G8X24_TYPELESS,
    VK_FORMAT_B8G8R8A8_UNORM, //VK_FORMAT_D32_FLOAT_S8X24_UINT,
    VK_FORMAT_B8G8R8A8_UNORM, //VK_FORMAT_R32_FLOAT_X8X24_TYPELESS,
    VK_FORMAT_B8G8R8A8_UNORM, //VK_FORMAT_X32_TYPELESS_G8X24_UINT,
    VK_FORMAT_B8G8R8A8_UNORM, //VK_FORMAT_R10G10B10A2_TYPELESS,
    VK_FORMAT_B8G8R8A8_UNORM, //VK_FORMAT_R10G10B10A2_UNORM,
    VK_FORMAT_B8G8R8A8_UNORM, //VK_FORMAT_R10G10B10A2_UINT,
    VK_FORMAT_B8G8R8A8_UNORM, //VK_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
    VK_FORMAT_B8G8R8A8_UNORM, //VK_FORMAT_R11G11B10_FLOAT,
    VK_FORMAT_B8G8R8A8_UNORM, //VK_FORMAT_R8G8B8A8_TYPELESS,
    VK_FORMAT_B8G8R8A8_UNORM,
    VK_FORMAT_B8G8R8A8_SRGB, //VK_FORMAT_R8G8B8A8_UNORM_SRGB,
    VK_FORMAT_B8G8R8A8_UINT,
    VK_FORMAT_B8G8R8A8_SNORM,
    VK_FORMAT_R8G8B8A8_SINT,
    VK_FORMAT_R16G16_UINT, //VK_FORMAT_R16G16_TYPELESS,
    VK_FORMAT_R16G16_SFLOAT,
    VK_FORMAT_R16G16_UNORM,
    VK_FORMAT_R16G16_UINT,
    VK_FORMAT_R16G16_SNORM,
    VK_FORMAT_R16G16_SINT,
    VK_FORMAT_R32_UINT, //VK_FORMAT_R32_TYPELESS,
    VK_FORMAT_D32_SFLOAT,
    VK_FORMAT_R32_SFLOAT,
    VK_FORMAT_R32_UINT,
    VK_FORMAT_R32_SINT,
    VK_FORMAT_D24_UNORM_S8_UINT, //VK_FORMAT_R24G8_TYPELESS,
    VK_FORMAT_D24_UNORM_S8_UINT,
    VK_FORMAT_D24_UNORM_S8_UINT, //VK_FORMAT_R24_UNORM_X8_TYPELESS,
    VK_FORMAT_D24_UNORM_S8_UINT, //VK_FORMAT_X24_TYPELESS_G8_UINT,
    VK_FORMAT_R8G8_UINT, //VK_FORMAT_R8G8_TYPELESS,
    VK_FORMAT_R8G8_UNORM,
    VK_FORMAT_R8G8_UINT,
    VK_FORMAT_R8G8_SNORM,
    VK_FORMAT_R8G8_SINT,
    VK_FORMAT_R16_UINT, //VK_FORMAT_R16_TYPELESS,
    VK_FORMAT_R16_UINT, //VK_FORMAT_R16_FLOAT,
    VK_FORMAT_D16_UNORM,
    VK_FORMAT_R16_UNORM,
    VK_FORMAT_R16_UINT,
    VK_FORMAT_R16_SNORM,
    VK_FORMAT_R16_SINT,
    VK_FORMAT_R8_UINT, //VK_FORMAT_R8_TYPELESS,
    VK_FORMAT_R8_UNORM,
    VK_FORMAT_R8_UINT,
    VK_FORMAT_R8_SNORM,
    VK_FORMAT_R8_SINT,
};
/*
const D3D12_RESOURCE_STATES D3D12BatchBackend::ResourceStates[] = {
    D3D12_RESOURCE_STATE_COMMON,
    D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
    D3D12_RESOURCE_STATE_INDEX_BUFFER,
    D3D12_RESOURCE_STATE_RENDER_TARGET,
    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
    D3D12_RESOURCE_STATE_DEPTH_WRITE,
    D3D12_RESOURCE_STATE_DEPTH_READ,
    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    D3D12_RESOURCE_STATE_STREAM_OUT,
    D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
    D3D12_RESOURCE_STATE_COPY_DEST,
    D3D12_RESOURCE_STATE_COPY_SOURCE,
    D3D12_RESOURCE_STATE_RESOLVE_DEST,
    D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
    D3D12_RESOURCE_STATE_GENERIC_READ,
  //  D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
   // D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE,
    D3D12_RESOURCE_STATE_PRESENT,
    D3D12_RESOURCE_STATE_PREDICATION,
 //   D3D12_RESOURCE_STATE_VIDEO_DECODE_READ,
  //  D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE,
  //  D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ,
   // D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE,
  //  D3D12_RESOURCE_STATE_VIDEO_ENCODE_READ,
  //  D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE
};

const D3D12_RESOURCE_BARRIER_FLAGS D3D12BatchBackend::ResourceBarrieFlags[] = {
    D3D12_RESOURCE_BARRIER_FLAG_NONE,
    D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY,
    D3D12_RESOURCE_BARRIER_FLAG_END_ONLY,
};

const D3D12_PRIMITIVE_TOPOLOGY_TYPE D3D12BatchBackend::PrimitiveTopologyTypes[] = {
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
};

const D3D12_PRIMITIVE_TOPOLOGY D3D12BatchBackend::PrimitiveTopologies[] = {
    D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
    D3D_PRIMITIVE_TOPOLOGY_LINELIST,
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
};
*/

VkInstance createInstance() {
    uint32_t instance_layer_count;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));
    picoLogf("\n{} layers found!", instance_layer_count);
    if (instance_layer_count > 0) {
        std::unique_ptr<VkLayerProperties[]> instance_layers(new VkLayerProperties[instance_layer_count]);
        VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers.get()));
        for (int i = 0; i < instance_layer_count; ++i) {
            picoLog(instance_layers[i].layerName);
        }
    }

    std::vector<const char*> layers = {
        "VK_LAYER_KHRONOS_validation",
    };
    std::vector<const char*> extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#ifdef VK_USE_PLATFORM_MACOS_MVK
        VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
#endif
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME
    };
    uint32_t instance_extension_count;
    vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, nullptr);
    picoLogf("\n{} extensions found!", instance_extension_count);
    if (instance_extension_count > 0) {
        std::unique_ptr<VkExtensionProperties[]> instance_extensions(new VkExtensionProperties[instance_extension_count]);
        VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, instance_extensions.get()));
        for (int i = 0; i < instance_extension_count; ++i) {
            picoLog( instance_extensions[i].extensionName );
        }
    }

    VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    applicationInfo.pApplicationName = "VIKI";
    applicationInfo.applicationVersion = 0;
    applicationInfo.pEngineName = "VIKI";
    applicationInfo.engineVersion = 0;
    applicationInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    info.pApplicationInfo = &applicationInfo;
    info.enabledLayerCount = layers.size();
    info.ppEnabledLayerNames = layers.data();
    info.enabledExtensionCount = extensions.size();
    info.ppEnabledExtensionNames = extensions.data();

    VkInstance instance = 0;
    VK_CHECK(vkCreateInstance(&info, NULL, &instance));

    return instance;
}


VkBool32 debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    const char* type =
        (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        ? "ERROR"
        : (flags & (VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT))
        ? "WARNING"
        : "INFO";

    picoLog(std::format("{}: {}", type, pMessage));

    return VK_FALSE;
}


VkDebugReportCallbackEXT registerDebugCallback(VkInstance instance)
{
    VkDebugReportCallbackCreateInfoEXT createInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
    createInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;
    createInfo.pfnCallback = debugReportCallback;

    VkDebugReportCallbackEXT callback = 0;
    VK_CHECK(vkCreateDebugReportCallbackEXT(instance, &createInfo, 0, &callback));

    return callback;
}

uint32_t getGraphicsFamilyIndex(VkPhysicalDevice physicalDevice)
{
    uint32_t queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, 0);

    std::vector<VkQueueFamilyProperties> queues(queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queues.data());

    for (uint32_t i = 0; i < queueCount; ++i)
        if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            return i;

    return VK_QUEUE_FAMILY_IGNORED;
}

bool supportsPresentation(VkPhysicalDevice physicalDevice, uint32_t familyIndex)
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    return vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, familyIndex);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
    return true;
    //return vkGetPhysicalMacDevicePresentationSupport(physicalDevice, familyIndex);
#else
    return true;
#endif
}

VkPhysicalDevice pickPhysicalDevice(VkPhysicalDevice* physicalDevices, uint32_t physicalDeviceCount)
{
    VkPhysicalDevice discrete = 0;
    VkPhysicalDevice fallback = 0;

    for (uint32_t i = 0; i < physicalDeviceCount; ++i)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(physicalDevices[i], &props);

        printf("GPU%d: %s\n", i, props.deviceName);

        uint32_t familyIndex = getGraphicsFamilyIndex(physicalDevices[i]);
        if (familyIndex == VK_QUEUE_FAMILY_IGNORED)
            continue;

        if (!supportsPresentation(physicalDevices[i], familyIndex))
            continue;

        if (!discrete && props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            discrete = physicalDevices[i];
        }

        if (!fallback)
        {
            fallback = physicalDevices[i];
        }
    }

    VkPhysicalDevice result = discrete ? discrete : fallback;

    if (result)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(result, &props);

        picoLogf("Selected GPU {}", props.deviceName);
    } else
    {
        picoLog("ERROR: No GPUs found");
    }

    return result;
}


VkDevice createDevice(VkInstance instance, VkPhysicalDevice physicalDevice, uint32_t* familyIndex)
{
    *familyIndex = 0; // SHORTCUT: this needs to be computed from queue properties // TODO: this produces a validation error

    float queuePriorities[] = { 1.0f };

    VkDeviceQueueCreateInfo queueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueInfo.queueFamilyIndex = *familyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = queuePriorities;

    const char* extensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkPhysicalDeviceFeatures features = {};
    features.vertexPipelineStoresAndAtomics = true; // TODO: we aren't using this yet? is this a bug in glslang or in validation layers or do I just not understand the spec?

    VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueInfo;

    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);
    createInfo.pEnabledFeatures = &features;

    VkDevice device = 0;
    VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, 0, &device));

    return device;
}


VkSemaphore createSemaphore(VkDevice device)
{
    VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    VkSemaphore semaphore = 0;
    VK_CHECK(vkCreateSemaphore(device, &createInfo, 0, &semaphore));

    return semaphore;
}
VkFence createFence(VkDevice device)
{
    VkFenceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkFence fence = 0;
    VK_CHECK(vkCreateFence(device, &createInfo, 0, &fence));

    return fence;
}

VkCommandPool createCommandPool(VkDevice device, uint32_t familyIndex)
{
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = familyIndex;
    
    VkCommandPool commandPool = 0;
    VK_CHECK(vkCreateCommandPool(device, &createInfo, 0, &commandPool));

    return commandPool;
}

/*



ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device)
{
    ComPtr<ID3D12Fence> fence;

    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

    return fence;
}

HANDLE CreateEventHandle()
{
    HANDLE fenceEvent;

    fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    // assert(fenceEvent && "Failed to create fence event.");

    return fenceEvent;
}

uint64_t Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence,
    uint64_t& fenceValue)
{
    uint64_t fenceValueForSignal = ++fenceValue;
    ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValueForSignal));

    return fenceValueForSignal;
}

void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent,
    std::chrono::milliseconds duration = std::chrono::milliseconds::max())
{
    if (fence->GetCompletedValue() < fenceValue)
    {
        ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
        ::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
    }
}

void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence,
    uint64_t& fenceValue, HANDLE fenceEvent)
{
    uint64_t fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
    WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
}
*/


VKBackend::VKBackend() {
    VkResult result = volkInitialize();
    if (result != VK_SUCCESS)
        return;

    _instance = createInstance();
    volkLoadInstance(_instance);

    VkDebugReportCallbackEXT debugCallback = registerDebugCallback(_instance);

    VkPhysicalDevice physicalDevices[16];
    uint32_t physicalDeviceCount = sizeof(physicalDevices) / sizeof(physicalDevices[0]);
    VK_CHECK(vkEnumeratePhysicalDevices(_instance, &physicalDeviceCount, physicalDevices));

    _physicalDevice = pickPhysicalDevice(physicalDevices, physicalDeviceCount);
    picoAssert(_physicalDevice);

    _familyIndex = getGraphicsFamilyIndex(_physicalDevice);
    picoAssert(_familyIndex != VK_QUEUE_FAMILY_IGNORED);

    _device = createDevice(_instance, _physicalDevice, &_familyIndex);
    picoAssert(_device);

    vkGetDeviceQueue(_device, _familyIndex, 0, &_queue);
    picoAssert(_queue);

    for (int i = 0; i < VKBackend::CHAIN_NUM_FRAMES; ++i)
    {
        _acquireSemaphores[i] = createSemaphore(_device);
        picoAssert(_acquireSemaphores[i]);

        _releaseSemaphores[i] = createSemaphore(_device);
        picoAssert(_releaseSemaphores[i]);

        _inFlightFences[i] = createFence(_device);
        picoAssert(_inFlightFences[i]);
    }

    _commandPool = createCommandPool(_device, _familyIndex);
    picoAssert(_commandPool);

   /*

    _commandQueue = CreateCommandQueue(_device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    uint64_t frequency;
    _commandQueue->GetTimestampFrequency(&frequency);
    _commandQueueTimestampFrequency = (double)frequency;

    _fence = CreateFence(_device);
    _fenceEvent = CreateEventHandle();

    // Allocate a global descriptor heap referenced form this backend
    DescriptorHeapInit descriptorHeapInit {
        10000,
        1000
    };
    _descriptorHeap = this->createDescriptorHeap(descriptorHeapInit);

    // Allocate a default empty pipeline layout
    _emptyRootDescriptorLayout = this->createRootDescriptorLayout( {} );
    */
    }

VKBackend::~VKBackend() {

}


void VKBackend::acquireSwapchain(const SwapchainPointer& swapchain) {
    auto sw = static_cast<VKSwapchainBackend*>(swapchain.get());

    auto currentFrame = swapchain->currentIndex();

    // Wait for the in flight work to be done before steing to the next frame
    VK_CHECK(vkWaitForFences(_device, 1, &_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(_device, 1, &_inFlightFences[currentFrame]));

    uint32_t imageIndex = 0;
    VK_CHECK(vkAcquireNextImageKHR(_device, sw->_swapchain, UINT64_MAX,
        _acquireSemaphores[currentFrame], VK_NULL_HANDLE,
        &imageIndex));

    if (imageIndex != currentFrame) {
        picoLog(std::format("Error: CurrentFrame {} : ImageIndex {}", currentFrame, imageIndex));
    }

}


void VKBackend::executeBatch(const BatchPointer& batch) {
    auto bat = static_cast<VKBatchBackend*>(batch.get());

    auto currentFrame = bat->_currentBackBufferIndex;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = { _acquireSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] =
    { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &bat->_commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = { _releaseSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VK_CHECK(vkQueueSubmit(_queue, 1, &submitInfo, _inFlightFences[currentFrame]));
}

void VKBackend::presentSwapchain(const SwapchainPointer& swapchain) {
    auto sw = static_cast<VKSwapchainBackend*>(swapchain.get());

    uint32_t currentFrame = swapchain->currentIndex();


/*    // UINT syncInterval = g_VSync ? 1 : 0;
    UINT syncInterval = 1;
    // UINT presentFlags = g_TearingSupported && !g_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
    UINT presentFlags = 0;
    ThrowIfFailed(sw->_swapchain->Present(syncInterval, presentFlags));

    _frameFenceValues[sw->currentIndex()] = Signal(_commandQueue, _fence, _fenceValue);

    sw->_currentIndex = sw->_swapchain->GetCurrentBackBufferIndex();

    WaitForFenceValue(_fence, _frameFenceValues[sw->_currentIndex], _fenceEvent);
*/

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;

    VkSemaphore waitSemaphores[] = { _releaseSemaphores[currentFrame] };
    presentInfo.pWaitSemaphores = waitSemaphores;

    VkSwapchainKHR swapChains[] = { sw->_swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &currentFrame;
    
    presentInfo.pResults = nullptr;

    VK_CHECK(vkQueuePresentKHR(_queue, &presentInfo));



    // moving on to the next frame!
    swapchain->_currentIndex = (currentFrame + 1) % VKBackend::CHAIN_NUM_FRAMES;

}

void VKBackend::flush() {
  //  Flush(_commandQueue, _fence, _fenceValue, _fenceEvent);
}

/*
void VKBackend::garbageCollect(const ComPtr<ID3D12DeviceChild>& child) {
    _garbageObjects.push_back(child);
}

void VKBackend::flushGarbage() {

}
*/


#endif
