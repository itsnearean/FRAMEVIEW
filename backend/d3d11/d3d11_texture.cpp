#include "d3d11_texture.h"
#include <algorithm>
#include <cstring>
#include <cassert>
#include "../../utils/logger.h"

namespace backend::d3d11 {

d3d11_texture::d3d11_texture(ID3D11Device* device, uint32_t width, uint32_t height)
    : _device(device), _width(width), _height(height) {
    if (width > 0 && height > 0) {
        create();
    }
}

d3d11_texture::d3d11_texture(ID3D11Device* device, ID3D11Texture2D* existing_texture, ID3D11ShaderResourceView* existing_srv)
    : _device(device), _texture(existing_texture), _srv(existing_srv) {
    if (existing_texture) {
        D3D11_TEXTURE2D_DESC desc;
        existing_texture->GetDesc(&desc);
        _width = desc.Width;
        _height = desc.Height;
    }
}

d3d11_texture::~d3d11_texture() {
    invalidate();
}

void d3d11_texture::create() {
    if (!_device || _width == 0 || _height == 0) {
        utils::log_error("create() called with invalid params: device=%p, width=%d, height=%d\n", _device, _width, _height);
        return;
    }
    
    _texture.Reset();
    _srv.Reset();

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = _width;
    desc.Height = _height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

    HRESULT res = _device->CreateTexture2D(&desc, nullptr, &_texture);
    if (FAILED(res)) {
        utils::log_error("CreateTexture2D failed: 0x%08X", res);
        return;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = desc.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;
    res = _device->CreateShaderResourceView(_texture.Get(), &srv_desc, &_srv);
    if (FAILED(res)) {
        utils::log_error("CreateShaderResourceView failed: 0x%08X\n", res);
        return;
    }
    
}

bool d3d11_texture::set_data(const uint8_t* data, uint32_t width, uint32_t height) {
    _data.resize(width * height * 4);
    
    // Convert RGBA to BGRA format for D3D11 compatibility
    // D3D11 expects BGRA format (DXGI_FORMAT_R8G8B8A8_UNORM)
    for (uint32_t i = 0; i < width * height; ++i) {
        uint32_t src_idx = i * 4;
        uint32_t dst_idx = i * 4;
        _data[dst_idx + 0] = data[src_idx + 2];  // B (from R)
        _data[dst_idx + 1] = data[src_idx + 1];  // G
        _data[dst_idx + 2] = data[src_idx + 0];  // R (from B)
        _data[dst_idx + 3] = data[src_idx + 3];  // A
    }
    
    bool changed = (_width != width) || (_height != height);
    _width = width;
    _height = height;
    if (!_texture || changed) {
        create();
    }
    _dirty = true;
    return true;
}

void d3d11_texture::bind(uint32_t slot) {
    // This method is deprecated - use get_srv() instead
    // The renderer should handle actual binding
}

void d3d11_texture::unbind() {
    // This method is deprecated - use get_srv() instead
    // The renderer should handle actual unbinding
}

bool d3d11_texture::apply_changes() {
    // this function should queue the texture for update
    // user must call process_update_queue on the dict with a valid context
    // (see tex_wrapper_dx11::set_tex_data logic)
    // this is a placeholder; actual upload is done in process_update_queue
    _dirty = false;
    return true;
}

bool d3d11_texture::get_size(uint32_t& width, uint32_t& height) const {
    width = _width;
    height = _height;
    return true;
}

void d3d11_texture::clear_data() {
    _data.clear();
    _width = _height = 0;
    invalidate();
}

void d3d11_texture::invalidate() {
    _texture.Reset();
    _srv.Reset();
}

bool d3d11_texture::copy_texture_data(ID3D11DeviceContext* ctx) {
    if (!_texture || _data.empty()) {
        utils::log_error("copy_texture_data failed: texture=%p, data_empty=%d\n", _texture.Get(), _data.empty());
        return false;
    }
    D3D11_BOX box = {};
    box.left = 0;
    box.top = 0;
    box.front = 0;
    box.back = 1;
    box.right = _width;
    box.bottom = _height;
    ctx->UpdateSubresource(_texture.Get(), 0, &box, _data.data(), _width * 4, _width * _height * 4);
    _dirty = false;
    return true;
}

void d3d11_texture::request_update(d3d11_texture_dict* dict) {
    if (dict) dict->queue_update(this);
}

// --- d3d11_texture_dict ---

d3d11_texture_dict::d3d11_texture_dict(ID3D11Device* device)
    : _device(device) {}

d3d11_texture_dict::~d3d11_texture_dict() {
    clear_textures();
}

resources::tex d3d11_texture_dict::create_texture(uint32_t width, uint32_t height) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto tex = std::make_shared<d3d11_texture>(_device.Get(), width, height);
    _textures.push_back(tex);
    return tex;
}

resources::tex d3d11_texture_dict::create_texture_from_d3d11(ID3D11Texture2D* d3d_texture, ID3D11ShaderResourceView* srv) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto tex = std::make_shared<d3d11_texture>(_device.Get(), d3d_texture, srv);
    _textures.push_back(tex);
    return tex;
}

