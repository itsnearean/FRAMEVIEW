#include "font.h"
#include "../utils/logger.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <vector>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstdint>
#ifdef _WIN32
#include <wrl/client.h>
#include <d3d11.h>
#endif

namespace resources {

namespace {
constexpr int ATLAS_W = 1024;
constexpr int ATLAS_H = 1024;
constexpr uint32_t FIRST_CODEPOINT = 32;
constexpr uint32_t LAST_CODEPOINT = 0x2FFF; // back to full unicode range

// simple stubs for now
void make_sdf(const unsigned char* src, int w, int h, unsigned char* dst, int spread = 8) {
    // copy source to destination for now
    std::memcpy(dst, src, w * h);
}

void make_mcsdf(const unsigned char* src, int w, int h, unsigned char* dst, int spread = 8) {
    // copy source to destination for now
    for (int i = 0; i < w * h; ++i) {
        dst[3 * i + 0] = src[i];
        dst[3 * i + 1] = src[i];
        dst[3 * i + 2] = src[i];
    }
}

} // namespace

font::font(const char* path, float size, bool sdf, bool mcsdf)
    : _path(path), _size(size), _sdf(false), _mcsdf(false) {}

font::font(const void* data, size_t size, float pixel_height, bool sdf, bool mcsdf)
    : _path(), _size(pixel_height), _from_memory(true), _font_data(static_cast<const unsigned char*>(data), static_cast<const unsigned char*>(data) + size), _sdf(false), _mcsdf(false) {}

#ifdef _WIN32
bool font::load(ID3D11Device* device, resources::texture_dict* tex_dict) {
#else
bool font::load(resources::texture_dict* tex_dict) {
#endif
    if (_from_memory) return load_from_memory(device, tex_dict);
    
    // initialize freetype
    if (FT_Init_FreeType(&_ft_library)) {
        utils::log_error("Could not init FreeType");
        return false;
    }
    
    if (FT_New_Face(_ft_library, _path.c_str(), 0, &_ft_face)) {
        utils::log_error("Failed to load font file: %s", _path.c_str());
        FT_Done_FreeType(_ft_library);
        _ft_library = nullptr;
        return false;
    }
    
    // set font size using proper size request (like inspiration code)
    FT_Size_RequestRec req;
    req.type = FT_SIZE_REQUEST_TYPE_REAL_DIM;
    req.width = 0;
    req.height = static_cast<FT_Long>(_size * 64);
    req.horiResolution = 0;
    req.vertResolution = 0;
    if (FT_Request_Size(_ft_face, &req) != 0) {
        utils::log_error("FT_Request_Size failed");
        return false;
    }
    
    // initialize first atlas page
    _atlas_pages.clear();
    _atlas_page_widths.clear();
    _atlas_page_heights.clear();
    
    // allocate RGBA atlas
    _atlas_pages.push_back(std::vector<unsigned char>(ATLAS_W * ATLAS_H * 4, 0));
    _atlas_page_widths.push_back(ATLAS_W);
    _atlas_page_heights.push_back(ATLAS_H);
    
    _current_page = 0;
    _current_x = 0;
    _current_y = 0;
    _current_row_height = 0;
    
    _has_kerning = FT_HAS_KERNING(_ft_face);
    _colored = (FT_HAS_COLOR(_ft_face) != 0);
    
    // load basic ascii glyphs immediately
    for (uint32_t c = 32; c <= 127; ++c) {
        ensure_glyph(c);
    }
    
    // calculate metrics using FT_CEIL (like inspiration code)
    const auto& metrics = _ft_face->size->metrics;
    _metrics.ascender = static_cast<float>((metrics.ascender + 63) >> 6);  // equivalent to FT_CEIL
    _metrics.descender = static_cast<float>((metrics.descender + 63) >> 6);
    _metrics.line_height = static_cast<float>((metrics.height + 63) >> 6);
    _metrics.line_gap = static_cast<float>((metrics.height - metrics.ascender + metrics.descender + 63) >> 6);
    _metrics.max_advance = static_cast<float>((metrics.max_advance + 63) >> 6);
    
    // ensure atlas dimensions and bitmap are set for D3D11
    _atlas_width = ATLAS_W;
    _atlas_height = ATLAS_H;
    _atlas_bitmap = _atlas_pages[0];
    
#ifdef _WIN32
    // Create D3D11 texture for the atlas
    ID3D11Device* device_to_use = device ? device : nullptr; // TODO: get device from renderer
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = _atlas_width;
    desc.Height = _atlas_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R8G8B8A8_UNORM for rgba, DXGI_FORMAT_R8_UNORM for grayscale
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subres = {};
    subres.pSysMem = _atlas_bitmap.data();
    subres.SysMemPitch = _atlas_width * 4; // 4 for RGBA, 1 for grayscale

    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
    HRESULT hr = device_to_use->CreateTexture2D(&desc, &subres, &tex);
   
    if (FAILED(hr)) {
        utils::log_error("CreateTexture2D failed: 0x%08X", hr);
    }
    if (SUCCEEDED(hr)) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        device_to_use->CreateShaderResourceView(tex.Get(), &srvDesc, &_atlas_srv);
        
        // create proper resources::tex handle using texture dictionary
        if (tex_dict) {
            _atlas_tex = tex_dict->create_texture_from_d3d11(tex.Get(), _atlas_srv.Get());
        }
    }
#endif
    return true;
}

#ifdef _WIN32
bool font::load_from_memory(ID3D11Device* device, resources::texture_dict* tex_dict) {
#else
bool font::load_from_memory(resources::texture_dict* tex_dict) {
#endif
    if (FT_Init_FreeType(&_ft_library)) {
        utils::log_error("Could not init FreeType");
        return false;
    }
    if (FT_New_Memory_Face(_ft_library, _font_data.data(), static_cast<FT_Long>(_font_data.size()), 0, &_ft_face)) {
        utils::log_error("Failed to load font from memory");
        FT_Done_FreeType(_ft_library);
        return false;
    }
    // set font size using proper size request (like inspiration code)
    FT_Size_RequestRec req;
    req.type = FT_SIZE_REQUEST_TYPE_REAL_DIM;
    req.width = 0;
    req.height = static_cast<FT_Long>(_size * 64);
    req.horiResolution = 0;
    req.vertResolution = 0;
    if (FT_Request_Size(_ft_face, &req) != 0) {
        utils::log_error("FT_Request_Size failed");
    return false;
}
    
    _atlas_width = ATLAS_W;
    _atlas_height = ATLAS_H;
    _atlas_bitmap.assign(_atlas_width * _atlas_height * 4, 0);
    int x = 0, y = 0, row_h = 0;
    _has_kerning = FT_HAS_KERNING(_ft_face);
    _colored = (FT_HAS_COLOR(_ft_face) != 0);
    for (uint32_t c = FIRST_CODEPOINT; c <= LAST_CODEPOINT; ++c) {
        if (FT_Load_Char(_ft_face, c, FT_LOAD_RENDER)) continue;
        FT_GlyphSlot g = _ft_face->glyph;
        
        // atlas layout logic
        if (x + g->bitmap.width >= _atlas_width) {
            x = 0;
            y += row_h;
            row_h = 0;
        }
        if (y + g->bitmap.rows >= _atlas_height) {
            utils::log_error("Font atlas overflow for memory font at codepoint U+%04X", c);
            break;
        }
        
        if (g->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
            // 8-bit per pixel
            for (int j = 0; j < g->bitmap.rows; ++j) {
                for (int i = 0; i < g->bitmap.width; ++i) {
                    int atlas_idx = 4 * ((x + i) + (y + j) * _atlas_width);
                    unsigned char mask = g->bitmap.buffer[j * g->bitmap.pitch + i];
                    _atlas_bitmap[atlas_idx + 0] = 255;
                    _atlas_bitmap[atlas_idx + 1] = 255;
                    _atlas_bitmap[atlas_idx + 2] = 255;
                    _atlas_bitmap[atlas_idx + 3] = mask;
                }
            }
        } else if (g->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
            // 1-bit per pixel, each byte is 8 pixels
            for (int j = 0; j < g->bitmap.rows; ++j) {
                for (int i = 0; i < g->bitmap.width; ++i) {
                    int atlas_idx = 4 * ((x + i) + (y + j) * _atlas_width);
                    int byte = g->bitmap.buffer[j * g->bitmap.pitch + (i >> 3)];
                    int bit = 7 - (i & 7);
                    unsigned char mask = (byte & (1 << bit)) ? 255 : 0;
                    _atlas_bitmap[atlas_idx + 0] = 255;
                    _atlas_bitmap[atlas_idx + 1] = 255;
                    _atlas_bitmap[atlas_idx + 2] = 255;
                    _atlas_bitmap[atlas_idx + 3] = mask;
                }
            }
        } else {
            // unsupported pixel mode
            utils::log_warn("Unsupported pixel mode %d for glyph U+%04X", g->bitmap.pixel_mode, c);
            continue;
        }
        glyph_info info;
        info.u0 = float(x) / _atlas_width;
        info.v0 = float(y) / _atlas_height;
        info.u1 = float(x + g->bitmap.width) / _atlas_width;
        info.v1 = float(y + g->bitmap.rows) / _atlas_height;
        info.width = g->bitmap.width;
        info.height = g->bitmap.rows;
        info.advance = g->advance.x >> 6;
        info.bearingX = g->bitmap_left;
        info.bearingY = g->bitmap_top;
        info.codepoint = c;
        info.colored = _colored;
        _glyphs[c] = info;
        x += g->bitmap.width + 1;
        row_h = (row_h > static_cast<int>(g->bitmap.rows)) ? row_h : static_cast<int>(g->bitmap.rows);
    }
    // calculate metrics using FT_CEIL (like inspiration code)
    const auto& metrics = _ft_face->size->metrics;
    _metrics.ascender = static_cast<float>((metrics.ascender + 63) >> 6);  // equivalent to FT_CEIL
    _metrics.descender = static_cast<float>((metrics.descender + 63) >> 6);
    _metrics.line_height = static_cast<float>((metrics.height + 63) >> 6);
    _metrics.line_gap = static_cast<float>((metrics.height - metrics.ascender + metrics.descender + 63) >> 6);
    _metrics.max_advance = static_cast<float>((metrics.max_advance + 63) >> 6);
    FT_Done_Face(_ft_face);
    FT_Done_FreeType(_ft_library);
#ifdef _WIN32
    // Create D3D11 texture for the atlas
    ID3D11Device* device_to_use = device ? device : nullptr;
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = _atlas_width;
    desc.Height = _atlas_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA subres = {};
    subres.pSysMem = _atlas_bitmap.data();
    subres.SysMemPitch = _atlas_width * 4;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
    HRESULT hr = device_to_use->CreateTexture2D(&desc, &subres, &tex);
    if (SUCCEEDED(hr)) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        device_to_use->CreateShaderResourceView(tex.Get(), &srvDesc, &_atlas_srv);
    }
#endif
    return true;
}

#ifdef _WIN32
void font::update_atlas_texture(ID3D11Device* device) {
    if (!device) return;
    
    // create new texture with updated atlas data
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = _atlas_width;
    desc.Height = _atlas_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subres = {};
    subres.pSysMem = _atlas_bitmap.data();
    subres.SysMemPitch = _atlas_width * 4;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
    HRESULT hr = device->CreateTexture2D(&desc, &subres, &tex);
    
    if (SUCCEEDED(hr)) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(tex.Get(), &srvDesc, &_atlas_srv);
        
        // update the texture handle if we have a texture dictionary
        // note: this requires the font to have been loaded with a texture dictionary
        // for now, we'll keep the existing handle and just update the SRV
        if (_atlas_tex) {
            // the texture handle should already exist, so we just need to update the underlying D3D11 texture
            // for now, we'll keep the existing handle and just update the SRV
        }
    } else {
        utils::log_error("Failed to update atlas texture: 0x%08X", hr);
    }
}
#endif

