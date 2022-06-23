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

    // auto consola = std::make_shared<resources::font>("C:\\Windows\\Fonts\\consola.ttf", 32.f);
    // auto trebuchetMS = std::make_shared<resources::font>("resources\\fonts\\trebuchetMS.ttf", 32.f);
    // auto notoSans = std::make_shared<resources::font>("resources\\fonts\\NotoSans-VariableFont_wdth,wght.ttf", 32.f);
    auto notoSansSC = std::make_shared<resources::font>("resources\\fonts\\NotoSansSC-VariableFont_wght.ttf", 32.f);

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
    //consola->load(renderer.device());
    //trebuchetMS->load(renderer.device());
    //notoSans->load(renderer.device());
    notoSansSC->load(renderer.device());
    // trebuchetMS->add_fallback(notoSans);
    // notoSans->add_fallback(notoSansSC);
    // notoSansSC->add_fallback(consola);
    


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



    // create draw manager and buffers
    auto* draw_mgr = renderer.draw_manager();
    size_t buffer_id = draw_mgr->register_buffer(0);
    auto* buf = draw_mgr->get_buffer(buffer_id);
    // create a buffer for textured geometry
    size_t textured_buffer_id = draw_mgr->register_buffer(2);
    auto* textured_buf = draw_mgr->get_buffer(textured_buffer_id);
    // create a second buffer for text
    size_t text_buffer_id = draw_mgr->register_buffer(1);
    auto* text_buf = draw_mgr->get_buffer(text_buffer_id);

    MSG msg = {};
    bool running = true;
    while (running) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) running = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // clear buffers for new frame
        buf->vertices.clear();
        buf->indices.clear();
        textured_buf->vertices.clear();
        textured_buf->indices.clear();
        // clear texture stack for textured buffer
        textured_buf->clear_texture_stack();
        text_buf->vertices.clear();
        text_buf->indices.clear();

        // colored quad (non-textured)
        buf->prim_rect_multi_color({100, 100}, {300, 300},
            core::color{1.0f, 0.0f, 0.0f, 1.0f},
            core::color{0.0f, 1.0f, 0.0f, 1.0f},
            core::color{0.0f, 0.0f, 1.0f, 1.0f},
            core::color{1.0f, 1.0f, 0.0f, 1.0f});

        // textured quad using texture stack
        textured_buf->push_texture(tex);
        // calculate quad size to maintain texture aspect ratio
        float quad_width = 200.0f;  // Base width
        float quad_height = quad_width * (float)texture_height / (float)texture_width;  // Maintain aspect ratio
        textured_buf->prim_rect_uv({350, 100}, {350 + quad_width, 100 + quad_height}, {0, 0}, {1, 1}, core::pack_color_abgr({1, 1, 1, 1}));
 
        // triangle
        buf->triangle_filled({600, 300}, {700, 100}, {800, 300},
            core::pack_color_abgr({0, 1, 0, 1}),
            core::pack_color_abgr({0, 0, 1, 1}),
            core::pack_color_abgr({1, 1, 0, 1}));
            
        // n-gon
        buf->n_gon({950, 200}, 100, 7, core::pack_color_abgr({0.35f, 0.65f, 0, 1}));

        // line
        buf->line({100, 350}, {300, 400}, core::pack_color_abgr({1, 1, 1, 1}), core::pack_color_abgr({1, 0, 1, 1}), 4.0f);

        // polyline
        std::vector<core::position> poly_points = {{350, 350}, {400, 400}, {450, 350}, {500, 400}, {550, 350}};
        buf->poly_line(poly_points, core::pack_color_abgr({0, 1, 1, 1}), 3.0f, false);

        // filled circle
        buf->circle_filled({700, 400}, 50, core::pack_color_abgr({1, 0, 0, 1}), core::pack_color_abgr({1, 1, 0, 1}), 48);

        // text
        text_buf->push_font(notoSansSC);
        
        text_buf->text("Hello, FRAMEVIEW!", {100, 500}, core::pack_color_abgr({1, 1, 1, 1}));
        text_buf->text("Unicode test | 你好世界 | にちは", {100, 550}, core::pack_color_abgr({1, 1, 0, 1}));
        text_buf->pop_font();
        
        // update atlas texture after text rendering to include any new glyphs
        if (notoSansSC) {
            notoSansSC->update_atlas_texture(renderer.device());
        }

        renderer.begin_frame();
        renderer.clear(core::color{0.1f, 0.1f, 0.2f, 1.0f});
        
        // draw non-textured geometry buffer (color-only shader)
        if (buf && !buf->vertices.empty() && !buf->indices.empty()) {
            renderer.set_pixel_shader("color_only");
            renderer.draw_buffer(buf);
        } else {
            utils::log_warn("Geometry buffer empty: buf=%p, vertices=%zu, indices=%zu", 
                   buf, buf ? buf->vertices.size() : 0, buf ? buf->indices.size() : 0);
        }
        
        // draw textured geometry buffer with texture (generic shader)
        if (textured_buf && !textured_buf->vertices.empty() && 
        !textured_buf->indices.empty()) {
            renderer.set_pixel_shader("generic");
            renderer.draw_buffer(textured_buf);
        }

        // // draw text buffer with font atlas
        if (notoSansSC && notoSansSC->get_atlas_srv() && text_buf && !text_buf->vertices.empty() && !text_buf->indices.empty()) {

            renderer.set_pixel_shader("generic");
            // bind the font atlas SRV directly instead of using texture handle
            renderer.set_font_atlas(notoSansSC->get_atlas_srv());
            renderer.draw_buffer(text_buf);
        } else {
            utils::log_warn("Skipping text draw: notoSansSC=%p, atlas_srv=%p, text_buf=%p, vertices=%zu, indices=%zu",
                   notoSansSC.get(), notoSansSC ? notoSansSC->get_atlas_srv() : nullptr, text_buf,
                   text_buf ? text_buf->vertices.size() : 0, text_buf ? text_buf->indices.size() : 0);
        }
        
        renderer.end_frame();
    }
    return 0;
}
