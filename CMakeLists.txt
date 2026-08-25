cmake_minimum_required(VERSION 3.20)
project(PixelForge VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Build options
option(PIXELFORGE_BUILD_TESTS "Build unit tests" ON)
option(PIXELFORGE_USE_SYSTEM_OPENCV "Use system OpenCV" OFF)

# Find dependencies
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets Core Gui Concurrent)
find_package(OpenCV REQUIRED COMPONENTS core imgproc photo highgui imgcodecs objdetect)
find_package(Threads REQUIRED)

# OpenMP for parallel processing
find_package(OpenMP)

# Source directories
set(CORE_SOURCES_DIR ${CMAKE_SOURCE_DIR}/src/core)
set(APP_SOURCES_DIR ${CMAKE_SOURCE_DIR}/src/app)
set(GUI_SOURCES_DIR ${CMAKE_SOURCE_DIR}/src/gui)
set(UTILS_SOURCES_DIR ${CMAKE_SOURCE_DIR}/src/utils)

# Collect core engine sources
file(GLOB_RECURSE CORE_SOURCES 
    ${CORE_SOURCES_DIR}/*.cpp
    ${CORE_SOURCES_DIR}/*.h
)

# Collect application layer sources
file(GLOB_RECURSE APP_SOURCES 
    ${APP_SOURCES_DIR}/*.cpp
    ${APP_SOURCES_DIR}/*.h
)

# Collect GUI sources
file(GLOB_RECURSE GUI_SOURCES 
    ${GUI_SOURCES_DIR}/*.cpp
    ${GUI_SOURCES_DIR}/*.h
    ${GUI_SOURCES_DIR}/*.ui
)

# Collect utility sources
file(GLOB_RECURSE UTILS_SOURCES 
    ${UTILS_SOURCES_DIR}/*.cpp
    ${UTILS_SOURCES_DIR}/*.h
)

# Resources
set(RESOURCE_FILES
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)

# Main executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    ${CORE_SOURCES}
    ${APP_SOURCES}
    ${GUI_SOURCES}
    ${UTILS_SOURCES}
    ${RESOURCE_FILES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::Core
    Qt6::Gui
    Qt6::Concurrent
    ${OpenCV_LIBS}
    Threads::Threads
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PIXELFORGE_HAS_OPENMP)
endif()

# Platform-specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
elseif(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "PixelForge"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    )
endif()

# Tests
if(PIXELFORGE_BUILD_TESTS)
    enable_testing()
    find_package(GTest QUIET)
    if(GTest_FOUND)
        add_subdirectory(tests)
    else()
        message(STATUS "GoogleTest not found, skipping tests")
    endif()
endif()

# Install
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

# CPack configuration
set(CPACK_PACKAGE_NAME "PixelForge")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION "Desktop Image Style & Filter Editor")
set(CPACK_PACKAGE_VENDOR "PixelForge Team")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
else()
    set(CPACK_GENERATOR "DEB;TGZ")
endif()

include(CPack)