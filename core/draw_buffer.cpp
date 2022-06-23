#include "draw_buffer.h"
#include <cmath>
#include <string>
#include "../resources/font.h"
#include "../utils/logger.h"
#include <stack>

namespace core {

void draw_buffer::prim_rect(const position& a, const position& c, const color& col) {
    // add rectangle outline vertices and indices
    const uint32_t base_idx = static_cast<uint32_t>(vertices.size());
    
    // add 4 vertices for rectangle outline
    vertices.push_back(core::vertex(a.x, a.y, 0, pack_color_abgr(col), 0, 0)); // top-left
    vertices.push_back(core::vertex(c.x, a.y, 0, pack_color_abgr(col), 0, 0)); // top-right
    vertices.push_back(core::vertex(c.x, c.y, 0, pack_color_abgr(col), 0, 0)); // bottom-right
    vertices.push_back(core::vertex(a.x, c.y, 0, pack_color_abgr(col), 0, 0)); // bottom-left
    
    // add indices for line strip (outline)
    indices.push_back(base_idx);
    indices.push_back(base_idx + 1);
    indices.push_back(base_idx + 2);
    indices.push_back(base_idx + 3);
    indices.push_back(base_idx); // close the rectangle
}

void draw_buffer::prim_rect_filled(const position& a, const position& c, const color& col) {
    // add filled rectangle vertices and indices
    const uint32_t base_idx = static_cast<uint32_t>(vertices.size());
    
    // add 4 vertices for filled rectangle
    vertices.push_back(core::vertex(a.x, a.y, 0, pack_color_abgr(col), 0, 0)); // top-left
    vertices.push_back(core::vertex(c.x, a.y, 0, pack_color_abgr(col), 0, 0)); // top-right
    vertices.push_back(core::vertex(c.x, c.y, 0, pack_color_abgr(col), 0, 0)); // bottom-right
    vertices.push_back(core::vertex(a.x, c.y, 0, pack_color_abgr(col), 0, 0)); // bottom-left
    
    // add indices for 2 triangles (filled rectangle)
    indices.push_back(base_idx);
    indices.push_back(base_idx + 1);
    indices.push_back(base_idx + 2);
    indices.push_back(base_idx);
    indices.push_back(base_idx + 2);
    indices.push_back(base_idx + 3);
}

void draw_buffer::prim_rect_multi_color(const position& a, const position& c, 
                                       const color& col_top_left, const color& col_top_right,
                                       const color& col_bot_left, const color& col_bot_right) {
    size_t vtx_before = vertices.size(), idx_before = indices.size();
    // add multi-color rectangle vertices and indices
    const uint32_t base_idx = static_cast<uint32_t>(vertices.size());
    
    // add 4 vertices with different colors
    vertices.push_back(core::vertex(a.x, a.y, 0, pack_color_abgr(col_top_left), 0, 0));     // top-left
    vertices.push_back(core::vertex(c.x, a.y, 0, pack_color_abgr(col_top_right), 0, 0));    // top-right
    vertices.push_back(core::vertex(c.x, c.y, 0, pack_color_abgr(col_bot_right), 0, 0));    // bottom-right
    vertices.push_back(core::vertex(a.x, c.y, 0, pack_color_abgr(col_bot_left), 0, 0));     // bottom-left
    
    // add indices for 2 triangles (filled rectangle)
    indices.push_back(base_idx);
    indices.push_back(base_idx + 1);
    indices.push_back(base_idx + 2);
    indices.push_back(base_idx);
    indices.push_back(base_idx + 2);
    indices.push_back(base_idx + 3);
}

void draw_buffer::line(const position& a, const position& b, uint32_t color_a, uint32_t color_b, float thickness) {
    size_t vtx_before = vertices.size(), idx_before = indices.size();
    // simple line as a thin quad (rectangle)
    // for now, just use two points and a degenerate quad
    float dx = b.x - a.x, dy = b.y - a.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len == 0) return;
    float nx = -dy / len * (thickness * 0.5f);
    float ny = dx / len * (thickness * 0.5f);
    position p0 = {a.x + nx, a.y + ny};
    position p1 = {a.x - nx, a.y - ny};
    position p2 = {b.x - nx, b.y - ny};
    position p3 = {b.x + nx, b.y + ny};
    uint32_t base = static_cast<uint32_t>(vertices.size());
    vertices.push_back(vertex(p0.x, p0.y, 0, color_a, 0, 0));
    vertices.push_back(vertex(p1.x, p1.y, 0, color_a, 0, 0));
    vertices.push_back(vertex(p2.x, p2.y, 0, color_b, 0, 0));
    vertices.push_back(vertex(p3.x, p3.y, 0, color_b, 0, 0));
    indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
    indices.push_back(base + 0); indices.push_back(base + 2); indices.push_back(base + 3);
}

