#pragma once
#include <cstdint>

namespace math {

struct color_rgba {
    uint8_t _r, _g, _b, _a;
    color_rgba(int r, int g, int b, int a = 255u) : _r(r), _g(g), _b(b), _a(a) {}
    color_rgba(uint32_t val) {
        _r = (val >> 24);
        _g = (val >> 16) & 0xFF;
        _b = (val >> 8) & 0xFF;
        _a = val & 0xFF;
    }
    uint32_t as_argb() const { return (_a << 24) | (_r << 16) | (_g << 8) | _b; }
    uint32_t as_abgr() const { return (_a << 24) | (_b << 16) | (_g << 8) | _r; }
    operator uint32_t() const { return (_r << 24) | (_g << 16) | (_b << 8) | _a; }
    uint8_t r() const { return _r; }
    uint8_t g() const { return _g; }
    uint8_t b() const { return _b; }
    uint8_t a() const { return _a; }
    static color_rgba white() { return color_rgba{255, 255, 255}; }
    static color_rgba black() { return color_rgba{0, 0, 0}; }
    static color_rgba red()   { return color_rgba{235, 64, 52}; }
    static color_rgba green() { return color_rgba{52, 217, 77}; }
    static color_rgba blue()  { return color_rgba{34, 108, 199}; }
};

} // namespace math 