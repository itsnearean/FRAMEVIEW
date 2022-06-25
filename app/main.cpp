#include <memory>
#include <iostream>
#include <windows.h>
#include <vector>
#include <string>
#include <stack>
#include "../backend/d3d11/d3d11_renderer.h"
#include "../core/renderer.h"
#include "../core/draw_types.h"
#include "../resources/font.h"
#include "../utils/logger.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

using namespace backend::d3d11;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

int main() {
    // create window
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"FRAMEVIEW";
    if (!RegisterClassW(&wc)) {
        MessageBoxA(nullptr, "Failed to register window class", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    HWND hwnd = CreateWindowExW(0, L"FRAMEVIEW", L"FRAMEVIEW", WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
                              nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    if (!hwnd) {
        MessageBoxA(nullptr, "Failed to create window", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // create renderer
    d3d11_renderer renderer;
    renderer.initialize(1280, 720, hwnd);

    // load fonts (now renderer is available)
    // auto consola = std::make_shared<resources::font>("C:\\Windows\\Fonts\\consola.ttf", 32.f);
    // auto trebuchetMS = std::make_shared<resources::font>("resources\\fonts\\trebuchetMS.ttf", 32.f);
    auto notoSans = std::make_shared<resources::font>("resources\\fonts\\NotoSans-VariableFont_wdth,wght.ttf", 32.f);
    auto notoSansSC = std::make_shared<resources::font>("resources\\fonts\\NotoSansSC-VariableFont_wght.ttf", 32.f);

    // load fonts with texture dictionary
    // consola->load(renderer.device(), renderer.texture_dict());
    // trebuchetMS->load(renderer.device(), renderer.texture_dict());
    notoSans->load(renderer.device(), renderer.texture_dict());
    notoSansSC->load(renderer.device(), renderer.texture_dict());
    
    // set up fallback font chain
    notoSansSC->add_fallback(notoSans);
    // notoSans->add_fallback(trebuchetMS);
    // trebuchetMS->add_fallback(consola);
    
    // set default fallback for any font that doesn't have specific fallbacks
    auto default_fallback = std::make_shared<resources::font>("C:\\Windows\\Fonts\\arial.ttf", 32.f);
    default_fallback->load(renderer.device(), renderer.texture_dict());
    
    notoSansSC->set_default_fallback(default_fallback);
    notoSans->set_default_fallback(default_fallback);
    // trebuchetMS->set_default_fallback(default_fallback);
    // consola->set_default_fallback(default_fallback);


    // create a test texture (checkerboard)
    auto* tex_dict = renderer.texture_dict();
    // auto tex = tex_dict->create_texture(128, 128);
    // we'll create the texture after loading the image to get the correct dimensions
    resources::tex tex = nullptr;
    int texture_width = 0, texture_height = 0;  // Store texture dimensions
    
    // std::vector<uint8_t> image_data(128 * 128 * 4);
    // for (int y = 0; y < 128; ++y) {
    //     for (int x = 0; x < 128; ++x) {
    //         int idx = (y * 128 + x) * 4;
    //         bool checker = ((x / 8) % 2) ^ ((y / 8) % 2);  // smaller squares
    //         image_data[idx + 0] = checker ? 255 : 0;    // R: white or black
    //         image_data[idx + 1] = checker ? 255 : 0;    // G: white or black  
    //         image_data[idx + 2] = checker ? 255 : 0;    // B: white or black
    //         image_data[idx + 3] = 255;                  // A: fully opaque
    //     }
    // }

    int width, height, channels;
    // Force 4 channels (RGBA) for consistent format
    unsigned char *image_data = stbi_load("resources/textures/test.png", &width, &height, &channels, 4);
    if (image_data == NULL) utils::log_warn("Failed to load image: %s\n", stbi_failure_reason());

    // Create texture with actual image dimensions
    tex = tex_dict->create_texture(width, height);
    texture_width = width;
    texture_height = height;
    
    
    tex_dict->set_texture_data(tex, image_data, width, height);
    stbi_image_free(image_data);
    
    tex_dict->process_update_queue(renderer.context());



    // create draw manager and unified buffer
    auto* draw_mgr = renderer.draw_manager();
    size_t unified_buffer_id = draw_mgr->register_buffer(0);
    auto* unified_buf = draw_mgr->get_buffer(unified_buffer_id);

    MSG msg = {};
    bool running = true;
    while (running) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) running = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // clear unified buffer for new frame
        unified_buf->clear_all();
        
        // ensure texture stack is clean for this frame
        if (unified_buf->texture_stack_depth() > 0) {
            utils::log_warn("Texture stack not empty at frame start, clearing: depth=%zu", unified_buf->texture_stack_depth());
            unified_buf->clear_texture_stack();
        }

        // colored quad (non-textured)
        unified_buf->prim_rect_multi_color({100, 100}, {300, 300},
            core::color{1.0f, 0.0f, 0.0f, 1.0f},
            core::color{0.0f, 1.0f, 0.0f, 1.0f},
            core::color{0.0f, 0.0f, 1.0f, 1.0f},
            core::color{1.0f, 1.0f, 0.0f, 1.0f});

        // demonstrate new RAII texture management
        {
            auto texture_scope = unified_buf->push_texture_scope(tex);

            float quad_width = 200.0f;  // Base width
            float quad_height = quad_width * (float)texture_height / (float)texture_width;  // Maintain aspect ratio

            // texture will be automatically popped when scope ends
            unified_buf->prim_rect_uv({350, 100}, {350 + quad_width, 100 + quad_height}, {0, 0}, {1, 1}, core::pack_color_abgr({1, 1, 1, 1}));
        } // texture automatically popped here

        
        // demonstrate texture stack validation
        // if (!unified_buf->is_texture_stack_valid()) {
        //     utils::log_warn("Texture stack validation failed");
        // }
            
        // triangle
        unified_buf->triangle_filled({600, 300}, {700, 100}, {800, 300},
            core::pack_color_abgr({0, 1, 0, 1}),
            core::pack_color_abgr({0, 0, 1, 1}),
            core::pack_color_abgr({1, 1, 0, 1}));
            
        // n-gon
        unified_buf->n_gon({950, 200}, 100, 7, core::pack_color_abgr({0.35f, 0.65f, 0, 1}));

        // line
        unified_buf->line({100, 350}, {300, 400}, core::pack_color_abgr({1, 1, 1, 1}), core::pack_color_abgr({1, 0, 1, 1}), 4.0f);

        // polyline
        std::vector<core::position> poly_points = {{350, 350}, {400, 400}, {450, 350}, {500, 400}, {550, 350}};
        unified_buf->poly_line(poly_points, core::pack_color_abgr({0, 1, 1, 1}), 3.0f, false);

        // filled circle
        unified_buf->circle_filled({700, 400}, 50, core::pack_color_abgr({1, 0, 0, 1}), core::pack_color_abgr({1, 1, 0, 1}), 48);

        // text with fallback font demonstration
        unified_buf->push_font(notoSansSC);
        
        unified_buf->text("Hello, FRAMEVIEW!", {100, 500}, core::pack_color_abgr({1, 1, 1, 1}));
        unified_buf->text("Unicode test | 你好世界 | にちは", {100, 550}, core::pack_color_abgr({1, 1, 0, 1}));
        
        // test fallback fonts with characters that might not be in the primary font (also aren't in the current nor the default fallback :D)
        unified_buf->text("Fallback test: 🚀🎮🌟", {100, 600}, core::pack_color_abgr({0, 1, 1, 1}));
        unified_buf->text("Mixed languages: こんにちは Hello 안녕하세요", {100, 650}, core::pack_color_abgr({1, 0.5f, 1, 1}));
        
        unified_buf->pop_font();
        
        // test different fonts to show fallback system
        // unified_buf->push_font(trebuchetMS);
        // unified_buf->text("Trebuchet MS font with fallbacks", {100, 700}, core::pack_color_abgr({0.5f, 1, 0.5f, 1}));
        // unified_buf->pop_font();
        
        renderer.begin_frame();
        renderer.clear(core::color{0.1f, 0.2f, 0.3f, 1.0f});
        
        // draw unified buffer (handles all geometry types automatically)
        if (unified_buf && !unified_buf->vertices.empty() && !unified_buf->indices.empty()) {
            // utils::log_info("Rendering unified buffer: %zu commands, %zu vertices, %zu indices", 
            //                unified_buf->command_count(), unified_buf->total_vertex_count(), unified_buf->total_index_count());
            renderer.draw_buffer(unified_buf);
        } else {
            utils::log_warn("Unified buffer empty: buf=%p, vertices=%zu, indices=%zu", 
                   unified_buf, unified_buf ? unified_buf->vertices.size() : 0, unified_buf ? unified_buf->indices.size() : 0);
        }
        
        // demonstrate error handling and texture stack safety
        // utils::log_info("=== Texture Stack Demo ===");
        // utils::log_info("Unified buffer texture stack depth: %zu", unified_buf->texture_stack_depth());
        
        // demonstrate safe texture operations
        // unified_buf->push_texture(nullptr); // this should be safely ignored
        // utils::log_info("After pushing null texture, stack depth: %zu", unified_buf->texture_stack_depth());
        
        // demonstrate RAII texture management
        // {
        //     auto scope1 = unified_buf->push_texture_scope(tex);
        //     utils::log_info("Inside scope 1, texture stack depth: %zu", unified_buf->texture_stack_depth());
            
        //     {
        //         auto scope2 = unified_buf->push_texture_scope(tex);
        //         utils::log_info("Inside scope 2, texture stack depth: %zu", unified_buf->texture_stack_depth());
        //     } // scope2 ends, texture popped
            
        //     utils::log_info("After scope 2 ends, texture stack depth: %zu", unified_buf->texture_stack_depth());
        // } // scope1 ends, texture popped
        
        // utils::log_info("After all scopes end, texture stack depth: %zu", unified_buf->texture_stack_depth());
        // utils::log_info("=== End Texture Stack Demo ===");

        // demonstrate fallback shader system
        // utils::log_info("=== Shader System Demo ===");
        
        // test normal shader loading
        // renderer.set_pixel_shader("generic");
        // utils::log_info("Set generic shader successfully");
        
        // test color-only shader
        // renderer.set_pixel_shader("color_only");
        // utils::log_info("Set color_only shader successfully");
        
        // test fallback behavior (this would normally fail if shaders weren't loaded)
        // renderer.set_pixel_shader("generic");
        // utils::log_info("Shader system demo completed");
        // utils::log_info("=== End Shader System Demo ===");
        
        // memory tracking and leak detection
        // if (renderer.texture_dict()) {
        //     renderer.texture_dict()->log_memory_stats();
        // }
        
        renderer.end_frame();
    }
    return 0;
}
