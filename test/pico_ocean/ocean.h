#pragma once
#include <stdint.h>
#include <complex>
#include <math.h>
#include <core/math/LinearAlgebra.h>
#include <graphics/drawables/PrimitiveDrawable.h>

namespace ocean {


    const float M_PI = acosf(-1.0f);
    using complexf = std::complex<float>;
    complexf expI(float theta) {
        return complexf(cosf(theta), sinf(theta));
    }

    void CooleyTukey(int N, int s, int q, int d, complexf* x) {
        int m = N / 2;
        if (N > 1) {
            for (int p = 0; p < m; p++) {
                complexf wp = expI(-p * 2 * M_PI / N);
                complexf a = x[q + p + 0];
                complexf b = x[q + p + m];
                x[q + p + 0] = a + b;
                x[q + p + m] = (a - b) * wp;
            }
            CooleyTukey(N / 2, 2 * s, q + 0, d + 0, x);
            CooleyTukey(N / 2, 2 * s, q + m, d + s, x);
        }
        else if (q > d) {
            complexf tmp = x[q];
            x[q] = x[d];
            x[d] = tmp;
        }
    }

    void FourierTransform(int N, complexf* x) {
        CooleyTukey(N, 1, 0, 0, x);
    }

    void InverseFourierTransform(int N, complexf* x) {
        for (int p = 0; p < N; p++)
            x[p] = std::conj(x[p]);
        FourierTransform(N, x);
        for (int p = 0; p < N; p++)
            x[p] = std::conj(x[p]);
    }

    void InverseFourierTransform2D(int N, complexf* c) {
        std::vector<complexf> rows(N);
        for (int y = 0; y < N; y++) {
            for (int x = 0; x < N; x++)
                rows[x] = c[y + x * N];
            InverseFourierTransform(N, rows.data());
            for (int x = 0; x < N; x++)
                c[y + x * N] = rows[x];
        }

        std::vector<complexf> columns(N);
        for (int x = 0; x < N; x++) {
            for (int y = 0; y < N; y++)
                columns[y] = c[y + x * N];
            InverseFourierTransform(N, columns.data());
            for (int y = 0; y < N; y++)
                c[y + x * N] = columns[y];
        }
    }

float Random() {
    static unsigned int randomState = 0x36dc64af;
    randomState = randomState ^ (randomState << 13u);
    randomState = randomState ^ (randomState >> 17u);
    randomState = randomState ^ (randomState << 5u);
    randomState *= 1685821657u;
    unsigned int intermediate = ((randomState & 0x007FFFFFu) | 0x3F800000u);
    return *((float*)(&intermediate)) - 1.0f;
}

float RandomGaussian() {
    float a = 0.f;
    for (int i = 0; i < 12; i++)
        a += Random();
    return a - 6.f;
}

int Floor(float x) {
    return x < 0 ? int(x) - 1 : int(x);
}

float Fract(float x) {
    return x - Floor(x);
}

float Mod(float x, float y) {
    return x - y * Floor(x / y);
}

template <typename T>
T Mix(T x, T y, float a) {
    return (1.f - a) * x + a * y;
}

template <typename T>
T BilinearInterpolation(float x, float y, int width, int height, T* values) {
    int xi1 = Mod(x, width);
    int yi1 = Mod(y, height);
    int xi2 = Mod(x + 1, width);
    int yi2 = Mod(y + 1, height);

    T topLeft = values[yi1 + xi1 * width];
    T topRight = values[yi1 + xi2 * width];
    T bottomLeft = values[yi2 + xi1 * width];
    T bottomRight = values[yi2 + xi2 * width];

    float xf = Fract(x);
    T top = Mix(topLeft, topRight, xf);
    T bottom = Mix(bottomLeft, bottomRight, xf);

    return Mix(top, bottom, Fract(y));
}
constexpr float PATCH_SIZE = 256.0f;
//constexpr float PATCH_SIZE = 50.0f;
constexpr float WIND_SPEED = 4.5f;
constexpr float GRAVITY = 9.81f;
constexpr float CHOPPINESS = 1.2f;
const core::vec2 WIND_DIRECTION = core::normalize(core::vec2(-1, -1));

class Ocean {
public:
    

    Ocean(uint32_t resolution) {
        _resolution = resolution;
        _spectrum0.resize(resolution * resolution);
        _spectrum.resize(resolution * resolution);
        _choppinesses.resize(resolution * resolution);
        _choppiness_displacements.resize(resolution * resolution);
        _heights.resize(resolution * resolution);
        _angular_speeds.resize(resolution * resolution);
    }
    

    template <typename T>
    T GetRemappedValues(float x, float y, T* values) {
    //    return 0.000048F * BilinearInterpolation(100.f * x, 100.f * y, _resolution, _resolution, values);
        return  BilinearInterpolation(x * PATCH_SIZE, y * PATCH_SIZE, _resolution, _resolution, values);
    }

