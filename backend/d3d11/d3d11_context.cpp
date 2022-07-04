#include "d3d11_context.h"
#include <cassert>
#include <fstream>
#include "../../utils/logger.h"
#include <d3d11.h>

namespace backend::d3d11 {

std::vector<char> d3d11_context::load_shader_blob(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return {};
    std::vector<char> blob(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(blob.data(), blob.size());
    return blob;
}

d3d11_context::d3d11_context() = default;
d3d11_context::~d3d11_context() = default;

Microsoft::WRL::ComPtr<ID3D11BlendState> _blend_state;

void d3d11_context::initialize(int width, int height, HWND hwnd) {
    _width = width;
    _height = height;
    // create device, context, swapchain, rtv
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Width = width;
    scd.BufferDesc.Height = height;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &scd,
        &_swapchain, &_device, nullptr, &_context);
    if (FAILED(hr)) {
        utils::log_error("D3D11CreateDeviceAndSwapChain failed: 0x%08X", hr);
    }

    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    hr = _swapchain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) {
        utils::log_error("GetBuffer failed: 0x%08X", hr);
    }
    hr = _device->CreateRenderTargetView(backBuffer.Get(), nullptr, &_rtv);
    if (FAILED(hr)) {
        utils::log_error("CreateRenderTargetView failed: 0x%08X", hr);
    }

    _tex_dict = std::make_unique<d3d11_texture_dict>(_device.Get());
    _resource_manager = std::make_unique<d3d11_resource_manager>(_device.Get(), _context.Get());

    // load shaders
    auto vs_blob = load_shader_blob("resources/shaders/vertex/generic.cso");
    auto ps_blob = load_shader_blob("resources/shaders/pixel/generic.cso");
    auto ps_color_blob = load_shader_blob("resources/shaders/pixel/color_only.cso");
    auto vs_fallback_blob = load_shader_blob("resources/shaders/vertex/fallback.cso");
    auto ps_fallback_blob = load_shader_blob("resources/shaders/pixel/fallback.cso");
    
    // create vertex shader
    hr = _device->CreateVertexShader(vs_blob.data(), vs_blob.size(), nullptr, &_vs);
    if (FAILED(hr)) {
        utils::log_error("CreateVertexShader failed: 0x%08X", hr);
        return;
    }
    
    // create pixel shader
    hr = _device->CreatePixelShader(ps_blob.data(), ps_blob.size(), nullptr, &_ps);
    if (FAILED(hr)) {
        utils::log_error("CreatePixelShader failed: 0x%08X", hr);
        return;
    }
    
    // create color-only pixel shader
    hr = _device->CreatePixelShader(ps_color_blob.data(), ps_color_blob.size(), nullptr, &_ps_color_only);
    if (FAILED(hr)) {
        utils::log_error("CreatePixelShader (color_only) failed: 0x%08X", hr);
        return;
    }
    
    // create fallback shaders (these should always work)
    if (!vs_fallback_blob.empty()) {
        hr = _device->CreateVertexShader(vs_fallback_blob.data(), vs_fallback_blob.size(), nullptr, &_vs_fallback);
        if (FAILED(hr)) {
            utils::log_warn("CreateVertexShader (fallback) failed: 0x%08X", hr);
        }
    }
    
    if (!ps_fallback_blob.empty()) {
        hr = _device->CreatePixelShader(ps_fallback_blob.data(), ps_fallback_blob.size(), nullptr, &_ps_fallback);
        if (FAILED(hr)) {
            utils::log_warn("CreatePixelShader (fallback) failed: 0x%08X", hr);
        }
    }
    
    // initialize current shader to generic shader
    _current_ps = _ps.Get();
    // input layout: float3 pos, float4 col, float2 uv
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    hr = _device->CreateInputLayout(layout, 3, vs_blob.data(), vs_blob.size(), &_input_layout);
    if (FAILED(hr)) {
        utils::log_error("CreateInputLayout failed: 0x%08X", hr);
    }

    // create sampler state
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = _device->CreateSamplerState(&samplerDesc, &_sampler);
    if (FAILED(hr)) {
        utils::log_error("CreateSamplerState failed: 0x%08X", hr);
    }

    // create matrix constant buffer
    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.ByteWidth = sizeof(float) * 16;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = _device->CreateBuffer(&cbd, nullptr, &_matrix_cb);
    if (FAILED(hr)) {
        utils::log_error("CreateBuffer (matrix_cb) failed: 0x%08X", hr);
    }

    // create blend state for alpha blending
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    hr = _device->CreateBlendState(&blendDesc, &_blend_state);
    if (FAILED(hr)) {
        utils::log_error("CreateBlendState failed: 0x%08X", hr);
    }

    // initialize projection matrix
    update_projection_matrix();
}

void d3d11_context::resize(int width, int height) {
    _width = width;
    _height = height;
    if (_rtv) _rtv.Reset();
    _swapchain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    _swapchain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    _device->CreateRenderTargetView(backBuffer.Get(), nullptr, &_rtv);
}

