#pragma once
#include <cstdint>
#include <array>
#include "../math/vec2f.h"
#include "../math/vec4f.h"

namespace core {

using position = math::vec2f;
using color = math::vec4f;

struct rect {
    position xy;
    position zw;
    rect() : xy{0,0}, zw{0,0} {}
    rect(float x, float y, float z, float w) : xy{x, y}, zw{z, w} {}
    rect(const position& xy, const position& zw) : xy(xy), zw(zw) {}
};

inline uint32_t pack_color_abgr(const color& c) {
    uint8_t r = static_cast<uint8_t>(c.x * 255.0f);
    uint8_t g = static_cast<uint8_t>(c.y * 255.0f);
    uint8_t b = static_cast<uint8_t>(c.z * 255.0f);
    uint8_t a = static_cast<uint8_t>(c.w * 255.0f);
    return (a << 24) | (b << 16) | (g << 8) | r;
}

struct vertex {
    float pos[3] = {0, 0, 0};
    union {
        uint8_t col[4];
        uint32_t col_u32;
    };
    float uv[2] = {0, 0};
    vertex() = default;
    vertex(float x, float y, float z, uint32_t color, float u, float v) {
        pos[0] = x; pos[1] = y; pos[2] = z;
        col_u32 = color;
        uv[0] = u; uv[1] = v;
    }
};

// add enums for draw modes, blend, etc. as needed

} // namespace core 