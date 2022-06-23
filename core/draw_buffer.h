#pragma once
#include <vector>
#include <memory>
#include <functional>
#include "draw_types.h"
#include <string>
#include <stack>
#include "../resources/font.h"
#include "../resources/texture.h"

namespace resources { struct font; }

namespace core {

struct draw_command {
    uint32_t elem_count = 0;
    rect clip_rect;
    rect circle_outer_clip;
    bool circle_scissor = false;
    uint32_t tex_id = 0;
    bool font_texture = false;
    bool native_texture = false;
    uint8_t blur_strength = 0;
    uint8_t blur_pass_count = 0;
    color key_color;
    std::function<void(const draw_command*)> callback;
    // matrix transform could be added here
};

class draw_buffer {
public:
    std::vector<vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<draw_command> cmds;
    
    // methods from inspiration
    std::pair<uint32_t, uint32_t> vtx_idx_count() const {
        return { static_cast<uint32_t>(vertices.size()), static_cast<uint32_t>(indices.size()) };
    }
    
    void prim_reserve(uint32_t idx_count, uint32_t vtx_count) {
        vertices.reserve(vertices.size() + vtx_count);
        indices.reserve(indices.size() + idx_count);
    }
    
    void prim_rect(const position& a, const position& c, const color& col);
    void prim_rect_filled(const position& a, const position& c, const color& col);
    void prim_rect_multi_color(const position& a, const position& c, 
                              const color& col_top_left, const color& col_top_right,
                              const color& col_bot_left, const color& col_bot_right);
    
    void set_blur(uint8_t strength, uint8_t passes = 1);
    void set_key_color(const color& col);
    
    void line_strip(const std::vector<position>& points, uint32_t color, float thickness);
    void line(const position& a, const position& b, uint32_t color_a, uint32_t color_b, float thickness = 1.0f);
    void poly_line(const std::vector<position>& points, uint32_t color, float thickness = 1.0f, bool closed = false);
    void triangle_filled(const position& a, const position& b, const position& c, uint32_t color_a, uint32_t color_b, uint32_t color_c);
    void circle_filled(const position& center, float radius, uint32_t color_inner, uint32_t color_outer, int segments = 32);
    void prim_rect_uv(const position& a, const position& c, const position& uv_a, const position& uv_c, uint32_t color);
    void n_gon(const position& center, float radius, int sides, uint32_t color);
    void text(const std::string& str, const position& pos, uint32_t color); // stub
    
    void push_font(std::shared_ptr<resources::font> font);
    void pop_font();
    std::shared_ptr<resources::font> current_font() const;

    // Texture stack methods
    void push_texture(resources::tex texture);
    void pop_texture();
    resources::tex current_texture() const;
    
    // Helper method for automatic texture push/pop
    template<typename Func>
    void with_texture(resources::tex texture, Func&& func) {
        push_texture(texture);
        func();
        pop_texture();
    }
    
    // Clear texture stack
    void clear_texture_stack();

    // add more as needed
private:
    std::vector<std::shared_ptr<resources::font>> font_stack_;
    std::vector<resources::tex> texture_stack_;
};

using draw_buffer_ptr = std::shared_ptr<draw_buffer>;

} // namespace core 