#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include <memory>
#include <mutex>
#include <algorithm>
#include "../../resources/texture.h"

namespace backend::d3d11 {

class d3d11_texture;

class d3d11_texture : public resources::texture {
public:
    d3d11_texture(ID3D11Device* device, uint32_t width, uint32_t height);
    d3d11_texture(ID3D11Device* device, ID3D11Texture2D* existing_texture, ID3D11ShaderResourceView* existing_srv = nullptr);
    ~d3d11_texture() override;

    bool set_data(const uint8_t* data, uint32_t width, uint32_t height) override;
    bool apply_changes() override;
    bool get_size(uint32_t& width, uint32_t& height) const override;
    void clear_data() override;
    void invalidate() override;
    void create() override;

    // request this texture to be updated in the dict's update queue
    void request_update(class d3d11_texture_dict* dict);
    // upload data to GPU (call from render thread)
    bool copy_texture_data(ID3D11DeviceContext* ctx);

    ID3D11ShaderResourceView* srv() const { return _srv.Get(); }
    uint32_t width() const override { return _width; }
    uint32_t height() const override { return _height; }
    void bind(uint32_t slot = 0) override;
    void unbind() override;
    ID3D11ShaderResourceView* get_srv() const override { return _srv.Get(); }

    bool _dirty = false;
private:
    Microsoft::WRL::ComPtr<ID3D11Device> _device;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> _texture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _srv;
    std::vector<uint8_t> _data;
    uint32_t _width = 0, _height = 0;
};

class d3d11_texture_dict : public resources::texture_dict {
public:
    d3d11_texture_dict(ID3D11Device* device);
    ~d3d11_texture_dict() override;

    resources::tex create_texture(uint32_t width, uint32_t height) override;
    resources::tex create_texture_from_d3d11(ID3D11Texture2D* d3d_texture, ID3D11ShaderResourceView* srv = nullptr);
    void destroy_texture(resources::tex tex) override;
    bool set_texture_data(resources::tex tex, const uint8_t* data, uint32_t width, uint32_t height) override;
    bool get_texture_size(resources::tex tex, uint32_t& width, uint32_t& height) override;
    void clear_textures() override;
    void pre_reset() override;
    void post_reset() override;
    
    // memory tracking
    size_t texture_count() const { return _textures.size(); }
    size_t update_queue_size() const { return _update_queue.size(); }
    void log_memory_stats() const;

    // queue a texture for update
    void queue_update(d3d11_texture* tex);
    // process all queued updates with the given context
    void process_update_queue(ID3D11DeviceContext* ctx);

private:
    Microsoft::WRL::ComPtr<ID3D11Device> _device;
    std::vector<resources::tex> _textures;
    mutable std::mutex _mutex;
    mutable std::mutex _update_queue_mutex;
    std::vector<d3d11_texture*> _update_queue;
};

} // namespace backend::d3d11 