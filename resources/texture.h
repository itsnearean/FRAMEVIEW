#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <mutex>
#ifdef _WIN32
#include <d3d11.h>
#endif

namespace resources {

class texture {
public:
    virtual ~texture() = default;
    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;
    virtual void bind(uint32_t slot = 0) = 0;
    virtual void unbind() = 0;

    virtual bool set_data(const uint8_t* data, uint32_t width, uint32_t height) = 0;
    virtual bool apply_changes() = 0;
    virtual bool get_size(uint32_t& width, uint32_t& height) const = 0;
    virtual void clear_data() = 0;
    virtual void invalidate() = 0;
    virtual void create() = 0;
#ifdef _WIN32
    virtual ID3D11ShaderResourceView* get_srv() const = 0;
#endif
};

using tex = std::shared_ptr<texture>;

class texture_dict {
public:
    virtual ~texture_dict() = default;

    virtual tex create_texture(uint32_t width, uint32_t height) = 0;
#ifdef _WIN32
    virtual tex create_texture_from_d3d11(ID3D11Texture2D* d3d_texture, ID3D11ShaderResourceView* srv = nullptr) = 0;
#endif
    virtual void destroy_texture(tex tex) = 0;
    virtual bool set_texture_data(tex tex, const uint8_t* data, uint32_t width, uint32_t height) = 0;
    virtual bool get_texture_size(tex tex, uint32_t& width, uint32_t& height) = 0;
    virtual void clear_textures() = 0;
    virtual void pre_reset() = 0;
    virtual void post_reset() = 0;
};

using texture_dict_ptr = std::shared_ptr<texture_dict>;

} // namespace resources