void d3d11_context::begin_frame() {
    // set render target
    _context->OMSetRenderTargets(1, _rtv.GetAddressOf(), nullptr);
    // set viewport
    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = static_cast<float>(_width);
    vp.Height = static_cast<float>(_height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    _context->RSSetViewports(1, &vp);
    // clear
    float clearColor[4] = {0, 0, 0, 1};
    _context->ClearRenderTargetView(_rtv.Get(), clearColor);

    // set blend state for alpha blending
    float blendFactor[4] = {0, 0, 0, 0};
    _context->OMSetBlendState(_blend_state.Get(), blendFactor, 0xFFFFFFFF);

    // update projection matrix and constant buffer
    update_projection_matrix();
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = _context->Map(_matrix_cb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr)) {
        memcpy(mappedResource.pData, _projection_matrix, sizeof(float) * 16);
        _context->Unmap(_matrix_cb.Get(), 0);
    } else {
        utils::log_error("Map matrix buffer failed: 0x%08X", hr);
    }
    _context->VSSetConstantBuffers(0, 1, _matrix_cb.GetAddressOf());
}

void d3d11_context::end_frame() {
    _swapchain->Present(1, 0);
    if (_tex_dict) _tex_dict->process_update_queue(_context.Get());
}

void d3d11_context::buffer(const core::buffer* buf) {
    if (!buf || buf->vertices.empty() || buf->indices.empty()) {
        utils::log_warn("buffer: buffer is empty or invalid");
        return;
    }

    // Create single vertex and index buffer for all geometry
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.ByteWidth = UINT(buf->vertices.size() * sizeof(core::vertex));
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = buf->vertices.data();

    Microsoft::WRL::ComPtr<ID3D11Buffer> vbo;
    HRESULT hr = _device->CreateBuffer(&vbDesc, &vbData, &vbo);
    if (FAILED(hr)) {
        utils::log_error("CreateBuffer (vertex) failed: 0x%08X", hr);
        return;
    }

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.Usage = D3D11_USAGE_DYNAMIC;
    ibDesc.ByteWidth = UINT(buf->indices.size() * sizeof(uint32_t));
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = buf->indices.data();

    Microsoft::WRL::ComPtr<ID3D11Buffer> ibo;
    hr = _device->CreateBuffer(&ibDesc, &ibData, &ibo);
    if (FAILED(hr)) {
        utils::log_error("CreateBuffer (index) failed: 0x%08X", hr);
        return;
    }

    // Set common state
    _context->IASetInputLayout(_input_layout.Get());
    UINT stride = sizeof(core::vertex);
    UINT offset = 0;
    _context->IASetVertexBuffers(0, 1, vbo.GetAddressOf(), &stride, &offset);
    _context->IASetIndexBuffer(ibo.Get(), DXGI_FORMAT_R32_UINT, 0);
    _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _context->PSSetSamplers(0, 1, _sampler.GetAddressOf());

    size_t index_offset = 0;
    for (const auto& cmd : buf->cmds) {
        if (cmd.elem_count == 0) continue;

        // Set shader based on command type
        switch (cmd.type) {
            case core::geometry_type::color_only:
                _context->VSSetShader(_vs.Get(), nullptr, 0);
                _context->PSSetShader(_ps_color_only.Get(), nullptr, 0);
                break;
                
            case core::geometry_type::textured:
                _context->VSSetShader(_vs.Get(), nullptr, 0);
                _context->PSSetShader(_ps.Get(), nullptr, 0);
                break;
                
            case core::geometry_type::font_atlas:
                // Use generic shaders for font rendering
                _context->VSSetShader(_vs.Get(), nullptr, 0);
                _context->PSSetShader(_ps.Get(), nullptr, 0);
                break;
                
            default:
                // fallback to generic shaders
                _context->VSSetShader(_vs.Get(), nullptr, 0);
                _context->PSSetShader(_ps.Get(), nullptr, 0);
                break;
        }

        // Bind appropriate texture based on command type
        if (cmd.type == core::geometry_type::font_atlas) {
            // For font commands, bind the font atlas texture
            // utils::log_info("Processing font command: elem_count=%u, font_texture=%s", 
            //                cmd.elem_count, cmd.font_texture ? "true" : "false");
            
            if (cmd.font_texture) {
                auto font = cmd.font;
                if (font && font->get_atlas_srv()) {
                    // ensure font atlas texture is updated with any new glyphs before binding
                    // this is necessary for Unicode glyphs to display properly
                    font->update_atlas_texture(_device.Get());
                    
                    // utils::log_debug("Binding font atlas SRV for font");
                    ID3D11ShaderResourceView* srv = font->get_atlas_srv();
                    _context->PSSetShaderResources(0, 1, &srv);
                    utils::log_debug("context: bound atlas for '%s'", font->path().c_str());
                } else {
                    utils::log_warn("context: font atlas SRV not available");
                }
            }
        } else if (cmd.type == core::geometry_type::textured) {
            // For textured commands, bind the current texture from stack
            if (cmd.native_texture) {
                auto tex = cmd.texture;
                if (tex) {
                    // Get the D3D11 SRV from the texture
                    ID3D11ShaderResourceView* srv = tex->get_srv();
                    if (srv) {
                        // utils::log_debug("Binding texture SRV for textured command");
                        _context->PSSetShaderResources(0, 1, &srv);
                        utils::log_debug("context: bound texture SRV");
                    } else {
                        utils::log_warn("context: texture SRV not available");
                    }
                }
            }
        }

        // Draw this command's geometry
        _context->DrawIndexed(cmd.elem_count, static_cast<UINT>(index_offset), 0);
        index_offset += cmd.elem_count;
    }
}

