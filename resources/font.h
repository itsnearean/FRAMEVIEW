#pragma once

#include "texture.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef _WIN32
#include <wrl/client.h>
#include <d3d11.h>
#endif

namespace resources {

struct glyph_info {
    float u0, v0, u1, v1; // uv coordinates in atlas
    int width, height;    // glyph size in pixels
    int advance;          // advance to next glyph
    int bearingX, bearingY; // offset from baseline
    uint32_t codepoint;     // unicode codepoint
    bool colored = false;   // true if glyph is color (colr/cpal)
};

struct font_metrics {
    float ascender = 0.f;
    float descender = 0.f;
    float line_gap = 0.f;
    float line_height = 0.f;
    float max_advance = 0.f;
};

struct opentype_features {
    bool ligatures = true;
    bool alternates = false;
};

class font {
public:
    font(const char* path, float size, bool sdf = false, bool mcsdf = false);
    font(const void* data, size_t size, float pixel_height, bool sdf = false, bool mcsdf = false);
    virtual ~font() = default;

#ifdef _WIN32
    virtual bool load(ID3D11Device* device = nullptr, resources::texture_dict* tex_dict = nullptr);
    virtual bool load_from_memory(ID3D11Device* device = nullptr, resources::texture_dict* tex_dict = nullptr);
#else
    virtual bool load(resources::texture_dict* tex_dict = nullptr);
    virtual bool load_from_memory(resources::texture_dict* tex_dict = nullptr);
#endif

    virtual void unload();

    float size() const;
    const std::string& path() const { return _path; }
    const std::unordered_map<uint32_t, glyph_info>& glyphs() const { return _glyphs; }
    const std::vector<unsigned char>& atlas_bitmap() const { return _atlas_bitmap; }
    int atlas_width() const { return _atlas_width; }
    int atlas_height() const { return _atlas_height; }
    const font_metrics& metrics() const { return _metrics; }
    bool is_sdf() const { return _sdf; }
    bool is_mcsdf() const { return _mcsdf; }
    bool is_colored() const { return _colored; }

    // kerning api
    int get_kerning(uint32_t left, uint32_t right);
    bool has_kerning() const { return _has_kerning; }

    // fallback font chain
    void add_fallback(std::shared_ptr<font> fallback);
    const std::vector<std::shared_ptr<font>>& fallbacks() const { return _fallbacks; }
    
    // enhanced fallback system
    std::shared_ptr<font> get_fallback_for_codepoint(uint32_t codepoint) const;
    bool has_glyph_in_fallbacks(uint32_t codepoint) const;
    void set_default_fallback(std::shared_ptr<font> fallback);
    std::shared_ptr<font> get_default_fallback() const { return _default_fallback; }

    // dynamic glyph paging
    bool has_glyph(uint32_t codepoint) const;
    bool request_glyph(uint32_t codepoint); // loads and packs glyph on demand
    bool ensure_glyph(uint32_t codepoint); // ensures glyph is available, loads if needed

    // opentype features
    void set_opentype_features(const opentype_features& features);
    const opentype_features& get_opentype_features() const { return _ot_features; }

    // static: load all fonts from resources/fonts
    static std::vector<std::shared_ptr<font>> load_all_from_folder(const std::string& folder, float size, bool sdf = false, bool mcsdf = false, resources::texture_dict* tex_dict = nullptr);

    resources::tex get_atlas_tex() const { return _atlas_tex; }
    int get_glyph_page(uint32_t codepoint) const; // get which page a glyph is on

#ifdef _WIN32
    ID3D11ShaderResourceView* get_atlas_srv() const { return _atlas_srv.Get(); }
    void update_atlas_texture(ID3D11Device* device = nullptr);
#endif

protected:
    resources::tex _atlas_tex;
    std::unordered_map<uint32_t, glyph_info> _glyphs;
    std::string _path;
    float _size;
    std::vector<unsigned char> _atlas_bitmap;
    int _atlas_width = 0, _atlas_height = 0;
    font_metrics _metrics;
    // for memory font
    std::vector<unsigned char> _font_data;
    bool _from_memory = false;
    // kerning
    bool _has_kerning = false;
    // sdf
    bool _sdf = false;
    bool _mcsdf = false;
    // color font
    bool _colored = false;
    // fallbacks
    std::vector<std::shared_ptr<font>> _fallbacks;
    // opentype features
    opentype_features _ot_features;
    // default fallback
    std::shared_ptr<font> _default_fallback;
    // paging
    std::vector<uint32_t> _pending_glyphs;
    std::vector<std::vector<unsigned char>> _atlas_pages; // multiple atlas pages
    std::vector<int> _atlas_page_widths;
    std::vector<int> _atlas_page_heights;
    int _current_page = 0;
    int _current_x = 0;
    int _current_y = 0;
    int _current_row_height = 0;
    FT_Library _ft_library = nullptr;
    FT_Face _ft_face = nullptr;
#ifdef _WIN32
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _atlas_srv;
#endif
};

using font_ptr = std::shared_ptr<font>;

} // namespace resources 