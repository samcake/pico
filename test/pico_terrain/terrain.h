#pragma once
#include <stdint.h>
#include <math.h>
#include <core/math/Math3D.h>
#include <graphics/drawables/PrimitiveDraw.h>
#include <graphics/drawables/HeightmapDraw.h>



namespace terrain {

    class Terrain;
    using TerrainPointer = std::shared_ptr<Terrain>;

    class Terrain {
    public:
        static TerrainPointer createFromDEM(const std::string& filename);


        Terrain(uint32_t resolution, float spacing) {
            _spacing = spacing;
            _resolution = resolution;
            _heights.resize(resolution * resolution);
        }
    

        float GetHeight(float x, float y) {
        }

        std::vector<float> _heights;
        int32_t _resolution{ 0};
        float _spacing { 1.0f };
    };
}


