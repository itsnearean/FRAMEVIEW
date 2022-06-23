#pragma once
#include "constants.h"

namespace math {

template<typename T>
constexpr T rad2deg(T rad) {
    return rad * T(180) / PI<T>;
}

template<typename T>
constexpr T deg2rad(T deg) {
    return deg * PI<T> / T(180);
}

} // namespace math 