void draw_buffer::line_strip(const std::vector<position>& points, uint32_t color, float thickness) {
    size_t vtx_before = vertices.size(), idx_before = indices.size();
    if (points.size() < 2) return;
    for (size_t i = 0; i < points.size() - 1; ++i) {
        line(points[i], points[i + 1], color, color, thickness);
    }
}

void draw_buffer::poly_line(const std::vector<position>& points, uint32_t color, float thickness, bool closed) {
    size_t vtx_before = vertices.size(), idx_before = indices.size();
    if (points.size() < 2) return;
    for (size_t i = 0; i < points.size() - 1; ++i) {
        line(points[i], points[i + 1], color, color, thickness);
    }
    if (closed) {
        line(points.back(), points.front(), color, color, thickness);
    }
}

void draw_buffer::triangle_filled(const position& a, const position& b, const position& c, uint32_t color_a, uint32_t color_b, uint32_t color_c) {
    size_t vtx_before = vertices.size(), idx_before = indices.size();
    uint32_t base = static_cast<uint32_t>(vertices.size());
    vertices.push_back(vertex(a.x, a.y, 0, color_a, 0, 0));
    vertices.push_back(vertex(b.x, b.y, 0, color_b, 0, 0));
    vertices.push_back(vertex(c.x, c.y, 0, color_c, 0, 0));
    indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
}

void draw_buffer::circle_filled(const position& center, float radius, uint32_t color_inner, uint32_t color_outer, int segments) {
    size_t vtx_before = vertices.size(), idx_before = indices.size();
    if (segments < 3) segments = 3;
    uint32_t base = static_cast<uint32_t>(vertices.size());
    vertices.push_back(vertex(center.x, center.y, 0, color_inner, 0.5f, 0.5f));
    for (int i = 0; i <= segments; ++i) {
        float angle = float(i) / float(segments) * 2.0f * 3.14159265f;
        float x = center.x + std::cos(angle) * radius;
        float y = center.y + std::sin(angle) * radius;
        float u = 0.5f + 0.5f * std::cos(angle);
        float v = 0.5f + 0.5f * std::sin(angle);
        vertices.push_back(vertex(x, y, 0, color_outer, u, v));
    }
    for (int i = 1; i <= segments; ++i) {
        indices.push_back(base);
        indices.push_back(base + i);
        indices.push_back(base + i + 1);
    }
}

void draw_buffer::prim_rect_uv(const position& a, const position& c, const position& uv_a, const position& uv_c, uint32_t color) {
    size_t vtx_before = vertices.size(), idx_before = indices.size();
    // add rectangle outline vertices and indices
    uint32_t base = static_cast<uint32_t>(vertices.size());
    position b = {c.x, a.y};
    position d = {a.x, c.y};
    position uv_b = {uv_c.x, uv_a.y};
    position uv_d = {uv_a.x, uv_c.y};
    vertices.push_back(vertex(a.x, a.y, 0, color, uv_a.x, uv_a.y));
    vertices.push_back(vertex(b.x, b.y, 0, color, uv_b.x, uv_b.y));
    vertices.push_back(vertex(c.x, c.y, 0, color, uv_c.x, uv_c.y));
    vertices.push_back(vertex(d.x, d.y, 0, color, uv_d.x, uv_d.y));
    indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
    indices.push_back(base + 0); indices.push_back(base + 2); indices.push_back(base + 3);
}

void draw_buffer::n_gon(const position& center, float radius, int sides, uint32_t color) {
    size_t vtx_before = vertices.size(), idx_before = indices.size();
    if (sides < 3) sides = 3;
    uint32_t base = static_cast<uint32_t>(vertices.size());
    vertices.push_back(vertex(center.x, center.y, 0, color, 0.5f, 0.5f));
    for (int i = 0; i <= sides; ++i) {
        float angle = float(i) / float(sides) * 2.0f * 3.14159265f;
        float x = center.x + std::cos(angle) * radius;
        float y = center.y + std::sin(angle) * radius;
        float u = 0.5f + 0.5f * std::cos(angle);
        float v = 0.5f + 0.5f * std::sin(angle);
        vertices.push_back(vertex(x, y, 0, color, u, v));
    }
    for (int i = 1; i <= sides; ++i) {
        indices.push_back(base);
        indices.push_back(base + i);
        indices.push_back(base + i + 1);
    }
}

