#pragma once
#include <cmath>
#include <cstddef>

namespace math {

struct vec2f {
    float x, y;
    vec2f() : x(0.f), y(0.f) {}
    vec2f(float x, float y) : x(x), y(y) {}
    float dot(const vec2f& o) const { return (x * o.x) + (y * o.y); }
    float dot() const { return dot(*this); }
    float length_sqr() const { return dot(); }
    float length() const { return std::sqrt(length_sqr()); }
    float reciprocal_length() const { return 1.f / length(); }
    vec2f& normalize() { return (*this *= reciprocal_length()); }
    vec2f normalized() const { auto r{*this}; return r.normalize(); }
    bool operator==(const vec2f& o) const { return (x == o.x && y == o.y); }
    vec2f& operator+=(float v) { x += v; y += v; return *this; }
    vec2f& operator-=(float v) { x -= v; y -= v; return *this; }
    vec2f& operator/=(float v) { x /= v; y /= v; return *this; }
    vec2f& operator*=(float v) { x *= v; y *= v; return *this; }
    vec2f& operator+=(const vec2f& o) { x += o.x; y += o.y; return *this; }
    vec2f& operator-=(const vec2f& o) { x -= o.x; y -= o.y; return *this; }
    vec2f& operator/=(const vec2f& o) { x /= o.x; y /= o.y; return *this; }
    vec2f& operator*=(const vec2f& o) { x *= o.x; y *= o.y; return *this; }
    vec2f operator+(float v) const { auto r{*this}; r.x += v; r.y += v; return r; }
    vec2f operator-(float v) const { auto r{*this}; r.x -= v; r.y -= v; return r; }
    vec2f operator*(float v) const { auto r{*this}; r.x *= v; r.y *= v; return r; }
    vec2f operator/(float v) const { auto r{*this}; r.x /= v; r.y /= v; return r; }
    vec2f operator+(const vec2f& o) const { auto r{*this}; r.x += o.x; r.y += o.y; return r; }
    vec2f operator-(const vec2f& o) const { auto r{*this}; r.x -= o.x; r.y -= o.y; return r; }
    vec2f operator*(const vec2f& o) const { auto r{*this}; r.x *= o.x; r.y *= o.y; return r; }
    vec2f operator/(const vec2f& o) const { auto r{*this}; r.x /= o.x; r.y /= o.y; return r; }
    const float& operator[](size_t idx) const { return reinterpret_cast<const float*>(this)[idx]; }
    float& operator[](size_t idx) { return reinterpret_cast<float*>(this)[idx]; }
};

inline vec2f operator*(float v, const vec2f& o) { auto r{o}; r.x *= v; r.y *= v; return r; }

} // namespace math 