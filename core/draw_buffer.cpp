#include "draw_buffer.h"
#include <cmath>
#include <string>
#include "../resources/font.h"
#include "../utils/logger.h"
#include <stack>

namespace core {

void draw_buffer::prim_rect(const position& a, const position& c, const color& col) {
    // collect vertices and indices for this rectangle outline
    std::vector<vertex> rect_vertices;
    std::vector<uint32_t> rect_indices;
    
    // add 4 vertices for rectangle outline
    rect_vertices.push_back(core::vertex(a.x, a.y, 0, pack_color_abgr(col), 0, 0)); // top-left
    rect_vertices.push_back(core::vertex(c.x, a.y, 0, pack_color_abgr(col), 0, 0)); // top-right
    rect_vertices.push_back(core::vertex(c.x, c.y, 0, pack_color_abgr(col), 0, 0)); // bottom-right
    rect_vertices.push_back(core::vertex(a.x, c.y, 0, pack_color_abgr(col), 0, 0)); // bottom-left
    
    // add indices for line strip (outline)
    rect_indices.push_back(0);
    rect_indices.push_back(1);
    rect_indices.push_back(2);
    rect_indices.push_back(3);
    rect_indices.push_back(0); // close the rectangle
    
    // use unified geometry system
    add_geometry_color_only(rect_vertices, rect_indices);
}

void draw_buffer::prim_rect_filled(const position& a, const position& c, const color& col) {
    // collect vertices and indices for this color-only quad
    std::vector<vertex> quad_vertices;
    std::vector<uint32_t> quad_indices;
    
    // add 4 vertices for filled rectangle
    quad_vertices.push_back(core::vertex(a.x, a.y, 0, pack_color_abgr(col), 0, 0)); // top-left
    quad_vertices.push_back(core::vertex(c.x, a.y, 0, pack_color_abgr(col), 0, 0)); // top-right
    quad_vertices.push_back(core::vertex(c.x, c.y, 0, pack_color_abgr(col), 0, 0)); // bottom-right
    quad_vertices.push_back(core::vertex(a.x, c.y, 0, pack_color_abgr(col), 0, 0)); // bottom-left
    
    // add indices for 2 triangles (filled rectangle)
    quad_indices.push_back(0);
    quad_indices.push_back(1);
    quad_indices.push_back(2);
    quad_indices.push_back(0);
    quad_indices.push_back(2);
    quad_indices.push_back(3);
    
    // use unified geometry system
    add_geometry_color_only(quad_vertices, quad_indices);
}

void draw_buffer::prim_rect_multi_color(const position& a, const position& c, 
                                       const color& col_top_left, const color& col_top_right,
                                       const color& col_bot_left, const color& col_bot_right) {
    // collect vertices and indices for this multi-color quad
    std::vector<vertex> quad_vertices;
    std::vector<uint32_t> quad_indices;
    
    // add 4 vertices with different colors
    quad_vertices.push_back(core::vertex(a.x, a.y, 0, pack_color_abgr(col_top_left), 0, 0));     // top-left
    quad_vertices.push_back(core::vertex(c.x, a.y, 0, pack_color_abgr(col_top_right), 0, 0));    // top-right
    quad_vertices.push_back(core::vertex(c.x, c.y, 0, pack_color_abgr(col_bot_right), 0, 0));    // bottom-right
    quad_vertices.push_back(core::vertex(a.x, c.y, 0, pack_color_abgr(col_bot_left), 0, 0));     // bottom-left
    
    // add indices for 2 triangles (filled rectangle)
    quad_indices.push_back(0);
    quad_indices.push_back(1);
    quad_indices.push_back(2);
    quad_indices.push_back(0);
    quad_indices.push_back(2);
    quad_indices.push_back(3);
    
    // use unified geometry system
    add_geometry_color_only(quad_vertices, quad_indices);
}

void draw_buffer::line(const position& a, const position& b, uint32_t color_a, uint32_t color_b, float thickness) {
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
    
    // collect vertices and indices for this line quad
    std::vector<vertex> line_vertices;
    std::vector<uint32_t> line_indices;
    
    line_vertices.push_back(vertex(p0.x, p0.y, 0, color_a, 0, 0));
    line_vertices.push_back(vertex(p1.x, p1.y, 0, color_a, 0, 0));
    line_vertices.push_back(vertex(p2.x, p2.y, 0, color_b, 0, 0));
    line_vertices.push_back(vertex(p3.x, p3.y, 0, color_b, 0, 0));
    
    line_indices.push_back(0); 
    line_indices.push_back(1); 
    line_indices.push_back(2);
    line_indices.push_back(0); 
    line_indices.push_back(2); 
    line_indices.push_back(3);
    
    // use unified geometry system
    add_geometry_color_only(line_vertices, line_indices);
}

void draw_buffer::line_strip(const std::vector<position>& points, uint32_t color, float thickness) {
    if (points.size() < 2) return;
    
    // collect all line segments
    std::vector<vertex> strip_vertices;
    std::vector<uint32_t> strip_indices;
    
    for (size_t i = 0; i < points.size() - 1; ++i) {
        const position& a = points[i];
        const position& b = points[i + 1];
        
        float dx = b.x - a.x, dy = b.y - a.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len == 0) continue;
        
        float nx = -dy / len * (thickness * 0.5f);
        float ny = dx / len * (thickness * 0.5f);
        position p0 = {a.x + nx, a.y + ny};
        position p1 = {a.x - nx, a.y - ny};
        position p2 = {b.x - nx, b.y - ny};
        position p3 = {b.x + nx, b.y + ny};
        
        uint32_t base = static_cast<uint32_t>(strip_vertices.size());
        strip_vertices.push_back(vertex(p0.x, p0.y, 0, color, 0, 0));
        strip_vertices.push_back(vertex(p1.x, p1.y, 0, color, 0, 0));
        strip_vertices.push_back(vertex(p2.x, p2.y, 0, color, 0, 0));
        strip_vertices.push_back(vertex(p3.x, p3.y, 0, color, 0, 0));
        
        strip_indices.push_back(base + 0); 
        strip_indices.push_back(base + 1); 
        strip_indices.push_back(base + 2);
        strip_indices.push_back(base + 0); 
        strip_indices.push_back(base + 2); 
        strip_indices.push_back(base + 3);
    }
    
