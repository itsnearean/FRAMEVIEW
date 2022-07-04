#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <string>
#include "../../core/context.h"
#include "d3d11_texture.h"
#include "d3d11_resource_manager.h"

namespace backend::d3d11 {

class d3d11_context : public core::context {
public:
    d3d11_context();
    ~d3d11_context() override;

    void initialize(int width, int height, HWND hwnd) override;
    void resize(int width, int height) override;
    void begin_frame() override;
    void end_frame() override;

    void buffer(const core::buffer* buf) override;
    void set_texture(resources::tex tex, uint32_t slot = 0) override;
    void set_font_atlas(ID3D11ShaderResourceView* srv) override;
    void set_pixel_shader(const std::string& shader_name) override;
    void clear(const core::color& col) override;

    // expose device/context for advanced usage
    ID3D11Device* device() const { return _device.Get(); }
    ID3D11DeviceContext* context() const { return _context.Get(); }
    IDXGISwapChain* swapchain() const { return _swapchain.Get(); }
    d3d11_texture_dict* texture_dict() { return _tex_dict.get(); }
    
    // draw manager access
    d3d11_resource_manager* resource_manager() { return _resource_manager.get(); }

    // RAII wrapper for D3D11 resources
    class resource_scope {
    public:
        resource_scope(d3d11_context* context);
        ~resource_scope();
        resource_scope(const resource_scope&) = delete;
        resource_scope& operator=(const resource_scope&) = delete;
        
        // bind texture for this scope
        void bind_texture(resources::tex texture);
        // bind font for this scope
        void bind_font(std::shared_ptr<resources::font> font);
        
    private:
        d3d11_context* _context;
        resources::tex _previous_texture;
        std::shared_ptr<resources::font> _previous_font;
    };
    
    // helper method for automatic resource management
    std::unique_ptr<resource_scope> create_resource_scope();

private:
    Microsoft::WRL::ComPtr<ID3D11Device> _device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context;
    Microsoft::WRL::ComPtr<IDXGISwapChain> _swapchain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _rtv;
    std::unique_ptr<d3d11_texture_dict> _tex_dict;
    std::unique_ptr<d3d11_resource_manager> _resource_manager;
    int _width = 0, _height = 0;
    float _projection_matrix[16];

    // shader and input layout
    Microsoft::WRL::ComPtr<ID3D11VertexShader> _vs;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> _ps;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> _ps_color_only;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> _ps_fallback;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> _vs_fallback;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> _input_layout;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> _sampler;
    Microsoft::WRL::ComPtr<ID3D11Buffer> _matrix_cb;
    Microsoft::WRL::ComPtr<ID3D11BlendState> _blend_state;
    
    // current shader state
    ID3D11PixelShader* _current_ps = nullptr;

    std::vector<char> load_shader_blob(const std::string& path);
    void update_projection_matrix();
};

} // namespace backend::d3d11 