#include "buffer.h"
#include <cmath>
#include <string>
#include <algorithm>
#include "../resources/font.h"
#include "../utils/logger.h"
#include <stack>
#include "../math/constants.h"

namespace core {

// Helper function to generate rounded quad geometry
void buffer::generate_rounded_quad_geometry(const position& a, const position& c, float rounding,
                                               std::vector<vertex>& vertices, std::vector<uint32_t>& indices,
                                               uint32_t color, const position& uv_a, const position& uv_c) {
    if (rounding <= 0.0f) {
        // no rounding, generate regular quad
        vertices.push_back(core::vertex(a.x, a.y, 0, color, uv_a.x, uv_a.y)); // top-left
        vertices.push_back(core::vertex(c.x, a.y, 0, color, uv_c.x, uv_a.y)); // top-right
        vertices.push_back(core::vertex(c.x, c.y, 0, color, uv_c.x, uv_c.y)); // bottom-right
        vertices.push_back(core::vertex(a.x, c.y, 0, color, uv_a.x, uv_c.y)); // bottom-left
        
        // add indices for 2 triangles (filled quad)
        indices.push_back(0);
        indices.push_back(1);
        indices.push_back(2);
        indices.push_back(0);
        indices.push_back(2);
        indices.push_back(3);
        return;
    }
    
    // calculate corner radius (clamp to prevent overlapping)
    float width = c.x - a.x;
    float height = c.y - a.y;
    float max_radius = (width < height ? width : height) * 0.5f;
    float radius = max_radius * rounding;
    
    // determine number of segments for corners (more segments = smoother)
    int segments = static_cast<int>((32.0f > radius * 0.5f ? 32.0f : radius * 0.5f));
    
    // ImGui-style 9-slice approach: center rect + 4 corners + 4 edges
    uint32_t base_vertex = static_cast<uint32_t>(vertices.size());
    
    // Store corner vertex indices for later use
    struct corner_info {
        uint32_t center_vertex;
        uint32_t first_arc_vertex;
        uint32_t last_arc_vertex;
    };
    std::vector<corner_info> corners;
    
    // 1. Center rectangle (no rounding)
    if (width > 2 * radius && height > 2 * radius) {
        // top-left, top-right, bottom-right, bottom-left of center
        vertices.push_back(core::vertex(a.x + radius, a.y + radius, 0, color, uv_a.x, uv_a.y));
        vertices.push_back(core::vertex(c.x - radius, a.y + radius, 0, color, uv_c.x, uv_a.y));
        vertices.push_back(core::vertex(c.x - radius, c.y - radius, 0, color, uv_c.x, uv_c.y));
        vertices.push_back(core::vertex(a.x + radius, c.y - radius, 0, color, uv_a.x, uv_c.y));
        
        // center rectangle indices
        uint32_t center_base = base_vertex;
        indices.push_back(center_base + 0);
        indices.push_back(center_base + 1);
        indices.push_back(center_base + 2);
        indices.push_back(center_base + 0);
        indices.push_back(center_base + 2);
        indices.push_back(center_base + 3);
        
        base_vertex += 4;
    }
    
    float angle_step = 0.5f * math::PI<float> / segments;

    // 2. Top-left corner (arc from 180° to 270°)
    if (radius > 0) {
        uint32_t corner_base = base_vertex;
    
        // Center of the arc (used for triangle fan)
        vertices.push_back(core::vertex(a.x + radius, a.y + radius, 0, color, uv_a.x, uv_a.y));
    
        float start_angle = math::PI<float>;
    
        for (int i = 0; i <= segments; ++i) {
            float angle = start_angle + i * angle_step;
            float x = a.x + radius + radius * std::cos(angle);
            float y = a.y + radius + radius * std::sin(angle);
    
            // UV interpolation
            float u = (x - a.x) / width;
            float v = (y - a.y) / height;
            u = uv_a.x + u * (uv_c.x - uv_a.x);
            v = uv_a.y + v * (uv_c.y - uv_a.y);
    
            vertices.push_back(core::vertex(x, y, 0, color, u, v));
        }
        
        // Triangle fan indices
        for (int i = 1; i <= segments; ++i) {
            indices.push_back(corner_base);
            indices.push_back(corner_base + i);
            indices.push_back(corner_base + i + 1);
        }
        
        // Store corner info: center, first arc vertex, last arc vertex
        corners.push_back({corner_base, corner_base + 1, corner_base + segments});
    
        base_vertex += segments + 2;
    }
    
    // 3. Top-right corner (arc from 270° to 0°)
    if (radius > 0) {
        uint32_t corner_base = base_vertex;
        vertices.push_back(core::vertex(c.x - radius, a.y + radius, 0, color, uv_c.x, uv_a.y));
    
        float start_angle = 1.5f * math::PI<float>; // 270°
    
        for (int i = 0; i <= segments; ++i) {
            float angle = start_angle + i * angle_step;
            float x = c.x - radius + radius * std::cos(angle);
            float y = a.y + radius + radius * std::sin(angle);
    
            float u = (x - a.x) / width;
            float v = (y - a.y) / height;
            u = uv_a.x + u * (uv_c.x - uv_a.x);
            v = uv_a.y + v * (uv_c.y - uv_a.y);
    
            vertices.push_back(core::vertex(x, y, 0, color, u, v));
        }
        
        for (int i = 1; i <= segments; ++i) {
            indices.push_back(corner_base);
            indices.push_back(corner_base + i);
            indices.push_back(corner_base + i + 1);
        }
        
        corners.push_back({corner_base, corner_base + 1, corner_base + segments});
    
        base_vertex += segments + 2;
    }
    
    // 4. Bottom-right corner (arc from 0° to 90°)
    if (radius > 0) {
        uint32_t corner_base = base_vertex;
        vertices.push_back(core::vertex(c.x - radius, c.y - radius, 0, color, uv_c.x, uv_c.y));
    
        float start_angle = 0.0f; // 0°
    
        for (int i = 0; i <= segments; ++i) {
            float angle = start_angle + i * angle_step;
            float x = c.x - radius + radius * std::cos(angle);
            float y = c.y - radius + radius * std::sin(angle);
    
            float u = (x - a.x) / width;
            float v = (y - a.y) / height;
            u = uv_a.x + u * (uv_c.x - uv_a.x);
            v = uv_a.y + v * (uv_c.y - uv_a.y);
    
            vertices.push_back(core::vertex(x, y, 0, color, u, v));
        }
        
        for (int i = 1; i <= segments; ++i) {
            indices.push_back(corner_base);
            indices.push_back(corner_base + i);
            indices.push_back(corner_base + i + 1);
        }
        
        corners.push_back({corner_base, corner_base + 1, corner_base + segments});
    
        base_vertex += segments + 2;
    }
    
    // 5. Bottom-left corner (arc from 90° to 180°)
    if (radius > 0) {
        uint32_t corner_base = base_vertex;
        vertices.push_back(core::vertex(a.x + radius, c.y - radius, 0, color, uv_a.x, uv_c.y));
    
        float start_angle = 0.5f * math::PI<float>; // 90°
    
        for (int i = 0; i <= segments; ++i) {
            float angle = start_angle + i * angle_step;
            float x = a.x + radius + radius * std::cos(angle);
            float y = c.y - radius + radius * std::sin(angle);
    
            float u = (x - a.x) / width;
            float v = (y - a.y) / height;
            u = uv_a.x + u * (uv_c.x - uv_a.x);
            v = uv_a.y + v * (uv_c.y - uv_a.y);
    
            vertices.push_back(core::vertex(x, y, 0, color, u, v));
        }
    
        for (int i = 1; i <= segments; ++i) {
            indices.push_back(corner_base);
            indices.push_back(corner_base + i);
            indices.push_back(corner_base + i + 1);
        }
        
        corners.push_back({corner_base, corner_base + 1, corner_base + segments});
    
        base_vertex += segments + 2;
    }
    
    // 6. Top edge strip (connects top-left and top-right corners)
    if (radius > 0 && corners.size() >= 2) {

        uint32_t edge_base = base_vertex;

        // left and right edge vertices
        vertices.push_back(core::vertex(a.x + radius, a.y, 0, color, uv_a.x, uv_a.y));
        vertices.push_back(core::vertex(c.x - radius, a.y, 0, color, uv_c.x, uv_a.y));

        // connect to corners using stored corner info
        uint32_t tlc = corners[0].center_vertex;  // top-left corner center vertex
        uint32_t trc = corners[1].center_vertex; // top-right corner center vertex
        
        // tl, tr, trc
        indices.push_back(edge_base + 0);
        indices.push_back(edge_base + 1);
        indices.push_back(trc);
        
        // tl, trc, tlc
        indices.push_back(edge_base + 0);
        indices.push_back(trc); 
        indices.push_back(tlc);    

        base_vertex += 2;

    }
    
    // 7. Right edge strip (connects top-right and bottom-right corners)
    if (radius > 0 && corners.size() >= 3) {

        uint32_t edge_base = base_vertex;
        // top and bottom edge vertices
        vertices.push_back(core::vertex(c.x, a.y + radius, 0, color, uv_c.x, uv_a.y));
        vertices.push_back(core::vertex(c.x, c.y - radius, 0, color, uv_c.x, uv_c.y));

        // connect to corners using stored corner info
        uint32_t trc = corners[1].center_vertex;   // top-right corner center vertex
        uint32_t brc = corners[2].center_vertex; // bottom-right corner center vertex
        
        // tr, br, brc
        indices.push_back(edge_base + 0);
        indices.push_back(edge_base + 1);
        indices.push_back(brc);
        
        // tr, brc, trc
        indices.push_back(edge_base + 0);
        indices.push_back(brc);
        indices.push_back(trc);
    
        base_vertex += 2;

    }
    
    // 8. Bottom edge strip (connects bottom-right and bottom-left corners)
    if (radius > 0 && corners.size() >= 4) {

        uint32_t edge_base = base_vertex;
        // left and right edge vertices
        vertices.push_back(core::vertex(a.x + radius, c.y, 0, color, uv_a.x, uv_c.y));
        vertices.push_back(core::vertex(c.x - radius, c.y, 0, color, uv_c.x, uv_c.y));

        // connect to corners using stored corner info
        uint32_t brc = corners[2].center_vertex;  // bottom-right corner center vertex
        uint32_t blc = corners[3].center_vertex;  // bottom-left corner center vertex
        
        // br, bl, blc
        indices.push_back(edge_base + 1);
        indices.push_back(edge_base + 0);
        indices.push_back(blc);
        
        // br, blc, brc
        indices.push_back(edge_base + 1);
        indices.push_back(blc);
        indices.push_back(brc);

        base_vertex += 2;

    }
    
    // 9. Left edge strip (connects bottom-left and top-left corners)
    if (radius > 0 && corners.size() >= 4) {

        uint32_t edge_base = base_vertex;
        // top and bottom edge vertices
        vertices.push_back(core::vertex(a.x, a.y + radius, 0, color, uv_a.x, uv_a.y));
        vertices.push_back(core::vertex(a.x, c.y - radius, 0, color, uv_a.x, uv_c.y));

        // connect to corners using stored corner info
        uint32_t blc = corners[3].center_vertex;  // bottom-left corner center vertex
        uint32_t tlc = corners[0].center_vertex;   // top-left corner center vertex
        
        // bl, tl, tlc
        indices.push_back(edge_base + 1);
        indices.push_back(edge_base + 0);
        indices.push_back(tlc);
        
        // bl, tlc, blc
        indices.push_back(edge_base + 1);
        indices.push_back(tlc);
        indices.push_back(blc);

        base_vertex += 2;

    }
}

void buffer::prim_rect(const position& a, const position& c, const color& col, float rounding) {
    // collect vertices and indices for this rectangle outline
    std::vector<vertex> rect_vertices;
    std::vector<uint32_t> rect_indices;
    
    if (rounding <= 0.0f) {
        // original outline logic for no rounding
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
    } else {
        // use rounded quad helper for outline (simplified - just perimeter)
        generate_rounded_quad_geometry(a, c, rounding, rect_vertices, rect_indices, pack_color_abgr(col));
    }
    
    // use unified geometry system
    add_geometry_color_only(rect_vertices, rect_indices);
}

void buffer::prim_rect_filled(const position& a, const position& c, const color& col, float rounding) {
    // collect vertices and indices for this color-only quad
    std::vector<vertex> quad_vertices;
    std::vector<uint32_t> quad_indices;
    
    // use the rounded quad helper function
    generate_rounded_quad_geometry(a, c, rounding, quad_vertices, quad_indices, pack_color_abgr(col));
    
    // use unified geometry system
    add_geometry_color_only(quad_vertices, quad_indices);
}

void buffer::prim_rect_multi_color(const position& a, const position& c, 
                                       const color& col_top_left, const color& col_top_right,
                                       const color& col_bot_left, const color& col_bot_right, float rounding) {
    // collect vertices and indices for this multi-color quad
    std::vector<vertex> quad_vertices;
    std::vector<uint32_t> quad_indices;
    
    if (rounding <= 0.0f) {
        // original multi-color logic for no rounding
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
    } else {
        // for rounded multi-color, we need to interpolate colors across the rounded surface
        // this is complex, so we'll use the average color for now
        color avg_color = {
            (col_top_left.x + col_top_right.x + col_bot_left.x + col_bot_right.x) * 0.25f,
            (col_top_left.y + col_top_right.y + col_bot_left.y + col_bot_right.y) * 0.25f,
            (col_top_left.z + col_top_right.z + col_bot_left.z + col_bot_right.z) * 0.25f,
            (col_top_left.w + col_top_right.w + col_bot_left.w + col_bot_right.w) * 0.25f
        };
        generate_rounded_quad_geometry(a, c, rounding, quad_vertices, quad_indices, pack_color_abgr(avg_color));
    }
    
    // use unified geometry system
    add_geometry_color_only(quad_vertices, quad_indices);
}

void buffer::line(const position& a, const position& b, uint32_t color_a, uint32_t color_b, float thickness) {
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

void buffer::line_strip(const std::vector<position>& points, uint32_t color, float thickness) {
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

void buffer::poly_line(const std::vector<position>& points, uint32_t color, float thickness, bool closed) {
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

void buffer::triangle_filled(const position& a, const position& b, const position& c, uint32_t color_a, uint32_t color_b, uint32_t color_c) {
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

void buffer::circle_filled(const position& center, float radius, uint32_t color_inner, uint32_t color_outer, int segments) {
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

void buffer::prim_rect_uv(const position& a, const position& c, const position& uv_a, const position& uv_c, uint32_t color, float rounding) {
    // collect vertices and indices for this textured quad
    std::vector<vertex> quad_vertices;
    std::vector<uint32_t> quad_indices;
    
    // use the rounded quad helper function with UV coordinates
    generate_rounded_quad_geometry(a, c, rounding, quad_vertices, quad_indices, color, uv_a, uv_c);
    
    // use unified geometry system
    if (current_texture()) {
        add_geometry_textured(quad_vertices, quad_indices, current_texture());
    } else {
        // fallback to color-only if no texture
        add_geometry_color_only(quad_vertices, quad_indices);
    }
}

void buffer::n_gon(const position& center, float radius, int sides, uint32_t color) {
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

void buffer::text(const std::string& str, const position& pos, uint32_t color) {
    if (font_stack_.empty()) {
        utils::log_warn("text: no font set, skipping text rendering");
        return;
    }
    
    auto base_font = font_stack_.back();
    if (!base_font) {
        utils::log_warn("text: font is null, skipping text rendering");
        return;
    }
    
    // collect vertices/indices per font run
    std::vector<vertex> run_vertices;
    std::vector<uint32_t> run_indices;
    std::shared_ptr<resources::font> run_font = nullptr;
    
    float x = pos.x;
    float baseline_y = pos.y + base_font->metrics().ascender;
        
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
        
        // select a font that can provide this glyph (base font or fallbacks)
        std::shared_ptr<resources::font> glyph_font = base_font;
        bool have_glyph = glyph_font->ensure_glyph(codepoint);
        if (!have_glyph) {
            // try declared fallbacks in order
            for (const auto& fb : base_font->fallbacks()) {
                if (fb && fb->ensure_glyph(codepoint)) { glyph_font = fb; have_glyph = true; break; }
            }
            // try default fallback as last resort
            if (!have_glyph) {
                auto df = base_font->get_default_fallback();
                if (df && df->ensure_glyph(codepoint)) { glyph_font = df; have_glyph = true; }
            }
            if (have_glyph && glyph_font.get() != base_font.get()) {
                utils::log_debug("text: using fallback font '%s' for U+%04X", glyph_font->path().c_str(), codepoint);
            }
        }
        if (!have_glyph) {
            utils::log_warn("glyph for codepoint U+%04X in font and all fallbacks", codepoint);
            ptr += bytes_read;
            continue;
        }
        
        // if font changed, flush previous run
        if (!run_font) {
            run_font = glyph_font;
            utils::log_debug("text: start run with font '%s'", run_font->path().c_str());
        }
        if (glyph_font.get() != run_font.get()) {
            if (!run_vertices.empty() && !run_indices.empty()) {
                add_geometry_font(run_vertices, run_indices, run_font);
                utils::log_debug("text: flush run font='%s' vtx=%zu idx=%zu", run_font->path().c_str(), run_vertices.size(), run_indices.size());
                run_vertices.clear();
                run_indices.clear();
            }
            run_font = glyph_font;
            utils::log_debug("text: switch run to font '%s'", run_font->path().c_str());
        }

        const auto& glyph = glyph_font->glyphs().at(codepoint);
        
        // calculate glyph position relative to baseline
        float x0 = x + glyph.bearingX;
        float y0 = baseline_y - glyph.bearingY; // bearingY is distance from baseline to top
        float x1 = x0 + glyph.width;
        float y1 = y0 + glyph.height;
         
        float u0 = glyph.u0, v0 = glyph.v0, u1 = glyph.u1, v1 = glyph.v1;
        
        // create glyph vertices and indices
        uint32_t base_vertex = static_cast<uint32_t>(run_vertices.size());
        
        // add 4 vertices for glyph quad
        run_vertices.push_back(core::vertex(x0, y0, 0, color, u0, v0)); // top-left
        run_vertices.push_back(core::vertex(x1, y0, 0, color, u1, v0)); // top-right
        run_vertices.push_back(core::vertex(x1, y1, 0, color, u1, v1)); // bottom-right
        run_vertices.push_back(core::vertex(x0, y1, 0, color, u0, v1)); // bottom-left
        
        // add indices for 2 triangles (filled quad)
        run_indices.push_back(base_vertex);
        run_indices.push_back(base_vertex + 1);
        run_indices.push_back(base_vertex + 2);
        run_indices.push_back(base_vertex);
        run_indices.push_back(base_vertex + 2);
        run_indices.push_back(base_vertex + 3);
        
        // advance to next character position
        x += glyph.advance;
        
        ptr += bytes_read;
    }
    
    // flush last run
    if (!run_vertices.empty() && !run_indices.empty() && run_font) {
        utils::log_debug("text: flush final run font='%s' vtx=%zu idx=%zu", run_font->path().c_str(), run_vertices.size(), run_indices.size());
        add_geometry_font(run_vertices, run_indices, run_font);
    }
}

void buffer::set_blur(uint8_t strength, uint8_t passes) {
    if (!cmds.empty()) {
        cmds.back().blur_strength = strength;
        cmds.back().pass_count = passes;
    }
}

void buffer::set_key_color(const color& col) {
    if (!cmds.empty()) {
        cmds.back().key_color = col;
    }
}

void buffer::push_font(std::shared_ptr<resources::font> font) {
    font_stack_.push_back(font);
}
void buffer::pop_font() {
    if (!font_stack_.empty()) font_stack_.pop_back();
}
std::shared_ptr<resources::font> buffer::current_font() const {
    if (!font_stack_.empty()) return font_stack_.back();
    return nullptr;
}

// Texture stack methods
void buffer::push_texture(resources::tex texture) {
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

void buffer::pop_texture() {
    if (texture_stack_.empty()) {
        utils::log_warn("pop_texture: texture stack is empty");
        return;
    }
    texture_stack_.pop_back();
}

resources::tex buffer::current_texture() const {
    if (texture_stack_.empty()) {
    return nullptr;
    }
    return texture_stack_.back();
}

// Enhanced texture management with RAII
buffer::texture_scope::texture_scope(buffer* buffer, resources::tex texture)
    : _buffer(buffer) {
    if (_buffer && texture) {
        _buffer->push_texture(texture);
    }
}

buffer::texture_scope::~texture_scope() {
    if (_buffer) {
        _buffer->pop_texture();
    }
}

std::unique_ptr<buffer::texture_scope> buffer::push_texture_scope(resources::tex texture) {
    return std::make_unique<texture_scope>(this, texture);
}

void buffer::clear_texture_stack() {
    texture_stack_.clear();
}

bool buffer::is_texture_stack_valid() const {
    // check for any null textures in the stack
    for (const auto& tex : texture_stack_) {
        if (!tex) {
            return false;
        }
    }
    return true;
}

// Unified geometry methods that automatically handle command creation
void buffer::add_geometry_color_only(const std::vector<vertex>& vertices, const std::vector<uint32_t>& indices) {
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

void buffer::add_geometry_textured(const std::vector<vertex>& vertices, const std::vector<uint32_t>& indices, resources::tex texture) {
    begin_command(core::geometry_type::textured, "generic");
    
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
        cmds.back().texture = texture;
    }
    
    end_command();
}

void buffer::add_geometry_font(const std::vector<vertex>& vertices, const std::vector<uint32_t>& indices, std::shared_ptr<resources::font> font) {
    // utils::log_info("add_geometry_font: vertices=%zu, indices=%zu, font=%s", 
    //                vertices.size(), indices.size(), font ? "valid" : "null");
    
    begin_command(core::geometry_type::font_atlas, "generic");
    
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
        cmds.back().font = font;
    }
    
    end_command();
}

// Command management
void buffer::begin_command(geometry_type type, const std::string& shader_hint) {
    draw_command cmd;
    cmd.type = type;
    cmd.shader_hint = shader_hint;
    cmds.push_back(cmd);
}

void buffer::end_command() {
    // command is already complete when added
}

draw_command* buffer::current_command() {
    if (cmds.empty()) return nullptr;
    return &cmds.back();
}

void buffer::clear_all() {
    vertices.clear();
    indices.clear();
    cmds.clear();
    clear_texture_stack();
    font_stack_.clear();
}

} // namespace core 