    // use unified geometry system
    if (!strip_vertices.empty() && !strip_indices.empty()) {
        add_geometry_color_only(strip_vertices, strip_indices);
    }
}

void draw_buffer::poly_line(const std::vector<position>& points, uint32_t color, float thickness, bool closed) {
    if (points.size() < 2) return;
    
    // collect all line segments
    std::vector<vertex> poly_vertices;
    std::vector<uint32_t> poly_indices;
    
    for (size_t i = 0; i < points.size() - 1; ++i) {
        const position& a = points[i];
        const position& b = points[i + 1];
        
        float dx = b.x - a.x, dy = b.y - a.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len == 0) continue;
        
        float nx = -dy / len * (thickness * 0.5f);
        float ny = dx / len * (thickness * 0.5f);
        position p0 = {a.x + nx, a.y + ny};
        position p1 = {a.x - nx, a.y - ny};
        position p2 = {b.x - nx, b.y - ny};
        position p3 = {b.x + nx, b.y + ny};
        
        uint32_t base = static_cast<uint32_t>(poly_vertices.size());
        poly_vertices.push_back(vertex(p0.x, p0.y, 0, color, 0, 0));
        poly_vertices.push_back(vertex(p1.x, p1.y, 0, color, 0, 0));
        poly_vertices.push_back(vertex(p2.x, p2.y, 0, color, 0, 0));
        poly_vertices.push_back(vertex(p3.x, p3.y, 0, color, 0, 0));
        
        poly_indices.push_back(base + 0); 
        poly_indices.push_back(base + 1); 
        poly_indices.push_back(base + 2);
        poly_indices.push_back(base + 0); 
        poly_indices.push_back(base + 2); 
        poly_indices.push_back(base + 3);
    }
    
    // close the polyline if requested
    if (closed && points.size() >= 3) {
        const position& a = points.back();
        const position& b = points.front();
        
        float dx = b.x - a.x, dy = b.y - a.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0) {
            float nx = -dy / len * (thickness * 0.5f);
            float ny = dx / len * (thickness * 0.5f);
            position p0 = {a.x + nx, a.y + ny};
            position p1 = {a.x - nx, a.y - ny};
            position p2 = {b.x - nx, b.y - ny};
            position p3 = {b.x + nx, b.y + ny};
            
            uint32_t base = static_cast<uint32_t>(poly_vertices.size());
            poly_vertices.push_back(vertex(p0.x, p0.y, 0, color, 0, 0));
            poly_vertices.push_back(vertex(p1.x, p1.y, 0, color, 0, 0));
            poly_vertices.push_back(vertex(p2.x, p2.y, 0, color, 0, 0));
            poly_vertices.push_back(vertex(p3.x, p3.y, 0, color, 0, 0));
            
            poly_indices.push_back(base + 0); 
            poly_indices.push_back(base + 1); 
            poly_indices.push_back(base + 2);
            poly_indices.push_back(base + 0); 
            poly_indices.push_back(base + 2); 
            poly_indices.push_back(base + 3);
        }
    }
    
    // use unified geometry system
    if (!poly_vertices.empty() && !poly_indices.empty()) {
        add_geometry_color_only(poly_vertices, poly_indices);
    }
}

