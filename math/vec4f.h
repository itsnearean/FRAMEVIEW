#pragma once
#include "vec2f.h"
#include <cstddef>

namespace math {

struct vec4f {
    union {
        struct { float x, y, z, w; };
        struct { vec2f xy, zw; };
    };
    vec4f() : x(0.f), y(0.f), z(0.f), w(0.f) {}
    vec4f(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    vec4f(const vec2f& xy, const vec2f& zw) : x(xy.x), y(xy.y), z(zw.x), w(zw.y) {}
    float operator[](size_t idx) const { return (&x)[idx]; }
    float& operator[](size_t idx) { return (&x)[idx]; }
    bool operator==(const vec4f& o) const { return (xy == o.xy && zw == o.zw); }
};

} // namespace math 