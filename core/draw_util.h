#pragma once
#include "draw_types.h"

namespace core {
class draw_buffer;

namespace util {
    void rectangle_filled_rounded(draw_buffer* buf, const position& top_left, const position& bot_right, float radius, const color& col_top_left, const color& col_top_right, const color& col_bot_left, const color& col_bot_right, uint8_t flags);
    void check_mark(draw_buffer* buf, position top_left, float width, const color& col);
    // add more as needed
}

} // namespace core 