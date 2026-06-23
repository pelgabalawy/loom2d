cmake_minimum_required(VERSION 3.21)
project(loom2d VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(LOOM2D_BUILD_TESTS "Build the loom2d test suite" ON)

# ── Python ───────────────────────────────────────────────────────────────────

find_package(Python 3.9 COMPONENTS Interpreter Development.Module REQUIRED)

# ── FetchContent dependencies ─────────────────────────────────────────────────

include(FetchContent)

# pybind11
FetchContent_Declare(pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v2.13.6
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(pybind11)

# SDL3
FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.14
    GIT_SHALLOW    TRUE)
set(SDL_SHARED       ON  CACHE BOOL "" FORCE)
set(SDL_STATIC       OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

# Box2D v3
FetchContent_Declare(box2d
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    GIT_TAG        v3.0.0
    GIT_SHALLOW    TRUE)
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_TESTBED    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(box2d)

# stb (header-only image loading / writing)
FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE)
FetchContent_Populate(stb)

# sokol (header-only GPU abstraction — sokol_gfx.h, sokol_log.h)
# Pinned to a known commit: sokol has no releases and its master API changes
# (the "views" binding model below is coded against exactly this header).
FetchContent_Declare(sokol
    GIT_REPOSITORY https://github.com/floooh/sokol.git
    GIT_TAG        ae0bc31daad8a60457cad4b5dae9223f237b2e34)
FetchContent_Populate(sokol)

# miniaudio (header-only audio)
FetchContent_Declare(miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG        0.11.21
    GIT_SHALLOW    TRUE)
FetchContent_Populate(miniaudio)

# Google Test (only for test build)
if(LOOM2D_BUILD_TESTS)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
        GIT_SHALLOW    TRUE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    enable_testing()
endif()

# ── Engine static library (no Python dep) ────────────────────────────────────

set(ENGINE_SOURCES
    src/platform/window.cpp
    src/graphics/renderer.cpp
    src/graphics/texture.cpp
    src/graphics/camera.cpp
    src/graphics/sprite_batcher.cpp
    src/graphics/sokol_impl.cpp
    src/scene/node.cpp
    src/scene/sprite_node.cpp
    src/scene/scene.cpp
    src/scene/animation.cpp
    src/scene/tilemap.cpp
    src/input/input.cpp
    src/physics/physics.cpp
    src/audio/audio.cpp
    src/assets/asset_manager.cpp
)

add_library(loom2d_core STATIC ${ENGINE_SOURCES})

target_include_directories(loom2d_core PUBLIC
    src/
    ${stb_SOURCE_DIR}
    ${sokol_SOURCE_DIR}
    ${miniaudio_SOURCE_DIR}
)

target_link_libraries(loom2d_core PUBLIC
    SDL3::SDL3
    box2d
)

# OpenGL backend for sokol_gfx (sokol self-loads GL function pointers).
if(WIN32)
    target_link_libraries(loom2d_core PUBLIC winmm ole32 propsys opengl32)
elseif(APPLE)
    target_link_libraries(loom2d_core PUBLIC "-framework OpenGL")
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(loom2d_core PUBLIC OpenGL::GL ${CMAKE_DL_LIBS})
endif()

# ── Python native module ──────────────────────────────────────────────────────

pybind11_add_module(loom2d_native src/bindings/module.cpp)
target_link_libraries(loom2d_native PRIVATE loom2d_core)

# Make the SDL shared library load from the same directory as the native module
# on every platform.
if(WIN32)
    # Windows has no rpath; just copy SDL3.dll next to the .pyd.
    add_custom_command(TARGET loom2d_native POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:loom2d_native>
        COMMENT "Copying SDL3.dll next to module")
else()
    # Linux/macOS: bake a relative rpath so the module finds libSDL3 next to it,
    # then copy the SONAME-versioned shared lib (the name recorded in the module's
    # NEEDED/load entries — e.g. libSDL3.so.0 / libSDL3.0.dylib).
    if(APPLE)
        set(_loom_rpath "@loader_path")
    else()
        set(_loom_rpath "$ORIGIN")
    endif()
    # BUILD_WITH_INSTALL_RPATH links the module with the final rpath up front, so
    # we don't depend on CMake's install-time rpath rewrite (which needs patchelf
    # and can silently no-op inside the manylinux wheel container, leaving the
    # module unable to find the bundled SDL — the cause of the Linux import fail).
    set_target_properties(loom2d_native PROPERTIES
        INSTALL_RPATH               "${_loom_rpath}"
        BUILD_RPATH                 "${_loom_rpath}"
        BUILD_WITH_INSTALL_RPATH    ON
        INSTALL_RPATH_USE_LINK_PATH OFF)
    add_custom_command(TARGET loom2d_native POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_SONAME_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:loom2d_native>
        COMMENT "Copying SDL3 shared library next to module")
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────

if(LOOM2D_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# ── Install (used by scikit-build-core to assemble the wheel) ─────────────────
# Everything lands in the `loom2d/` package dir so the wheel is self-contained:
# the native module + the SDL shared library it loads (via the $ORIGIN /
# @loader_path rpath baked above) sit side by side.

install(TARGETS loom2d_native
    LIBRARY DESTINATION loom2d
    RUNTIME DESTINATION loom2d)

if(WIN32)
    install(FILES $<TARGET_FILE:SDL3::SDL3> DESTINATION loom2d)
else()
    # The SONAME (e.g. libSDL3.so.0) is a *symlink* to the versioned real file on
    # Linux; install(FILES) would copy the symlink, which scikit-build-core then
    # drops when packing the wheel — leaving the module unable to find its SDL.
    # Copy the dereferenced real library content to a file named after the SONAME
    # so the wheel always contains a genuine shared object.
    install(CODE "file(MAKE_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/loom2d\")
        file(COPY_FILE
            \"$<TARGET_FILE:SDL3::SDL3>\"
            \"\${CMAKE_INSTALL_PREFIX}/loom2d/$<TARGET_SONAME_FILE_NAME:SDL3::SDL3>\"
            ONLY_IF_DIFFERENT)")
endif()
