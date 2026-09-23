cmake_minimum_required(VERSION 3.28)
project(MyGameEngine 
    VERSION 0.1.0 
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Global Options
option(BUILD_EDITOR "Build Engine Editor with Dear ImGui" ON)
option(BUILD_TESTS "Build Engine Unit Tests" ON)
option(ENABLE_TRACY "Enable Tracy Profiler Instrumentation" OFF)
option(ENABLE_LTO "Enable Link-Time Optimization (LTO/LTCG)" OFF)

if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(ipo_supported)
        message(STATUS "Link-Time Optimization (LTO/IPO) enabled.")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "Link-Time Optimization not supported: ${ipo_error}")
    endif()
endif()

# Include custom cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Engine Runtime Modules (Strict DAG order)
add_subdirectory(engine/core)
add_subdirectory(engine/platform)
add_subdirectory(engine/rhi)
add_subdirectory(engine/world)
add_subdirectory(engine/assets)
add_subdirectory(engine/renderer)
add_subdirectory(engine/physics)
add_subdirectory(engine/animation)
add_subdirectory(engine/audio)
add_subdirectory(engine/network)

# Default startup project for Visual Studio (F5 debugging)
set_property(DIRECTORY  PROPERTY VS_STARTUP_PROJECT GameRuntime)

# Standalone Game Runtime
add_subdirectory(runtime)

# Optional Editor Subproject
if(BUILD_EDITOR)
    add_subdirectory(editor)
endif()

# Optional Unit Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
