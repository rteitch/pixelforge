find_package(GTest REQUIRED)

# Test sources
set(TEST_SOURCES
    test_main.cpp
    test_delaunay.cpp
    test_color_quantization.cpp
    test_lut_engine.cpp
    test_noise_generator.cpp
    test_tone_curve.cpp
    test_image.cpp
    test_history_manager.cpp
    test_batch_processor.cpp
)

add_executable(PixelForgeTests ${TEST_SOURCES})

target_include_directories(PixelForgeTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(PixelForgeTests PRIVATE
    GTest::GTest
    GTest::Main
    Qt6::Core
    ${OpenCV_LIBS}
    Threads::Threads
)

# Link against core sources (not GUI - headless tests)
# We need to compile core/utils/app sources into the test target
file(GLOB_RECURSE CORE_TEST_SOURCES
    ${CMAKE_SOURCE_DIR}/src/core/*.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/*.cpp
    ${CMAKE_SOURCE_DIR}/src/app/*.cpp
)

# Exclude GUI sources from tests
list(FILTER CORE_TEST_SOURCES EXCLUDE REGEX ".*gui/.*")

target_sources(PixelForgeTests PRIVATE ${CORE_TEST_SOURCES})

add_test(NAME PixelForgeTests COMMAND PixelForgeTests)