void font::unload() {
    _glyphs.clear();
    _atlas_pages.clear();
    _atlas_page_widths.clear();
    _atlas_page_heights.clear();
    _pending_glyphs.clear();
    
    if (_ft_face) {
        FT_Done_Face(_ft_face);
        _ft_face = nullptr;
    }
    if (_ft_library) {
        FT_Done_FreeType(_ft_library);
        _ft_library = nullptr;
    }
}

int font::get_glyph_page(uint32_t codepoint) const {
    auto it = _glyphs.find(codepoint);
    if (it != _glyphs.end()) {
        // for now, assume all glyphs are on page 0 since we're not using multi-page atlas yet
        return 0;
    }
    return -1;
}

float font::size() const { return _size; }

int font::get_kerning(uint32_t left, uint32_t right) {
    if (_glyphs.empty() || !_has_kerning) return 0;
    FT_Init_FreeType(&_ft_library);
    if (_from_memory) {
        FT_New_Memory_Face(_ft_library, _font_data.data(), static_cast<FT_Long>(_font_data.size()), 0, &_ft_face);
    } else {
        FT_New_Face(_ft_library, _path.c_str(), 0, &_ft_face);
    }
    FT_Set_Pixel_Sizes(_ft_face, 0, static_cast<FT_UInt>(_size));
    FT_UInt l = FT_Get_Char_Index(_ft_face, left);
    FT_UInt r = FT_Get_Char_Index(_ft_face, right);
    FT_Vector kerning;
    kerning.x = kerning.y = 0;
    if (FT_Get_Kerning(_ft_face, l, r, FT_KERNING_DEFAULT, &kerning) == 0) {
        FT_Done_Face(_ft_face);
        FT_Done_FreeType(_ft_library);
        return kerning.x >> 6;
    }
    FT_Done_Face(_ft_face);
    FT_Done_FreeType(_ft_library);
    return 0;
}

