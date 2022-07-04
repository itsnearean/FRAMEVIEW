#include "d3d11_resource_manager.h"
#include "../../core/buffer.h"
#include "../../resources/font.h"
#include <algorithm>
#include <cassert>

namespace backend::d3d11 {

d3d11_resource_manager::d3d11_resource_manager(ID3D11Device* device, ID3D11DeviceContext* context)
    : _device(device), _context(context) {
    init();
}

d3d11_resource_manager::~d3d11_resource_manager() = default;

void d3d11_resource_manager::init() {
    // initialize any required resources
}

size_t d3d11_resource_manager::register_buffer(size_t init_priority) {
    std::lock_guard<std::mutex> lock(_list_mutex);
    
    buffer_node node;
    node.active_buffer = std::make_unique<core::buffer>();
    node.priority = init_priority;
    
    size_t index = _buffer_list.size();
    _buffer_list.push_back(std::move(node));
    _priorities.push_back({init_priority, index});
    
    // sort priorities
    std::sort(_priorities.begin(), _priorities.end());
    
    return index;
}

size_t d3d11_resource_manager::register_child_buffer(size_t parent, size_t priority) {
    std::lock_guard<std::mutex> lock(_list_mutex);
    
    if (parent >= _buffer_list.size()) return SIZE_MAX;
    
    size_t child_index = register_buffer(priority);
    if (child_index != SIZE_MAX) {
        _buffer_list[parent].child_buffers.push_back({priority, child_index});
        std::sort(_buffer_list[parent].child_buffers.begin(), _buffer_list[parent].child_buffers.end());
    }
    
    return child_index;
}

void d3d11_resource_manager::update_child_priority(size_t child_idx, size_t new_priority) {
    std::lock_guard<std::mutex> lock(_list_mutex);
    
    if (child_idx >= _buffer_list.size()) return;
    
    // find parent
    for (auto& node : _buffer_list) {
        auto it = std::find_if(node.child_buffers.begin(), node.child_buffers.end(),
            [child_idx](const auto& pair) { return pair.second == child_idx; });
        
        if (it != node.child_buffers.end()) {
            it->first = new_priority;
            std::sort(node.child_buffers.begin(), node.child_buffers.end());
            break;
        }
    }
    
    // update main priority list
    auto it = std::find_if(_priorities.begin(), _priorities.end(),
        [child_idx](const auto& pair) { return pair.second == child_idx; });
    
    if (it != _priorities.end()) {
        it->first = new_priority;
        std::sort(_priorities.begin(), _priorities.end());
    }
}

void d3d11_resource_manager::update_buffer_priority(size_t buffer_idx, size_t new_priority) {
    std::lock_guard<std::mutex> lock(_list_mutex);
    
    if (buffer_idx >= _buffer_list.size()) return;
    
    _buffer_list[buffer_idx].priority = new_priority;
    
    auto it = std::find_if(_priorities.begin(), _priorities.end(),
        [buffer_idx](const auto& pair) { return pair.second == buffer_idx; });
    
    if (it != _priorities.end()) {
        it->first = new_priority;
        std::sort(_priorities.begin(), _priorities.end());
    }
}

void d3d11_resource_manager::remove_buffer(size_t idx) {
    std::lock_guard<std::mutex> lock(_list_mutex);
    remove_buffer_recursive(idx);
}

void d3d11_resource_manager::remove_buffer_recursive(size_t idx) {
    if (idx >= _buffer_list.size()) return;
    
    // remove all children first
    auto& node = _buffer_list[idx];
    for (const auto& child : node.child_buffers) {
        remove_buffer_recursive(child.second);
    }
    
    // remove from priorities
    _priorities.erase(std::remove_if(_priorities.begin(), _priorities.end(),
        [idx](const auto& pair) { return pair.second == idx; }), _priorities.end());
    
    // remove from buffer list
    _buffer_list.erase(_buffer_list.begin() + idx);
    
    // update indices in priorities and child buffers
    for (auto& pair : _priorities) {
        if (pair.second > idx) pair.second--;
    }
    
    for (auto& node : _buffer_list) {
        for (auto& child : node.child_buffers) {
            if (child.second > idx) child.second--;
        }
    }
}

core::buffer* d3d11_resource_manager::get_buffer(size_t idx) {
    std::lock_guard<std::mutex> lock(_list_mutex);
    
    if (idx >= _buffer_list.size()) return nullptr;
    return _buffer_list[idx].active_buffer.get();
}

void d3d11_resource_manager::swap_buffers(size_t idx) {
    std::lock_guard<std::mutex> lock(_list_mutex);
    
    if (idx >= _buffer_list.size()) return;
    
    // swap active buffer with a new one
    _buffer_list[idx].active_buffer = std::make_unique<core::buffer>();
}

resources::font* d3d11_resource_manager::add_font(const char* file, float size, bool italic, bool bold, int rasterizer_flags) {
    std::lock_guard<std::mutex> lock(_font_mutex);
    
    std::string key = std::string(file) + "_" + std::to_string(size) + "_" + 
                     std::to_string(italic) + "_" + std::to_string(bold) + "_" + 
                     std::to_string(rasterizer_flags);
    
    auto it = _fonts.find(key);
    if (it != _fonts.end()) {
        return it->second.get();
    }
    
    auto font = std::make_unique<resources::font>(file, size);
    auto* font_ptr = font.get();
    _fonts[key] = std::move(font);
    
    return font_ptr;
}

void d3d11_resource_manager::remove_font(const resources::font* font_ptr) {
    std::lock_guard<std::mutex> lock(_font_mutex);
    
    auto it = std::find_if(_fonts.begin(), _fonts.end(),
        [font_ptr](const auto& pair) { return pair.second.get() == font_ptr; });
    
    if (it != _fonts.end()) {
        _fonts.erase(it);
    }
}

void d3d11_resource_manager::update_matrix_translate(size_t buffer, const core::position& xy_translate, size_t cmd_idx) {
    std::lock_guard<std::mutex> lock(_list_mutex);
    
    if (buffer >= _buffer_list.size()) return;
    if (cmd_idx >= _buffer_list[buffer].active_buffer->cmds.size()) return;
    
    // update the command's matrix transform
    // this would need matrix support in draw_command
}

void d3d11_resource_manager::draw() {
    std::lock_guard<std::mutex> lock(_list_mutex);
    
    // calculate total vertex and index counts
    uint32_t total_vertices = 0;
    uint32_t total_indices = 0;
    
    for (const auto& priority_pair : _priorities) {
        const auto& node = _buffer_list[priority_pair.second];
        auto [vtx_count, idx_count] = node.active_buffer->vtx_idx_count();
        total_vertices += vtx_count;
        total_indices += idx_count;
        
        // also count children
        for (const auto& child : node.child_buffers) {
            const auto& child_node = _buffer_list[child.second];
            auto [child_vtx, child_idx] = child_node.active_buffer->vtx_idx_count();
            total_vertices += child_vtx;
            total_indices += child_idx;
        }
    }
    
    if (total_vertices == 0 || total_indices == 0) {
        return;
    }
    
    // render all buffers in priority order
    for (const auto& priority_pair : _priorities) {
        const auto& node = _buffer_list[priority_pair.second];
        
        // render main buffer
        if (node.active_buffer && !node.active_buffer->vertices.empty() && !node.active_buffer->indices.empty()) {
            // we need access to the renderer to call buffer
            // for now, we'll need to pass the renderer reference or implement rendering here
            // this is a temporary solution - ideally the resource_manager should have access to the renderer
        }
        
        // render child buffers
        for (const auto& child : node.child_buffers) {
            const auto& child_node = _buffer_list[child.second];
            if (child_node.active_buffer && !child_node.active_buffer->vertices.empty() && !child_node.active_buffer->indices.empty()) {
                // same issue as above
            }
        }
    }
}

} // namespace backend::d3d11 