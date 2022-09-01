### FRAMEVIEW: Architecture and Rendering Guide

This document explains how FRAMEVIEW is structured, how rendering works end-to-end, key design decisions, and what’s required to build and extend the project.

### Goals and Philosophy
- **Unified 2D rendering** with a single buffer and per-command state
- **Robust resource management** via RAII and centralized dictionaries
- **Text that “just works”**, including Unicode with a fallback chain
- **Platform-first backend**: D3D11 initially, others pluggable later
- **Practical diagnostics**: debug logging gated by compile- and run-time flags

---

### High-Level Architecture

- `app/main.cpp`: Example app. Creates a window, initializes the renderer, loads fonts/textures, builds a single `core::draw_buffer`, and renders it each frame.
- `core/`:
  - `draw_types.h`: Vertex, color, position types and helpers (e.g., `pack_color_abgr`)
  - `draw_buffer.h/.cpp`: Unified geometry buffer and draw-command list. High-level drawing APIs (rects, text, lines, etc.).
  - `renderer.h`: Abstract renderer interface
  - `draw_manager.*`: Registers and stores `draw_buffer`s (D3D11 version currently used)
- `backend/d3d11/`:
  - `d3d11_renderer.*`: D3D11 device, swapchain, shaders, input layout, blend state. Draws `draw_buffer` by iterating commands.
  - `d3d11_texture.*`: D3D11 textures + a dictionary for creation, updates, and tracking
  - `d3d11_draw_manager.*`: Glue for using `draw_buffer` with D3D11
- `resources/`:
  - `font.*`: FreeType-based font loading, glyph paging, atlas creation, fallback chain
  - `texture.h`: Texture and dictionary interfaces
  - `shader.*`: Shader helpers (simple at the moment)
  - `shaders/`: Compiled `.cso` blobs for D3D11
- `utils/`:
  - `logger.*`: Colorized logger with `info/warn/error/debug`, gated debug logging
  - `error.*`: Helpers for error creation/reporting

---

### Build and Dependencies

- Toolchain: CMake + MSVC (Windows)
- Graphics: D3D11 (headers and runtime)
- Fonts: FreeType (header/lib paths configured in `CMakeLists.txt` to `E:\freetype`)
- Shaders: Precompiled `.cso` blobs under `resources/shaders/...`

CMake config:
- C++17
- Debug logging controlled by per-config compile definitions:
  - `UTILS_DEBUG_LOGGING=1` for Debug
  - `UTILS_DEBUG_LOGGING=0` for Release/RelWithDebInfo/MinSizeRel
- Output under `out/`

Runtime toggling of debug logs:
- Call `utils::set_debug_logging(true|false)` at startup if desired

Required assets:
- Fonts under `resources/fonts/` (e.g., NotoSans, NotoSansSC)
- Textures under `resources/textures/`
- Compiled shader blobs under `resources/shaders/`

---

### Rendering: End-to-End

1. The app creates a single `core::draw_buffer`
2. High-level APIs on `draw_buffer` append vertices/indices and a corresponding `draw_command` to `cmds`
   - Geometry APIs: `prim_rect_filled`, `prim_rect_uv`, `line`, `text`, `n_gon`, etc.
   - Every call updates unified `vertices` and `indices`
   - A `draw_command` records state (type, elem_count, resource handles, hints)
3. `backend::d3d11::d3d11_renderer::draw_buffer`:
   - Creates a transient `ID3D11Buffer` for vertices and indices
   - Sets pipeline state: input layout, topology, sampler, blend, projection constant buffer
   - Iterates `cmds`:
     - Chooses shaders based on `cmd.type` and `shader_hint`
     - Binds resources referenced by the command (font atlas, texture SRV)
     - Draws the exact `elem_count`, advancing index offset
4. `end_frame` presents the swapchain and flushes texture update queue (CPU→GPU uploads)

Why this approach:
- **Single buffer** improves batching and state transitions
- **Per-command resources** avoid global implicit stacks for binding state
- **Draw-time atlas update** ensures Unicode glyphs render correctly

---

### Unified Geometry Buffer and Commands

