cmake_minimum_required(VERSION 3.25)

project(MinecraftDX11 VERSION 0.1.0 LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "MinecraftDX11 targets Windows only (Direct3D 11).")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Third-party (vendored, see third_party/README.md) ---

add_library(miniz STATIC third_party/miniz/miniz.c)
target_include_directories(miniz PUBLIC third_party)
target_compile_definitions(miniz PUBLIC MINIZ_NO_ZLIB_COMPATIBLE_NAMES _CRT_SECURE_NO_WARNINGS)
target_compile_options(miniz PRIVATE /W0)

add_library(third_party_headers INTERFACE)
target_include_directories(third_party_headers SYSTEM INTERFACE third_party)

# --- Game code, shared by the game and the tests ---

file(GLOB_RECURSE MC_ENGINE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.h"
)
list(REMOVE_ITEM MC_ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/src/client/main/Main.cpp")

add_library(mc_engine STATIC ${MC_ENGINE_SOURCES})
target_include_directories(mc_engine PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_compile_definitions(mc_engine PUBLIC
    UNICODE _UNICODE
    WIN32_LEAN_AND_MEAN NOMINMAX
    $<$<CONFIG:Debug>:MC_DEBUG>
)
# /fp:precise: IEEE results without fused multiply-add (MSVC only contracts with an explicit /fp:contract),
# so float math matches the JVM bit for bit.
target_compile_options(mc_engine PUBLIC /W4 /permissive- /utf-8 /fp:precise)
target_link_libraries(mc_engine
    PUBLIC d3d11 dxgi dxguid
    PRIVATE miniz third_party_headers dbghelp bcrypt
)

# --- Game ---

add_executable(MinecraftDX11 WIN32 src/client/main/Main.cpp)
target_link_libraries(MinecraftDX11 PRIVATE mc_engine)

# --- Tests ---

enable_testing()
file(GLOB MC_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tests/*.cpp")
add_executable(MinecraftDX11Tests ${MC_TEST_SOURCES})
target_include_directories(MinecraftDX11Tests PRIVATE "${CMAKE_SOURCE_DIR}/tests")
target_link_libraries(MinecraftDX11Tests PRIVATE mc_engine third_party_headers)
add_test(NAME MinecraftDX11Tests COMMAND MinecraftDX11Tests)