void draw_buffer::text(const std::string& str, const position& pos, uint32_t color) {
    auto font = current_font();
    if (!font) {
        // TODO: handle this better, ideally by using a default font as fallback
        utils::log_warn("No font set for text rendering");
        return;
    }
    
    // get font metrics for proper baseline positioning
    const auto& metrics = font->metrics();
    // baseline position: pos.y is the baseline, no need to add ascender
    float baseline_y = pos.y;
    float x = pos.x;
        
    // process string as UTF-8
    const char* ptr = str.c_str();
    const char* end = ptr + str.length();
    
    while (ptr < end) {
        uint32_t codepoint = 0;
        int bytes_read = 0;
        
        // decode UTF-8 character
        if ((*ptr & 0x80) == 0) {
            // single byte
            codepoint = static_cast<uint8_t>(*ptr);
            bytes_read = 1;
        } else if ((*ptr & 0xE0) == 0xC0) {
            // two bytes
            if (ptr + 1 < end) {
                codepoint = ((*ptr & 0x1F) << 6) | (*(ptr + 1) & 0x3F);
                bytes_read = 2;
            }
        } else if ((*ptr & 0xF0) == 0xE0) {
            // three bytes
            if (ptr + 2 < end) {
                codepoint = ((*ptr & 0x0F) << 12) | ((*(ptr + 1) & 0x3F) << 6) | (*(ptr + 2) & 0x3F);
                bytes_read = 3;
            }
        } else if ((*ptr & 0xF8) == 0xF0) {
            // four bytes
            if (ptr + 3 < end) {
                codepoint = ((*ptr & 0x07) << 18) | ((*(ptr + 1) & 0x3F) << 12) | ((*(ptr + 2) & 0x3F) << 6) | (*(ptr + 3) & 0x3F);
                bytes_read = 4;
            }
        }
        
        if (bytes_read == 0) {
            // invalid UTF-8, skip one byte
            ptr++;
            continue;
        }
        
        // ensure glyph is loaded (dynamic paging)
        if (!font->ensure_glyph(codepoint)) {
            utils::log_error("Failed to load glyph for codepoint U+%04X", codepoint);
            ptr += bytes_read;
            continue;
        }
        
        const auto& glyph = font->glyphs().at(codepoint);
        
        // calculate glyph position relative to baseline
        float x0 = x + glyph.bearingX;
        float y0 = baseline_y - glyph.bearingY; // bearingY is distance from baseline to top
        float x1 = x0 + glyph.width;
        float y1 = y0 + glyph.height;
         
        float u0 = glyph.u0, v0 = glyph.v0, u1 = glyph.u1, v1 = glyph.v1;
        prim_rect_uv({x0, y0}, {x1, y1}, {u0, v0}, {u1, v1}, color);
        
        // advance to next character position
        x += glyph.advance;
        
        ptr += bytes_read;
    }
    
}

void draw_buffer::set_blur(uint8_t strength, uint8_t passes) {
    if (!cmds.empty()) {
        cmds.back().blur_strength = strength;
        cmds.back().blur_pass_count = passes;
    }
}

void draw_buffer::set_key_color(const color& col) {
    if (!cmds.empty()) {
        cmds.back().key_color = col;
    }
}

void draw_buffer::push_font(std::shared_ptr<resources::font> font) {
    font_stack_.push_back(font);
}
void draw_buffer::pop_font() {
    if (!font_stack_.empty()) font_stack_.pop_back();
}
std::shared_ptr<resources::font> draw_buffer::current_font() const {
    if (!font_stack_.empty()) return font_stack_.back();
    return nullptr;
}

// Texture stack methods
void draw_buffer::push_texture(resources::tex texture) {
    texture_stack_.push_back(texture);
}
void draw_buffer::pop_texture() {
    if (!texture_stack_.empty()) texture_stack_.pop_back();
}
resources::tex draw_buffer::current_texture() const {
    if (!texture_stack_.empty()) return texture_stack_.back();
    return nullptr;
}

void draw_buffer::clear_texture_stack() {
    texture_stack_.clear();
}

} // namespace core 