- `core::vertex`: 3 floats for position (`pos[3]`), packed ABGR color (`col_u32`), 2 floats for UV
- `draw_buffer` collects:
  - `vertices: std::vector<vertex>`
  - `indices: std::vector<uint32_t>`
  - `cmds: std::vector<draw_command>`
- `draw_command` (key fields):
  - `type: geometry_type` (color_only, textured, font_atlas, …)
  - `elem_count: uint32_t` (indices to draw for the command)
  - `shader_hint: std::string` (e.g., "color_only", "generic")
  - `font: std::shared_ptr<resources::font>` (for font commands)
  - `texture: resources::tex` (for textured commands)
  - Additional state: blur strength, key color, clipping rects (extensible)

Why per-command resources:
- D3D binding is explicit and stateless; storing resources on commands makes the renderer stateless and predictable
- Avoids bugs where a resource stack and geometry get out of sync

---

### Text Rendering and Fallbacks

- FreeType used for font loading, metrics, and glyph rasterization
- Fonts create a **RGBA atlas** (initial 1024x1024) and pack glyph bitmaps row-by-row
- `ensure_glyph(codepoint)` loads/renders a glyph on demand and updates the atlas page in memory
- `d3d11_renderer::draw_buffer` calls `font->update_atlas_texture(device)` right before binding the atlas SRV, ensuring the GPU sees latest glyphs
- **Fallbacks**:
  - Base font attempts `ensure_glyph`
  - If missing, try fallback chain in order, then optional default fallback
  - A codepoint→font cache speeds repeated queries
  - Preloading skips `.notdef` glyphs (`FT_Get_Char_Index == 0`)

Why draw-time atlas updates:
- Unicode often arrives dynamically. Updating immediately before binding avoids “missing glyph” frames and user-side synchronization burdens.

---

### Texture System

- Interface: `resources::texture` and `resources::texture_dict`
- D3D11 impl: `backend::d3d11::d3d11_texture` and `d3d11_texture_dict`
- Creation:
  - Dictionary constructs `shared_ptr<d3d11_texture>` and tracks them
  - `create_texture_from_d3d11` wraps an existing texture and optional SRV (used by fonts)
- Updates:
  - CPU updates go into a vector; a dirty flag is set
  - Dict queues textures for update and later calls `copy_texture_data(ctx)` to upload
- Binding:
  - Renderer fetches SRV via `get_srv()` and binds it to the pixel shader

Why a dictionary:
- Centralized lifetime & updates
- Thread-safety via mutexes
- Low friction for wrapping externally created textures (e.g., font atlases)

---

### Shaders and Pipeline

- Input layout matches `core::vertex`
- Pixel shaders:
  - `generic.cso` (textured)
  - `color_only.cso` (color-only geometry)
  - `fallback.cso` available if main PS creation fails (best-effort)
- Vertex shader:
  - `generic.cso` (with fallback variant)
- Constant buffer:
  - Orthographic projection per frame:
    - Maps (0,0)→(-1,-1) and (width,height)→(1,1)
- Sampler:
  - Linear filter, wrap addressing
- Blend:
  - Alpha blending enabled

---

### Rounded Quads

- All rect draw APIs support `rounding` parameter (0.0 to 0.5)
- Geometry generation follows an ImGui-inspired 9-slice:
  - Center rectangle
  - Four corner arcs (trig-based segments)
  - Four edge strips bridging center and corners
- Produces triangles that correctly fill the rounded rectangle area
- UVs supported for textured variants

Why 9-slice:
- Clear separation between center, corners, and edges
- Easy to adjust smoothness via segments
- Robust connection logic

---

### Logging and Diagnostics

- `utils::logger` with functions: `log_info`, `log_warn`, `log_error`, `log_debug`
- Debug gating:
  - Compile-time: `UTILS_DEBUG_LOGGING` set by config (Debug=1, others=0)
  - Runtime: `utils::set_debug_logging(true|false)` to toggle during execution
- Typical debug logs:
  - Text runs and font switching
  - Renderer resource binds (atlas, textures)
  - Geometry generation diagnostics (when needed)

Why both compile and runtime control:
- In Release, debug logging is off by default, keeping output clean
- Developers can temporarily enable debug output without a rebuild

---

### Error Handling, RAII, and Thread-Safety

