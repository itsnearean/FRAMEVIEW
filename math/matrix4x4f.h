#pragma once
#include <array>
#include "vec4f.h"
#include <cstddef>

namespace math {

struct matrix4x4f {
    std::array<vec4f, 4> matrix = {};
    matrix4x4f() = default;
    matrix4x4f(const vec4f& row1, const vec4f& row2, const vec4f& row3, const vec4f& row4)
        : matrix{row1, row2, row3, row4} {}
    const vec4f& operator[](size_t idx) const { return matrix[idx]; }
    vec4f& operator[](size_t idx) { return matrix[idx]; }
};

} // namespace math 