#pragma once
#include <cmath>
#include <cstddef>

namespace math {

struct vec3f {
    float x, y, z;
    vec3f() : x(0.f), y(0.f), z(0.f) {}
    vec3f(float x, float y, float z) : x(x), y(y), z(z) {}
    float dot(const vec3f& o) const { return (x * o.x) + (y * o.y) + (z * o.z); }
    float length_sqr() const { return dot(*this); }
    float length() const { return std::sqrt(length_sqr()); }
    float reciprocal_length() const { return 1.f / length(); }
    vec3f& normalize() { return (*this *= reciprocal_length()); }
    vec3f normalized() const { auto r{*this}; return r.normalize(); }
    bool operator==(const vec3f& o) const { return (x == o.x && y == o.y && z == o.z); }
    vec3f& operator+=(float v) { x += v; y += v; z += v; return *this; }
    vec3f& operator-=(float v) { x -= v; y -= v; z -= v; return *this; }
    vec3f& operator/=(float v) { x /= v; y /= v; z /= v; return *this; }
    vec3f& operator*=(float v) { x *= v; y *= v; z *= v; return *this; }
    vec3f& operator+=(const vec3f& o) { x += o.x; y += o.y; z += o.z; return *this; }
    vec3f& operator-=(const vec3f& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    vec3f& operator/=(const vec3f& o) { x /= o.x; y /= o.y; z /= o.z; return *this; }
    vec3f& operator*=(const vec3f& o) { x *= o.x; y *= o.y; z *= o.z; return *this; }
    vec3f operator+(float v) const { auto r{*this}; r.x += v; r.y += v; r.z += v; return r; }
    vec3f operator-(float v) const { auto r{*this}; r.x -= v; r.y -= v; r.z -= v; return r; }
    vec3f operator*(float v) const { auto r{*this}; r.x *= v; r.y *= v; r.z *= v; return r; }
    vec3f operator/(float v) const { auto r{*this}; r.x /= v; r.y /= v; r.z /= v; return r; }
    vec3f operator+(const vec3f& o) const { auto r{*this}; r.x += o.x; r.y += o.y; r.z += o.z; return r; }
    vec3f operator-(const vec3f& o) const { auto r{*this}; r.x -= o.x; r.y -= o.y; r.z -= o.z; return r; }
    vec3f operator*(const vec3f& o) const { auto r{*this}; r.x *= o.x; r.y *= o.y; r.z *= o.z; return r; }
    vec3f operator/(const vec3f& o) const { auto r{*this}; r.x /= o.x; r.y /= o.y; r.z /= o.z; return r; }
    const float& operator[](size_t idx) const { return reinterpret_cast<const float*>(this)[idx]; }
    float& operator[](size_t idx) { return reinterpret_cast<float*>(this)[idx]; }
};

inline vec3f operator*(float v, const vec3f& o) { auto r{o}; r.x *= v; r.y *= v; r.z *= v; return r; }

} // namespace math 