- All D3D11 COM objects managed by `ComPtr` RAII
- `d3d11_texture_dict` uses mutexes to guard texture lists and update queues (marked `mutable` to allow locking in const methods used for diagnostics)
- Validation:
  - Texture dimension checks on push
  - Graceful warnings for missing glyphs or SRVs

Guideline:
- Keep GPU state binding localized in the renderer
- Don’t bind from resource classes (they only expose SRVs and data)

---

### Extending FRAMEVIEW

- New geometry primitive:
  - Add a helper on `draw_buffer` that appends vertices/indices
  - Call `add_geometry_color_only`, `add_geometry_textured`, or `add_geometry_font` as appropriate
- New rendering feature (e.g., post-processing):
  - Introduce a new `geometry_type` or a post-pass in renderer
  - Record parameters in `draw_command`
  - Implement the bind/draw logic in the renderer switch block
- New backend (OpenGL/Vulkan):
  - Implement `core::renderer` interface
  - Provide backend-specific `texture` and `texture_dict`
  - Mirror D3D11 renderer flow: upload unified buffer, iterate commands, bind state, draw

---

### Performance Considerations

- Unified buffer reduces draw calls and state changes
- Each frame currently creates transient VB/IB (simple and safe). Potential optimizations:
  - Persistent buffers with mapped writes
  - Arena allocators for vertex/index memory
  - Command merging for identical states
- Text performance:
  - Fallback cache drastically reduces repeated font lookups
  - On-demand glyph paging avoids huge atlases upfront

---

### How to Use (Typical Frame)

- Create and clear the unified buffer
- Draw geometry:
  - Color quad: `prim_rect_filled({x0,y0},{x1,y1}, color, rounding)`
  - Textured quad: `push_texture_scope(tex)`, then `prim_rect_uv(...)`
  - Text:
    - `push_font(notoSans)`
    - `text("Hello 你好 にちは", pos, color)`
    - `pop_font()`
- Renderer:
  - `begin_frame()`
  - `draw_buffer(unified_buf)`
  - `end_frame()`

---

### Known Pitfalls and Fixes

- Unicode not rendering:
  - Ensure `update_atlas_texture` runs right before binding atlas SRV. We do this in `d3d11_renderer::draw_buffer` for font commands.
- Vertex struct access:
  - Use `vertex.pos[0/1]` for XY in logs; `vertex` doesn’t have `x/y` fields.
- D3D11 `PSSetShaderResources` signature:
  - Requires pointers to an array of SRVs, not SRV itself.
- Fallbacks not triggering:
  - We skip `.notdef` glyphs and return false from `ensure_glyph` if `FT_Get_Char_Index` is 0. This guarantees real fallback selection.

---

### Roadmap (Selected)
- Rendering
  - Working blur (post-processing): scissor/blur passes and kernel variants
  - Masking/clipping and additional blend modes
- Shader system
  - Hot-reloading and reflection
- Backends
  - OpenGL, Vulkan, D3D9
- Performance
  - Persistent buffers, command merging, profiling tools

---

### Project Requirements Recap

- Windows 10+ with D3D11 runtime
- FreeType installed at `E:\freetype\` (or adjust `CMakeLists.txt`)
- Assets under `resources/` (fonts, textures, shaders)
- Build via CMake; run the produced executable in `out/`

---

### Conventions and Style

- **Modern C++ (C++23)**, RAII everywhere, no raw COM handling
- **Strong naming**:
  - `snake_case` for variables and methods
  - `SCREAMING_SNAKE_CASE` for constants
  - Member prefixes allowed (`_field`)
- **Explicit state in commands** rather than implicit global context
- **Clear separation**:
  - Resource classes don’t own render state; renderers bind explicitly
- **Diagnostic logging**:
  - Debug-level only; minimal output in Release builds

---

- In short: FRAMEVIEW consolidates all drawing into a unified buffer with explicit per-command state, uses FreeType for dynamic glyph paging with a robust fallback chain, manages GPU resources via RAII dictionaries, and draws deterministically through a stateless renderer loop. This keeps the system predictable, extensible, and easy to debug while providing a solid base for future features like blur, clipping, and multiple backends.