void d3d11_texture_dict::destroy_texture(resources::tex tex) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = std::remove(_textures.begin(), _textures.end(), tex);
    _textures.erase(it, _textures.end());
}

bool d3d11_texture_dict::set_texture_data(resources::tex tex, const uint8_t* data, uint32_t width, uint32_t height) {
    auto d3d_tex = std::dynamic_pointer_cast<d3d11_texture>(tex);
    if (!d3d_tex) {
        utils::log_error("set_texture_data: dynamic_pointer_cast failed");
        return false;
    }
    bool result = d3d_tex->set_data(data, width, height);
    queue_update(d3d_tex.get());
    return result;
}

bool d3d11_texture_dict::get_texture_size(resources::tex tex, uint32_t& width, uint32_t& height) {
    auto d3d_tex = std::dynamic_pointer_cast<d3d11_texture>(tex);
    if (!d3d_tex) return false;
    return d3d_tex->get_size(width, height);
}

void d3d11_texture_dict::clear_textures() {
    std::lock_guard<std::mutex> lock(_mutex);
    _textures.clear();
    std::lock_guard<std::mutex> qlock(_update_queue_mutex);
    _update_queue.clear();
}

void d3d11_texture_dict::log_memory_stats() const {
    std::lock_guard<std::mutex> lock(_mutex);
    std::lock_guard<std::mutex> qlock(_update_queue_mutex);
    
    utils::log_info("Texture memory stats: textures=%zu, update_queue=%zu", 
                   _textures.size(), _update_queue.size());
    
    // log individual texture info
    for (size_t i = 0; i < _textures.size(); ++i) {
        if (auto tex = std::dynamic_pointer_cast<d3d11_texture>(_textures[i])) {
            utils::log_info("  Texture[%zu]: %ux%u, dirty=%s", 
                           i, tex->width(), tex->height(), 
                           tex->_dirty ? "true" : "false");
        }
    }
}

void d3d11_texture_dict::pre_reset() {
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& tex : _textures) {
        auto d3d_tex = std::dynamic_pointer_cast<d3d11_texture>(tex);
        if (d3d_tex) d3d_tex->invalidate();
    }
}

void d3d11_texture_dict::post_reset() {
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& tex : _textures) {
        auto d3d_tex = std::dynamic_pointer_cast<d3d11_texture>(tex);
        if (d3d_tex) d3d_tex->create();
    }
}

void d3d11_texture_dict::queue_update(d3d11_texture* tex) {
    if (!tex) {
        utils::log_error("queue_update: tex is null");
        return;
    }
    std::lock_guard<std::mutex> lock(_update_queue_mutex);
    if (std::find(_update_queue.begin(), _update_queue.end(), tex) == _update_queue.end()) {
        _update_queue.push_back(tex);
    } else {
        utils::log_warn("queue_update: texture %p already in queue", tex);
    }
}

void d3d11_texture_dict::process_update_queue(ID3D11DeviceContext* ctx) {
    std::lock_guard<std::mutex> lock(_update_queue_mutex);
    for (auto* tex : _update_queue) {
        if (tex && tex->_dirty) {
            tex->copy_texture_data(ctx);
        }
    }
    _update_queue.clear();
}

} // namespace backend::d3d11
