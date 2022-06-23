#include "shader.h"

namespace resources
{
    namespace shaders
    {
        // storage for vertex shader registry
        std::unordered_map<std::string, shader_handle*> vertex;
        // storage for pixel shader registry
        std::unordered_map<std::string, shader_handle*> pixel;
    } // namespace shaders
} // namespace resources 