void draw_buffer::triangle_filled(const position& a, const position& b, const position& c, uint32_t color_a, uint32_t color_b, uint32_t color_c) {
    // collect vertices and indices for this triangle
    std::vector<vertex> tri_vertices;
    std::vector<uint32_t> tri_indices;
    
    // add 3 vertices with different colors
    tri_vertices.push_back(core::vertex(a.x, a.y, 0, color_a, 0, 0)); // vertex a
    tri_vertices.push_back(core::vertex(b.x, b.y, 0, color_b, 0, 0)); // vertex b
    tri_vertices.push_back(core::vertex(c.x, c.y, 0, color_c, 0, 0)); // vertex c
    
    // add indices for 1 triangle
    tri_indices.push_back(0);
    tri_indices.push_back(1);
    tri_indices.push_back(2);
    
    // use unified geometry system
    add_geometry_color_only(tri_vertices, tri_indices);
}

void draw_buffer::circle_filled(const position& center, float radius, uint32_t color_inner, uint32_t color_outer, int segments) {
    if (segments < 3) segments = 3;
    
    // collect vertices and indices for this circle
    std::vector<vertex> circle_vertices;
    std::vector<uint32_t> circle_indices;
    
    // add center vertex
    circle_vertices.push_back(core::vertex(center.x, center.y, 0, color_inner, 0, 0));
    
    // add perimeter vertices
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * 3.14159f * float(i) / float(segments);
        float x = center.x + radius * std::cos(angle);
        float y = center.y + radius * std::sin(angle);
        circle_vertices.push_back(core::vertex(x, y, 0, color_outer, 0, 0));
    }
    
    // add indices for triangles (fan from center)
    for (int i = 1; i < segments; ++i) {
        circle_indices.push_back(0);           // center
        circle_indices.push_back(i);           // current perimeter vertex
        circle_indices.push_back(i + 1);       // next perimeter vertex
    }
    
    // close the circle
    circle_indices.push_back(0);
    circle_indices.push_back(segments);
    circle_indices.push_back(1);
    
    // use unified geometry system
    add_geometry_color_only(circle_vertices, circle_indices);
}

void draw_buffer::prim_rect_uv(const position& a, const position& c, const position& uv_a, const position& uv_c, uint32_t color) {
    // collect vertices and indices for this textured quad
    std::vector<vertex> quad_vertices;
    std::vector<uint32_t> quad_indices;
    
    // add 4 vertices for textured quad
    quad_vertices.push_back(core::vertex(a.x, a.y, 0, color, uv_a.x, uv_a.y)); // top-left
    quad_vertices.push_back(core::vertex(c.x, a.y, 0, color, uv_c.x, uv_a.y)); // top-right
    quad_vertices.push_back(core::vertex(c.x, c.y, 0, color, uv_c.x, uv_c.y)); // bottom-right
    quad_vertices.push_back(core::vertex(a.x, c.y, 0, color, uv_a.x, uv_c.y)); // bottom-left
    
    // add indices for 2 triangles (filled quad)
    quad_indices.push_back(0);
    quad_indices.push_back(1);
    quad_indices.push_back(2);
    quad_indices.push_back(0);
    quad_indices.push_back(2);
    quad_indices.push_back(3);
    
    // use unified geometry system
    if (current_texture()) {
        add_geometry_textured(quad_vertices, quad_indices, current_texture());
    } else {
        // fallback to color-only if no texture
        add_geometry_color_only(quad_vertices, quad_indices);
    }
}

