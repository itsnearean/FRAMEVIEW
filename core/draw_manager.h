#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include "draw_types.h"

namespace resources { struct font; }

namespace core {

class draw_buffer;

class draw_manager {
public:
    virtual ~draw_manager() = default;

    // buffer management
    virtual size_t register_buffer(size_t init_priority) = 0;
    virtual size_t register_child_buffer(size_t parent, size_t priority) = 0;
    virtual void update_child_priority(size_t child_idx, size_t new_priority) = 0;
    virtual void update_buffer_priority(size_t buffer_idx, size_t new_priority) = 0;
    virtual void remove_buffer(size_t idx) = 0;
    virtual draw_buffer* get_buffer(size_t idx) = 0;
    virtual void swap_buffers(size_t idx) = 0;
    
    // font management
    virtual resources::font* add_font(const char* file, float size, bool italic, bool bold, int rasterizer_flags) = 0;
    virtual void remove_font(const resources::font* font_ptr) = 0;
    
    // matrix operations
    virtual void update_matrix_translate(size_t buffer, const position& xy_translate, size_t cmd_idx) = 0;
    
    // initialization
    virtual void init() = 0;
};

using draw_manager_ptr = std::shared_ptr<draw_manager>;

} // namespace core 