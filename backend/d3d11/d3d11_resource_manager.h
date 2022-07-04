#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include "../../core/resource_manager.h"

namespace backend::d3d11 {

struct buffer_node {
    std::unique_ptr<core::buffer> active_buffer;
    std::vector<std::pair<size_t, size_t>> child_buffers; // priority, index
    size_t priority;
};

class d3d11_resource_manager : public core::resource_manager {
public:
    d3d11_resource_manager(ID3D11Device* device, ID3D11DeviceContext* context);
    ~d3d11_resource_manager() override;

    // buffer management
    size_t register_buffer(size_t init_priority) override;
    size_t register_child_buffer(size_t parent, size_t priority) override;
    void update_child_priority(size_t child_idx, size_t new_priority) override;
    void update_buffer_priority(size_t buffer_idx, size_t new_priority) override;
    void remove_buffer(size_t idx) override;
    core::buffer* get_buffer(size_t idx) override;
    void swap_buffers(size_t idx) override;
    
    // font management
    resources::font* add_font(const char* file, float size, bool italic, bool bold, int rasterizer_flags) override;
    void remove_font(const resources::font* font_ptr) override;
    
    // matrix operations
    void update_matrix_translate(size_t buffer, const core::position& xy_translate, size_t cmd_idx) override;
    
    // initialization
    void init() override;

    // rendering
    void draw();

private:
    Microsoft::WRL::ComPtr<ID3D11Device> _device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context;
    
    std::vector<buffer_node> _buffer_list;
    std::vector<std::pair<size_t, size_t>> _priorities; // priority, index
    std::mutex _list_mutex;
    
    // font management
    std::unordered_map<std::string, std::unique_ptr<resources::font>> _fonts;
    std::mutex _font_mutex;
    
    void rebuild_priorities();
    void remove_buffer_recursive(size_t idx);
};

} // namespace backend::d3d11 