void font::add_fallback(std::shared_ptr<font> fallback) {
    if (fallback && fallback.get() != this) {
        _fallbacks.push_back(fallback);
    }
}

std::shared_ptr<font> font::get_fallback_for_codepoint(uint32_t codepoint) const {
    // first check if we have the glyph
    if (has_glyph(codepoint)) {
        return nullptr; // no fallback needed
    }
    
    // check fallback fonts in order
    for (const auto& fallback : _fallbacks) {
        if (fallback && fallback->has_glyph(codepoint)) {
            return fallback;
        }
    }
    
    // check default fallback
    if (_default_fallback && _default_fallback->has_glyph(codepoint)) {
        return _default_fallback;
    }
    
    return nullptr;
}

bool font::has_glyph_in_fallbacks(uint32_t codepoint) const {
    // check fallback fonts
    for (const auto& fallback : _fallbacks) {
        if (fallback && fallback->has_glyph(codepoint)) {
            return true;
        }
    }
    
    // check default fallback
    if (_default_fallback && _default_fallback->has_glyph(codepoint)) {
        return true;
    }
    
    return false;
}

void font::set_default_fallback(std::shared_ptr<font> fallback) {
    if (fallback && fallback.get() != this) {
        _default_fallback = fallback;
    }
}

bool font::has_glyph(uint32_t codepoint) const {
    return _glyphs.find(codepoint) != _glyphs.end();
}

