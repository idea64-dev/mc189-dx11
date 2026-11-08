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

add_library(stb_vorbis STATIC third_party/stb/stb_vorbis.c)
target_compile_definitions(stb_vorbis PRIVATE STB_VORBIS_NO_STDIO _CRT_SECURE_NO_WARNINGS)
target_compile_options(stb_vorbis PRIVATE /W0)

add_library(third_party_headers INTERFACE)
target_include_directories(third_party_headers SYSTEM INTERFACE third_party)

# --- Game code, shared by the game and the tests ---

file(GLOB_RECURSE MC_ENGINE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.h"
)
list(REMOVE_ITEM MC_ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/src/client/main/Main.cpp")

# --- Shaders: compiled by fxc into headers with the bytecode ---

find_program(FXC_EXECUTABLE fxc REQUIRED HINTS "$ENV{WindowsSdkVerBinPath}x64" "$ENV{WindowsSdkBinPath}x64")
set(MC_SHADER_SOURCE "${CMAKE_SOURCE_DIR}/shaders/FixedFunction.hlsl")
set(MC_SHADER_OUTPUT "${CMAKE_BINARY_DIR}/generated/shaders")
set(MC_SHADER_HEADERS "")
foreach(shader IN ITEMS
        "FixedFunctionVS|vs_5_0|VSMain"
        "FixedFunctionPS|ps_5_0|PSMain"
        "FixedFunctionPSFlat|ps_5_0|PSMainFlat"
        "PresentVS|vs_5_0|PresentVS"
        "PresentPS|ps_5_0|PresentPS")
    string(REPLACE "|" ";" parts "${shader}")
    list(GET parts 0 name)
    list(GET parts 1 profile)
    list(GET parts 2 entry)
    add_custom_command(
        OUTPUT "${MC_SHADER_OUTPUT}/${name}.h"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${MC_SHADER_OUTPUT}"
        COMMAND "${FXC_EXECUTABLE}" /nologo /O3 /T ${profile} /E ${entry} /Vn g_${name} /Fh "${MC_SHADER_OUTPUT}/${name}.h" "${MC_SHADER_SOURCE}"
        DEPENDS "${MC_SHADER_SOURCE}"
        COMMENT "Compiling shader ${name}"
        VERBATIM)
    list(APPEND MC_SHADER_HEADERS "${MC_SHADER_OUTPUT}/${name}.h")
endforeach()

add_library(mc_engine STATIC ${MC_ENGINE_SOURCES} ${MC_SHADER_HEADERS} "${MC_SHADER_SOURCE}")
set_source_files_properties("${MC_SHADER_SOURCE}" PROPERTIES HEADER_FILE_ONLY TRUE)
target_include_directories(mc_engine PUBLIC "${CMAKE_SOURCE_DIR}/src" PRIVATE "${MC_SHADER_OUTPUT}")
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
    PRIVATE miniz stb_vorbis third_party_headers dbghelp bcrypt xaudio2 rpcrt4 icu
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
