cmake_minimum_required(VERSION 4.0)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message("Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/out/CMAKE)

project(FRAMEVIEW)

set(SOURCES
    app/main.cpp
    backend/d3d11/d3d11_draw_manager.cpp
    backend/d3d11/d3d11_renderer.cpp
    backend/d3d11/d3d11_texture.cpp
    core/draw_buffer.cpp
    resources/font.cpp
    resources/shader.cpp
    utils/error.cpp
    utils/logger.cpp
)

add_executable(FRAMEVIEW ${SOURCES})

target_include_directories(FRAMEVIEW PRIVATE
    ${PROJECT_SOURCE_DIR}
    E:\\freetype\\include
)

target_compile_options(FRAMEVIEW PRIVATE
    /Zi
    /EHsc
    /std:c++17
    /MT
)

# enable/disable debug logging at compile time by configuration
target_compile_definitions(FRAMEVIEW PRIVATE
    $<$<CONFIG:Debug>:UTILS_DEBUG_LOGGING=1>
    $<$<CONFIG:RelWithDebInfo>:UTILS_DEBUG_LOGGING=0>
    $<$<CONFIG:Release>:UTILS_DEBUG_LOGGING=0>
    $<$<CONFIG:MinSizeRel>:UTILS_DEBUG_LOGGING=0>
)

target_link_directories(FRAMEVIEW PRIVATE 
    "E:/freetype/lib"
)

target_link_libraries(FRAMEVIEW PRIVATE
    freetype.lib
    d3d11.lib
    d3dcompiler.lib
    dxgi.lib
    user32.lib
    kernel32.lib
    msvcrt.lib
    msvcmrt.lib
)

target_link_options(FRAMEVIEW PRIVATE 
    /NODEFAULTLIB:LIBCMT
)

set_target_properties(FRAMEVIEW PROPERTIES
    PDB_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/out/CMAKE
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/out/CMAKE
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/out/CMAKE
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/out/CMAKE
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/out/debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/out/release
)

# add_custom_command(TARGET FRAMEVIEW POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E move 
#         $<TARGET_FILE:FRAMEVIEW> 
#         "$<TARGET_FILE_DIR:FRAMEVIEW>/../.."
# )