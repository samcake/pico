// VKBackend_Descriptor.cpp
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

#include <algorithm>

#ifdef PICO_VULKAN

using namespace graphics;

VkSurfaceKHR createSurface(VkInstance instance, const SwapchainInit& init)
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    createInfo.hinstance = GetModuleHandle(0);
    createInfo.hwnd = init.hWnd;

    VkSurfaceKHR surface = 0;
    VK_CHECK(vkCreateWin32SurfaceKHR(instance, &createInfo, 0, &surface));
    return surface;
#elif defined (VK_USE_PLATFORM_MACOS_MVK)
    VkSurfaceKHR surface = 0;
    VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));
    return surface;

    /*    VkMacOSSurfaceCreateInfoMVK createInfo = { VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK };
        auto nswnd = glfwGetCocoaWindow(window);
        createInfo.pView = nswnd;

        VkSurfaceKHR surface = 0;
        VK_CHECK(vkCreateMacOSSurfaceMVK(instance, &createInfo, 0, &surface));
        return surface;*/
#elif defined (VK_USE_PLATFORM_METAL_EXT)

#else
    //#error Unsupported platform
#endif

}


VkFormat getSwapchainFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, graphics::PixelFormat ExpectedFormat)
{
    uint32_t formatCount = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, 0));
    picoAssert(formatCount > 0);

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data()));

    auto vkExpectedFormat = graphics::VKBackend::Format[(int32_t)ExpectedFormat];
    auto vkExpectedFormat2 = graphics::VKBackend::FormatBGRA[(int32_t)ExpectedFormat];

    if (formatCount == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
        return vkExpectedFormat;

    for (uint32_t i = 0; i < formatCount; ++i)
    {
        if (formats[i].format == vkExpectedFormat || formats[i].format == vkExpectedFormat2)
        {
            return formats[i].format;
        }
    }

    

    return VK_FORMAT_UNDEFINED;
}

VkSwapchainKHR createSwapchain(VkDevice device, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR surfaceCaps, uint32_t familyIndex, VkFormat format, uint32_t width, uint32_t height, VkSwapchainKHR oldSwapchain)
{
    VkCompositeAlphaFlagBitsKHR surfaceComposite =
        (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
        ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
        : (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
        ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR
        : (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
        ? VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR
        : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

    VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    createInfo.surface = surface;
    createInfo.minImageCount = std::max(2u, surfaceCaps.minImageCount);
    createInfo.imageFormat = format;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent.width = std::max(width, surfaceCaps.minImageExtent.width);
    createInfo.imageExtent.height = std::max(height, surfaceCaps.minImageExtent.height);
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = &familyIndex;
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = surfaceComposite;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.oldSwapchain = oldSwapchain;

    VkSwapchainKHR swapchain = 0;
    VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, 0, &swapchain));

    return swapchain;
}


VkImageView VK::createImageView(VkDevice device, VkImage image, VkFormat format)
{
    VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView view = 0;
    VK_CHECK(vkCreateImageView(device, &createInfo, 0, &view));

    return view;
}

void createSwapchainBackend(VKSwapchainBackend& sw, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t familyIndex, VkFormat format, VkRenderPass renderPass, VkSwapchainKHR oldSwapchain = 0)
{
    VkSurfaceCapabilitiesKHR surfaceCaps;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));

    uint32_t width = surfaceCaps.currentExtent.width;
    uint32_t height = surfaceCaps.currentExtent.height;

    VkSwapchainKHR swapchain = createSwapchain(device, surface, surfaceCaps, familyIndex, format, width, height, oldSwapchain);
    picoAssert(swapchain);

    uint32_t imageCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, 0));

    std::vector<VkImage> images(imageCount);
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data()));

    std::vector<VkImageView> imageViews(imageCount);
    for (uint32_t i = 0; i < imageCount; ++i)
    {
        imageViews[i] = VK::createImageView(device, images[i], format);
        picoAssert(imageViews[i]);
    }

    std::vector<VkFramebuffer> framebuffers(imageCount);
    for (uint32_t i = 0; i < imageCount; ++i)
    {
        framebuffers[i] = VK::createFramebuffer(device, renderPass, imageViews[i], width, height);
        picoAssert(framebuffers[i]);
    }

    sw._swapchain = swapchain;

    sw._images = images;
    sw._imageViews = imageViews;
    sw._framebuffers = framebuffers;

    sw._init.width = width;
    sw._init.height = height;
    sw._imageCount = imageCount;
}

void destroySwapchain(VkDevice device, const VKSwapchainBackend& swapchain)
{
    for (uint32_t i = 0; i < swapchain._imageCount; ++i)
        vkDestroyFramebuffer(device, swapchain._framebuffers[i], 0);

    for (uint32_t i = 0; i < swapchain._imageCount; ++i)
        vkDestroyImageView(device, swapchain._imageViews[i], 0);

    vkDestroySwapchainKHR(device, swapchain._swapchain, 0);
}


void resizeSwapchainIfNecessary(VKSwapchainBackend& result, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t familyIndex, VkFormat format, VkRenderPass renderPass)
{
    VkSurfaceCapabilitiesKHR surfaceCaps;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));

    uint32_t newWidth = surfaceCaps.currentExtent.width;
    uint32_t newHeight = surfaceCaps.currentExtent.height;

    if (result._init.width == newWidth && result._init.height == newHeight)
        return;

    auto old = result;

    createSwapchainBackend(result, physicalDevice, device, surface, familyIndex, format, renderPass, old._swapchain);

    VK_CHECK(vkDeviceWaitIdle(device));

    destroySwapchain(device, old);
}


VKSwapchainBackend::VKSwapchainBackend() : Swapchain() {

}

VKSwapchainBackend::~VKSwapchainBackend() {
}

SwapchainPointer VKBackend::createSwapchain(const SwapchainInit& init) {
    auto sw = new VKSwapchainBackend();

    sw->_init = init;

    sw->_surface = createSurface(_instance, init);

    VkBool32 presentSupported = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice, _familyIndex, sw->_surface, &presentSupported));
    picoAssert(presentSupported);

    sw->_swapchainFormat = getSwapchainFormat(_physicalDevice, sw->_surface, init.colorBufferFormat);

    sw->_renderPass = VK::createRenderPass(_device, sw->_swapchainFormat);

    createSwapchainBackend(*sw, _physicalDevice, _device, sw->_surface, _familyIndex, sw->_swapchainFormat, sw->_renderPass);

    return SwapchainPointer(sw);
}


void VKBackend::resizeSwapchain(const SwapchainPointer& swapchain, uint32_t width, uint32_t height) {

    auto sw = static_cast<VKSwapchainBackend*>(swapchain.get());

    // Update the client size.
    if (sw->_init.width != width || sw->_init.height != height)
    {
        sw->_init.width = std::max(8U, width);
        sw->_init.height = std::max(8U, height);

        flush(); // make sure the swapchain is not in use anymore
        resizeSwapchainIfNecessary(*sw, _physicalDevice, _device, sw->_surface, _familyIndex, sw->_swapchainFormat, sw->_renderPass);
    }
}


#endif