void draw_buffer::n_gon(const position& center, float radius, int sides, uint32_t color) {
    if (sides < 3) sides = 3;
    
    // collect vertices and indices for this n-gon
    std::vector<vertex> ngon_vertices;
    std::vector<uint32_t> ngon_indices;
    
    // add center vertex
    ngon_vertices.push_back(core::vertex(center.x, center.y, 0, color, 0, 0));
    
    // add perimeter vertices
    for (int i = 0; i < sides; ++i) {
        float angle = 2.0f * 3.14159f * float(i) / float(sides);
        float x = center.x + radius * std::cos(angle);
        float y = center.y + radius * std::sin(angle);
        ngon_vertices.push_back(core::vertex(x, y, 0, color, 0, 0));
    }
    
    // add indices for triangles (fan from center)
    for (int i = 1; i < sides; ++i) {
        ngon_indices.push_back(0);           // center
        ngon_indices.push_back(i);           // current perimeter vertex
        ngon_indices.push_back(i + 1);       // next perimeter vertex
    }
    
    // close the n-gon
    ngon_indices.push_back(0);
    ngon_indices.push_back(sides);
    ngon_indices.push_back(1);
    
    // use unified geometry system
    add_geometry_color_only(ngon_vertices, ngon_indices);
}

void draw_buffer::text(const std::string& str, const position& pos, uint32_t color) {
    if (font_stack_.empty()) {
        utils::log_warn("text: no font set, skipping text rendering");
        return;
    }
    
    auto font = font_stack_.back();
    if (!font) {
        utils::log_warn("text: font is null, skipping text rendering");
        return;
    }
    
    // collect all vertices and indices for this text
    std::vector<vertex> text_vertices;
    std::vector<uint32_t> text_indices;
    
    float x = pos.x;
    float baseline_y = pos.y + font->metrics().ascender;
    
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
                bytes_read = 3;
            }
        }
        
        if (bytes_read == 0) {
            // invalid UTF-8, skip one byte
            ptr++;
            continue;
        }
        
        // ensure glyph is loaded (dynamic paging)
        if (!font->ensure_glyph(codepoint)) {
            // try fallback fonts
            auto fallback_font = font->get_fallback_for_codepoint(codepoint);
            if (fallback_font && fallback_font->ensure_glyph(codepoint)) {
                font = fallback_font; // use fallback font for this glyph
            } else {
                utils::log_warn("Failed to load glyph for codepoint U+%04X in font and all fallbacks", codepoint);
                ptr += bytes_read;
                continue;
            }
        }
        
        const auto& glyph = font->glyphs().at(codepoint);
        
        // calculate glyph position relative to baseline
        float x0 = x + glyph.bearingX;
        float y0 = baseline_y - glyph.bearingY; // bearingY is distance from baseline to top
        float x1 = x0 + glyph.width;
        float y1 = y0 + glyph.height;
         
        float u0 = glyph.u0, v0 = glyph.v0, u1 = glyph.u1, v1 = glyph.v1;
        
        // create glyph vertices and indices
        uint32_t base_vertex = static_cast<uint32_t>(text_vertices.size());
        
        // add 4 vertices for glyph quad
        text_vertices.push_back(core::vertex(x0, y0, 0, color, u0, v0)); // top-left
        text_vertices.push_back(core::vertex(x1, y0, 0, color, u1, v0)); // top-right
        text_vertices.push_back(core::vertex(x1, y1, 0, color, u1, v1)); // bottom-right
        text_vertices.push_back(core::vertex(x0, y1, 0, color, u0, v1)); // bottom-left
        
        // add indices for 2 triangles (filled quad)
        text_indices.push_back(base_vertex);
        text_indices.push_back(base_vertex + 1);
        text_indices.push_back(base_vertex + 2);
        text_indices.push_back(base_vertex);
        text_indices.push_back(base_vertex + 2);
        text_indices.push_back(base_vertex + 3);
        
        // advance to next character position
        x += glyph.advance;
        
        ptr += bytes_read;
    }
    
    // use unified geometry system to add font geometry
    if (!text_vertices.empty() && !text_indices.empty()) {
        add_geometry_font(text_vertices, text_indices, font);
    }
}

