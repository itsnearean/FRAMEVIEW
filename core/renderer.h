#pragma once
#include <memory>
#include <string>
#include <cstdint>
#include "draw_types.h"
#include "draw_buffer.h"
#include "draw_manager.h"
#include "../resources/texture.h"

namespace core {

class renderer {
public:
    virtual ~renderer() = default;

    virtual void initialize(int width, int height, HWND hwnd) = 0;
    virtual void resize(int width, int height) = 0;
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;

    virtual void draw_buffer(const draw_buffer* buf) = 0;
    virtual void set_texture(resources::tex tex, uint32_t slot = 0) = 0;
    virtual void set_font_atlas(ID3D11ShaderResourceView* srv) = 0;
    virtual void set_pixel_shader(const std::string& shader_name) = 0;
    virtual void clear(const color& col) = 0;
};

using renderer_ptr = std::shared_ptr<renderer>;

} // namespace core 