#pragma once

#include <unordered_map>
#include <string>

namespace resources
{
    struct shader_handle; // forward declaration for shader handle

    namespace shaders
    {
        // extern std::unordered_map<std::string, shader_handle*> amplification;
        // extern std::unordered_map<std::string, shader_handle*> any_hit;
        // extern std::unordered_map<std::string, shader_handle*> callable;
        // extern std::unordered_map<std::string, shader_handle*> closest_hit;
        extern std::unordered_map<std::string, shader_handle*> compute;
        extern std::unordered_map<std::string, shader_handle*> geometry;
        // extern std::unordered_map<std::string, shader_handle*> hull;
        extern std::unordered_map<std::string, shader_handle*> mesh;
        // extern std::unordered_map<std::string, shader_handle*> meshlet;
        // extern std::unordered_map<std::string, shader_handle*> miss;
        extern std::unordered_map<std::string, shader_handle*> pixel;
        extern std::unordered_map<std::string, shader_handle*> vertex;
        // extern std::unordered_map<std::string, shader_handle*> raygen;
        // extern std::unordered_map<std::string, shader_handle*> task;
        // extern std::unordered_map<std::string, shader_handle*> tessellation_control;
        // extern std::unordered_map<std::string, shader_handle*> tessellation_eval;
    } // namespace shaders
} // namespace resources 