# FRAMEVIEW

FRAMEVIEW is a modular C++ rendering framework designed for extensibility, performance, and modern graphics development. It supports multiple graphics backends (Direct3D 11, OpenGL, Vulkan) and provides a clean abstraction for rendering, resource management, and command submission. 

## Features
- **Multi-backend support:** Direct3D 11, OpenGL, Vulkan (modular backends)
- **Command buffer system:** Efficient, extensible draw command recording
- **Resource management:** Textures, shaders, fonts, and more
- **Modern C++:** Smart pointers, RAII, STL, and type-safe abstractions
- **Extensible architecture:** Easy to add new backends or resource types

## Directory Structure
```
app/         # Application entry point
backend/     # Graphics backend implementations (d3d11, opengl, vulkan)
core/        # Core rendering abstractions (renderer, context, command buffer)
math/        # Math utilities (matrices, vectors, color)
resources/   # Resource management (textures, shaders, fonts)
utils/       # Logging, error handling, platform utilities
include/     # Third-party headers (e.g., stb_image)
tests/       # Unit and integration tests
```

## Build Instructions (Windows)
FRAMEVIEW is currently set up for Windows development using MSVC (cl.exe) and Direct3D 11. You may need to adjust include/library paths for your environment.

### Prerequisites
- Visual Studio or Build Tools (MSVC, cl.exe, CMAKE)
- Windows SDK
- [stb_image.h](https://github.com/nothings/stb) (for loading images into memory)
- [FreeType](https://www.freetype.org/) (for font rendering)

### Build the App
1. Clone the repository:
   ```ps
   git clone https://github.com/itsnearean/FRAMEVIEW.git
   cd FRAMEVIEW
   ```
2. Ensure FreeType is installed and linked against.
3. Compile Shaders:
   ```ps
   Get-ChildItem resources/shaders/pixel/*.hlsl | ForEach-Object { fxc /T ps_5_0 /E main /Fo ($_.DirectoryName + '\\' + $_.BaseName + '.cso') $_.FullName }

   Get-ChildItem resources/shaders/vertex/*.hlsl | ForEach-Object { fxc /T vs_5_0 /E main /Fo ($_.DirectoryName + '\\' + $_.BaseName + '.cso') $_.FullName }
   ```
4. using cmake: 
   ```sh 
   .\build.bat
   ```
5. (optional) using clang in powershell:
   ```sh
   cl.exe /Zi /EHsc /std:c++17 /MT /I . /I E:\freetype\include \
      /Fo .\out\obj\ /Fd .\out\FRAMEVIEW.pdb /Fe .\out\FRAMEVIEW.exe \
      .\app\main.cpp .\resources\font.cpp .\resources\shader.cpp \
      .\core\buffer.cpp .\backend\d3d11\d3d11_renderer.cpp \
      .\backend\d3d11\d3d11_resource_manager.cpp .\backend\d3d11\d3d11_texture.cpp \
      .\utils\error.cpp .\utils\logger.cpp \
      /link /LIBPATH:E:\freetype\lib freetype.lib d3d11.lib d3dcompiler.lib \
      dxgi.lib user32.lib kernel32.lib msvcrt.lib msvcmrt.lib \
      /NODEFAULTLIB:LIBCMT
   ```


## Running the App
- Run `out/FRAMEVIEW.exe` or `out/release/FRAMEVIEW.exe` after building. A window will appear demonstrating basic rendering, texture loading, and font rendering.

## Contributing
Contributions are welcome! Please open issues or pull requests for bug fixes, new features, or improvements. Follow modern C++ best practices and keep code modular and well-documented.

## License
[Specify your license here, e.g., MIT, Apache 2.0, etc.]

## Contact
For questions or support, open an issue on GitHub or contact the maintainer.
