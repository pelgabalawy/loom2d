include(GoogleTest)

# ── Headless SDL helper ───────────────────────────────────────────────────────
# Sets SDL_VIDEODRIVER=offscreen so tests can run without a display

# ── C++ unit tests ────────────────────────────────────────────────────────────

set(CPP_TEST_SOURCES
    cpp/test_math.cpp
    cpp/test_graphics.cpp
    cpp/test_scene.cpp
    cpp/test_animation.cpp
    cpp/test_input.cpp
    cpp/test_physics.cpp
    cpp/test_audio.cpp
    cpp/test_asset_cache.cpp
)

add_executable(loom2d_tests ${CPP_TEST_SOURCES})
target_link_libraries(loom2d_tests PRIVATE loom2d_core GTest::gtest_main)
target_include_directories(loom2d_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Make the test binary self-contained: copy the SDL shared library next to it so
# gtest_discover_tests (which runs the exe) and the tests themselves can load it.
if(WIN32)
    add_custom_command(TARGET loom2d_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:loom2d_tests>
        COMMENT "Copying SDL3.dll next to loom2d_tests")
endif()

# Environment for headless SDL
set_tests_properties(PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy")

gtest_discover_tests(loom2d_tests
    PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
    DISCOVERY_MODE PRE_TEST)