void draw_buffer::set_blur(uint8_t strength, uint8_t passes) {
    if (!cmds.empty()) {
        cmds.back().blur_strength = strength;
        cmds.back().pass_count = passes;
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
    if (!texture) {
        utils::log_warn("push_texture: attempting to push null texture");
        return;
    }
    
    // validate texture dimensions
    uint32_t width, height;
    if (!texture->get_size(width, height) || width == 0 || height == 0) {
        utils::log_warn("push_texture: invalid texture dimensions: %ux%u", width, height);
        return;
    }
    
    texture_stack_.push_back(texture);
    // utils::log_info("push_texture: added texture %ux%u, stack depth: %zu", width, height, texture_stack_.size());
}

void draw_buffer::pop_texture() {
    if (texture_stack_.empty()) {
        utils::log_warn("pop_texture: texture stack is empty");
        return;
    }
    texture_stack_.pop_back();
}

resources::tex draw_buffer::current_texture() const {
    if (texture_stack_.empty()) {
        return nullptr;
    }
    return texture_stack_.back();
}

// Enhanced texture management with RAII
draw_buffer::texture_scope::texture_scope(draw_buffer* buffer, resources::tex texture)
    : _buffer(buffer) {
    if (_buffer && texture) {
        _buffer->push_texture(texture);
    }
}

draw_buffer::texture_scope::~texture_scope() {
    if (_buffer) {
        _buffer->pop_texture();
    }
}

std::unique_ptr<draw_buffer::texture_scope> draw_buffer::push_texture_scope(resources::tex texture) {
    return std::make_unique<texture_scope>(this, texture);
}

void draw_buffer::clear_texture_stack() {
    texture_stack_.clear();
}

bool draw_buffer::is_texture_stack_valid() const {
    // check for any null textures in the stack
    for (const auto& tex : texture_stack_) {
        if (!tex) {
            return false;
        }
    }
    return true;
}

// Unified geometry methods that automatically handle command creation
void draw_buffer::add_geometry_color_only(const std::vector<vertex>& vertices, const std::vector<uint32_t>& indices) {
    begin_command(core::geometry_type::color_only, "color_only");
    
    uint32_t base_vertex = static_cast<uint32_t>(this->vertices.size());
    uint32_t base_index = static_cast<uint32_t>(this->indices.size());
    
    // add vertices
    this->vertices.insert(this->vertices.end(), vertices.begin(), vertices.end());
    
    // add indices with offset
    for (uint32_t idx : indices) {
        this->indices.push_back(base_vertex + idx);
    }
    
    // update command
    if (!cmds.empty()) {
        cmds.back().elem_count = static_cast<uint32_t>(indices.size());
    }
    
    end_command();
}

void draw_buffer::add_geometry_textured(const std::vector<vertex>& vertices, const std::vector<uint32_t>& indices, resources::tex texture) {
    begin_command(core::geometry_type::textured, "generic");
    
    // push texture to stack
    if (texture) {
        push_texture(texture);
    }
    
    uint32_t base_vertex = static_cast<uint32_t>(this->vertices.size());
    uint32_t base_index = static_cast<uint32_t>(this->indices.size());
    
    // add vertices
    this->vertices.insert(this->vertices.end(), vertices.begin(), vertices.end());
    
    // add indices with offset
    for (uint32_t idx : indices) {
        this->indices.push_back(base_vertex + idx);
    }
    
    // update command
    if (!cmds.empty()) {
        cmds.back().elem_count = static_cast<uint32_t>(indices.size());
        cmds.back().native_texture = true;
    }
    
    end_command();
}

void draw_buffer::add_geometry_font(const std::vector<vertex>& vertices, const std::vector<uint32_t>& indices, std::shared_ptr<resources::font> font) {
    // utils::log_info("add_geometry_font: vertices=%zu, indices=%zu, font=%s", 
    //                vertices.size(), indices.size(), font ? "valid" : "null");
    
    begin_command(core::geometry_type::font_atlas, "generic");
    
    // store font reference for rendering
    if (font) {
        push_font(font);
        // utils::log_info("Pushed font to stack, stack depth now: %zu", font_stack_.size());
    }
    
    uint32_t base_vertex = static_cast<uint32_t>(this->vertices.size());
    uint32_t base_index = static_cast<uint32_t>(this->indices.size());
    
    // add vertices
    this->vertices.insert(this->vertices.end(), vertices.begin(), vertices.end());
    
    // add indices with offset
    for (uint32_t idx : indices) {
        this->indices.push_back(base_vertex + idx);
    }
    
    // update command
    if (!cmds.empty()) {
        cmds.back().elem_count = static_cast<uint32_t>(indices.size());
        cmds.back().font_texture = true;
        // utils::log_info("Created font command: elem_count=%u, font_texture=%s", 
        //                cmds.back().elem_count, cmds.back().font_texture ? "true" : "false");
    }
    
    end_command();
}

// Command management
void draw_buffer::begin_command(geometry_type type, const std::string& shader_hint) {
    draw_command cmd;
    cmd.type = type;
    cmd.shader_hint = shader_hint;
    cmds.push_back(cmd);
}

void draw_buffer::end_command() {
    // command is already complete when added
}

draw_command* draw_buffer::current_command() {
    if (cmds.empty()) return nullptr;
    return &cmds.back();
}

void draw_buffer::clear_all() {
    vertices.clear();
    indices.clear();
    cmds.clear();
    clear_texture_stack();
    font_stack_.clear();
}

} // namespace core 