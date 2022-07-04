#pragma once
#include "types.h"

namespace core {
class buffer;

namespace util {
    void rectangle_filled_rounded(buffer* buf, const position& top_left, const position& bot_right, float radius, const color& col_top_left, const color& col_top_right, const color& col_bot_left, const color& col_bot_right, uint8_t flags);
    void check_mark(buffer* buf, position top_left, float width, const color& col);
    //TODO: implement and add to this list.
}

} // namespace core 