    core::vec2 GetChoppinessDisplacement(float x, float y) {
        auto displacement = GetRemappedValues(x, y, _choppiness_displacements.data());
        return core::vec2(displacement.real(), displacement.imag()) * CHOPPINESS;
    }


    float GetHeight(float x, float y) {
        core::vec2 horizontalDisplacement = GetChoppinessDisplacement(x, y);
        x -= horizontalDisplacement.x;
        y -= horizontalDisplacement.y;
        return GetRemappedValues(x, y, _heights.data());
    }

    float PhillipsSpectrumCoefs(const core::vec2& k) {
        float L = WIND_SPEED * WIND_SPEED / GRAVITY;
        float l = L / 300.0f;

        float kDotw = core::dot(k, WIND_DIRECTION);
        float k2 = core::dot(k, k);
        if (k2 < 0.000001f)
            return 0;

        float phillips = expf(-1.f / (k2 * L * L)) / (k2 * k2 * k2) * (kDotw * kDotw);
        // La vague se déplace dans le sens contraire du vent
        if (kDotw < 0)
            phillips *= 0.01f;

        return phillips * expf(-k2 * l * l);
    }

    void GenerateSpectra() {
        for (int i = 0; i < _resolution; i++)
            for (int j = 0; j < _resolution; j++) {
                core::vec2 k = core::vec2(_resolution - 2 * i, _resolution - 2 * j) * (M_PI / PATCH_SIZE);
                float p = sqrtf(PhillipsSpectrumCoefs(k) / 2);

                int index = i * _resolution + j;
                _spectrum0[index] = std::complex(RandomGaussian() * p, RandomGaussian() * p);
                _angular_speeds[index] = sqrt(GRAVITY * core::length(k));
            }
    }

    void UpdateHeights(float t) {
        for (int x = 0; x < _resolution; x++) {
            for (int y = 0; y < _resolution; y++) {
                int i = y + x * _resolution;
                float wt = _angular_speeds[i] * t;
                std::complex h = _spectrum0[i];
                std::complex<float> h1;
                if (y == 0 && x == 0)
                    h1 = _spectrum0[_resolution * _resolution - 1];
                else if (y == 0)
                    h1 = _spectrum0[_resolution - 1 + (_resolution - x) * _resolution];
                else if (x == 0)
                    h1 = _spectrum0[_resolution - y + (_resolution - x - 1) * _resolution];
                else
                    h1 = _spectrum0[(_resolution - y) + (_resolution - x) * _resolution];

                core::vec2 k = core::normalize(core::vec2(_resolution * .5f - x, _resolution * .5f - y));
                std::complex spec = h * expI(wt) + std::conj(h1) * expI(-wt);
                _spectrum[i] = spec;
                _choppinesses[i] = std::complex(k.y, -k.x) * spec;
            }
        }

        InverseFourierTransform2D(_resolution, _spectrum.data());
        InverseFourierTransform2D(_resolution, _choppinesses.data());

        for (int i = 0; i < _resolution; i++)
            for (int j = 0; j < _resolution; j++) {
                float sign = ((i + j) % 2) ? -1 : 1;
                int index = i * _resolution + j;
                _heights[index] = sign * _spectrum[index].real();
                _choppiness_displacements[index] = sign * _choppinesses[index];
            }
    }

    std::vector<std::complex<float>> _spectrum0;
    std::vector<std::complex<float>> _spectrum;
    std::vector<std::complex<float>> _choppinesses;
    std::vector<std::complex<float>> _choppiness_displacements;
    std::vector<float> _heights;
    std::vector<float> _angular_speeds;
    uint32_t _resolution{ 0};
};
}


ocean::Ocean* locean;
std::vector<graphics::NodeID> prim_nodes;
graphics::ScenePointer lscene;

void generateSpectra(graphics::ScenePointer& scene, graphics::Node& root, graphics::Drawable& drawable, int res) {

    locean = new ocean::Ocean(res);
    locean->GenerateSpectra();

    lscene = scene;

    int width = res;
    float offset = ocean::PATCH_SIZE / (float) width;

    for (int i = 0; i < width * width; ++i) {
        float t = acos(-1.0f) * i / float(width * width);
        auto p = core::vec3(-offset * (i % width), 0.0f, offset * (i / width));

        p.y = locean->GetHeight(p.x, p.z);

        auto p_node = scene->createNode(
            core::translation_rotation(
                p,
                core::rotor3(core::vec3::X, core::vec3(cos(t), 0, sin(t)))
            ),
            root.id());
        auto p_item = scene->createItem(p_node, drawable);
        prim_nodes.push_back(p_node.id());
    }
}

void updateHeights(float t) {

    locean->UpdateHeights(t);


    for (auto prim_node : prim_nodes) {

        lscene->_nodes.editTransform(prim_node, [&](core::mat4x3& rts) -> bool {
            auto h = locean->GetHeight(rts._columns[3].x, rts._columns[3].z);
            rts._columns[3].y = h     * 1.0f;
            return true;
            });
    }

}