bool font::request_glyph(uint32_t codepoint) {
    return ensure_glyph(codepoint);
}

bool font::ensure_glyph(uint32_t codepoint) {
    if (has_glyph(codepoint)) return true;
    
    if (!_ft_face) {
        utils::log_error("Font not loaded");
        return false;
    }
    
    // load the glyph
    if (FT_Load_Char(_ft_face, codepoint, FT_LOAD_RENDER)) {
        utils::log_warn("Failed to load glyph U+%04X", codepoint);
        return false;
    }
    
    FT_GlyphSlot g = _ft_face->glyph;
    
    // check if we need a new page
    if (_current_x + g->bitmap.width >= _atlas_page_widths[_current_page]) {
        _current_x = 0;
        _current_y += _current_row_height;
        _current_row_height = 0;
    }
    
    if (_current_y + g->bitmap.rows >= _atlas_page_heights[_current_page]) {
        // create new page
        _current_page++;
        _current_x = 0;
        _current_y = 0;
        _current_row_height = 0;
        
        _atlas_pages.push_back(std::vector<unsigned char>(ATLAS_W * ATLAS_H * 4, 0));
        _atlas_page_widths.push_back(ATLAS_W);
        _atlas_page_heights.push_back(ATLAS_H);
        
    }
    
    // copy glyph data to atlas
    auto& current_page = _atlas_pages[_current_page];
    int page_width = _atlas_page_widths[_current_page];
    
    if (g->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
        // 8-bit per pixel
        for (int j = 0; j < g->bitmap.rows; ++j) {
            for (int i = 0; i < g->bitmap.width; ++i) {
                int atlas_idx = 4 * ((_current_x + i) + (_current_y + j) * page_width);
                unsigned char mask = g->bitmap.buffer[j * g->bitmap.pitch + i];
                current_page[atlas_idx + 0] = 255; // R
                current_page[atlas_idx + 1] = 255; // G
                current_page[atlas_idx + 2] = 255; // B
                current_page[atlas_idx + 3] = mask; // A
            }
        }
    } else if (g->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
        // 1-bit per pixel, each byte is 8 pixels
        for (int j = 0; j < g->bitmap.rows; ++j) {
            for (int i = 0; i < g->bitmap.width; ++i) {
                int atlas_idx = 4 * ((_current_x + i) + (_current_y + j) * page_width);
                int byte = g->bitmap.buffer[j * g->bitmap.pitch + (i >> 3)];
                int bit = 7 - (i & 7);
                unsigned char mask = (byte & (1 << bit)) ? 255 : 0;
                current_page[atlas_idx + 0] = 255; // R
                current_page[atlas_idx + 1] = 255; // G
                current_page[atlas_idx + 2] = 255; // B
                current_page[atlas_idx + 3] = mask; // A
            }
        }
    } else {
        utils::log_warn("Unsupported pixel mode %d for glyph U+%04X", g->bitmap.pixel_mode, codepoint);
        return false;
    }
    
    // create glyph info
    glyph_info info;
    info.u0 = float(_current_x) / page_width;
    info.v0 = float(_current_y) / page_width;
    info.u1 = float(_current_x + g->bitmap.width) / page_width;
    info.v1 = float(_current_y + g->bitmap.rows) / page_width;
    info.width = g->bitmap.width;
    info.height = g->bitmap.rows;
    info.advance = g->advance.x >> 6;
    info.bearingX = g->bitmap_left;
    info.bearingY = g->bitmap_top;  // keep the actual bearingY
    info.codepoint = codepoint;
    info.colored = _colored;
    
    // store page number in a different way - we'll use a separate field or encode it differently
    // for now, let's not overwrite bearingY
    
    _glyphs[codepoint] = info;
    
    // update position
    _current_x += g->bitmap.width + 1;
    if (g->bitmap.rows > _current_row_height) {
        _current_row_height = g->bitmap.rows;
    }
    
    // update the atlas bitmap with the new glyph data
    _atlas_bitmap = _atlas_pages[_current_page];
    
    // update the D3D11 texture with the new atlas data
#ifdef _WIN32
    // we need to get the device from somewhere - for now, we'll update it later
    // this is a temporary solution - ideally we'd pass the device to ensure_glyph
#endif
    
    return true;
}

void font::set_opentype_features(const opentype_features& features) {
    _ot_features = features;
    // real implementation: configure shaping engine (e.g., harfbuzz) with these features
}

std::vector<std::shared_ptr<font>> font::load_all_from_folder(const std::string& folder, float size, bool sdf, bool mcsdf, resources::texture_dict* tex_dict) {
    std::vector<std::shared_ptr<font>> fonts;
    namespace fs = std::filesystem;
    for (const auto& entry : fs::directory_iterator(folder)) {
        if (!entry.is_regular_file()) continue;
        auto ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".ttf" || ext == ".otf") {
            auto f = std::make_shared<font>(entry.path().string().c_str(), size, sdf, mcsdf);
#ifdef _WIN32
            if (f->load(nullptr, tex_dict)) fonts.push_back(f);
#else
            if (f->load(tex_dict)) fonts.push_back(f);
#endif
        }
    }
    return fonts;
}



} // namespace resources 