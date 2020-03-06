// PointcloudDrawable
// 
#pragma once
#ifndef POINTCLOUD_DRAWABLE_H_
#define POINTCLOUD_DRAWABLE_H_

// #include "stdafx.h"
#include "../Forward.h"
#include <memory>


namespace document {
    class PointCloud;
    using PointCloudPointer = std::shared_ptr<PointCloud>;
}
namespace pico
{
    class DrawcallObject;
    using DrawcallObjectPointer = std::shared_ptr<DrawcallObject>;
    class Device;
    using DevicePointer = std::shared_ptr<Device>;
    class Camera;
    using CameraPointer = std::shared_ptr<Camera>;


    /*
    const pico::DrawcallObjectPointer& getDrawable(const PointcloudDrawable& x) {
        return x.getDrawable();
    }*/
    class VISUALIZATION_API PointCloudDrawable {
    public:
        PointCloudDrawable();
        ~PointCloudDrawable();
        
        pico::DrawcallObjectPointer allocateDocumentDrawcallObject(const pico::DevicePointer& device, const pico::CameraPointer& camera, const document::PointCloudPointer& pointcloud);
        pico::DrawcallObjectPointer getDrawable() const;

#pragma warning(push)
#pragma warning(disable: 4251) // class 'std::shared_ptr<pico::DrawcallObject>' needs to have dll-interface to be used by clients of class 'pico::PointcloudDrawable'
        pico::DrawcallObjectPointer _drawcall;     
#pragma warning(pop)
    };


} // !namespace pico

#endif // POINTCLOUD_DRAWABLE_H_