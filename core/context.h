#pragma once
#include <memory>
#include <string>
#include <cstdint>
#include "types.h"
#include "buffer.h"
#include "resource_manager.h"
#include "../resources/texture.h"

namespace core {

class context {
public:
    virtual ~context() = default;

    // initialization
    virtual void initialize(int width, int height, HWND hwnd) = 0;
    
    // pipeline
    virtual void clear(const color& col) = 0;
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
    virtual void buffer(const buffer* buf) = 0;
    
    //graphics state
    virtual void resize(int width, int height) = 0;
    virtual void set_texture(resources::tex tex, uint32_t slot = 0) = 0;
    virtual void set_font_atlas(ID3D11ShaderResourceView* srv) = 0;
    virtual void set_pixel_shader(const std::string& shader_name) = 0;
};

using context_ptr = std::shared_ptr<context>;

} // namespace core 