// RAII resource scope implementation
d3d11_context::resource_scope::resource_scope(d3d11_context* context)
    : _context(context), _previous_texture(nullptr), _previous_font(nullptr) {
}

d3d11_context::resource_scope::~resource_scope() {
    // restore previous state
    if (_context) {
        if (_previous_texture) {
            _context->set_texture(_previous_texture, 0);
        }
        // note: font binding restoration would need additional implementation
    }
}

void d3d11_context::resource_scope::bind_texture(resources::tex texture) {
    if (_context) {
        _previous_texture = texture; // store current for restoration
        _context->set_texture(texture, 0);
    }
}

void d3d11_context::resource_scope::bind_font(std::shared_ptr<resources::font> font) {
    if (_context && font) {
        _previous_font = font; // store current for restoration
        // bind font atlas texture

        ID3D11ShaderResourceView* srv = font->get_atlas_srv();
        if (&srv) {
            _context->_context->PSSetShaderResources(0, 1, &srv);
        }
    }
}

std::unique_ptr<d3d11_context::resource_scope> d3d11_context::create_resource_scope() {
    return std::make_unique<resource_scope>(this);
}

void d3d11_context::set_texture(resources::tex tex, uint32_t slot) {
    auto d3d_tex = std::dynamic_pointer_cast<d3d11_texture>(tex);
    if (d3d_tex) {
        ID3D11ShaderResourceView* srv = d3d_tex->srv();
        _context->PSSetShaderResources(slot, 1, &srv);
    } else {
        utils::log_error("set_texture: slot=%d, tex=%p, cast failed\n", slot, tex.get());
    }
}

void d3d11_context::set_pixel_shader(const std::string& shader_name) {
    if (shader_name == "color_only") {
        if (_ps_color_only) {
            _context->PSSetShader(_ps_color_only.Get(), nullptr, 0);
            _current_ps = _ps_color_only.Get();
        } else {
            utils::log_warn("color_only shader not available, using fallback");
            if (_ps_fallback) {
                _context->PSSetShader(_ps_fallback.Get(), nullptr, 0);
                _current_ps = _ps_fallback.Get();
            }
        }
    } else {
        if (_ps) {
            _context->PSSetShader(_ps.Get(), nullptr, 0);
            _current_ps = _ps.Get();
        } else {
            utils::log_warn("generic shader not available, using fallback");
            if (_ps_fallback) {
                _context->PSSetShader(_ps_fallback.Get(), nullptr, 0);
                _current_ps = _ps_fallback.Get();
            }
        }
    }
}

void d3d11_context::set_font_atlas(ID3D11ShaderResourceView* srv) {
    if (srv) {
        _context->PSSetShaderResources(0, 1, &srv);
    } else {
        utils::log_error("Font atlas SRV is null");
    }
}

void d3d11_context::clear(const core::color& col) {
    float clearColor[4] = {col.x, col.y, col.z, col.w};
    _context->ClearRenderTargetView(_rtv.Get(), clearColor);
}

void d3d11_context::update_projection_matrix() {
    // create orthographic projection matrix
    // maps screen coordinates (0,0) to (-1,-1) and (width,height) to (1,1)
    float w = static_cast<float>(_width);
    float h = static_cast<float>(_height);
    
    // orthographic projection matrix (row-major)
    _projection_matrix[0] = 2.0f / w;  _projection_matrix[1] = 0.0f;        _projection_matrix[2] = 0.0f;  _projection_matrix[3] = 0.0f;
    _projection_matrix[4] = 0.0f;      _projection_matrix[5] = -2.0f / h;    _projection_matrix[6] = 0.0f;  _projection_matrix[7] = 0.0f;
    _projection_matrix[8] = 0.0f;      _projection_matrix[9] = 0.0f;        _projection_matrix[10] = 1.0f; _projection_matrix[11] = 0.0f;
    _projection_matrix[12] = -1.0f;    _projection_matrix[13] = 1.0f;       _projection_matrix[14] = 0.0f; _projection_matrix[15] = 1.0f;
}

} // namespace backend::d3d11 