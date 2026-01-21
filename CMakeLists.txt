cmake_minimum_required(VERSION 3.15)
project(ELC4L VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    # Suppress MSVC legacy warnings
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DNOMINMAX)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    
    # Windows version targeting (Windows 7+)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()

# macOS-specific settings
if(APPLE)
    # Support both Intel and Apple Silicon
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for macOS")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS deployment version")
    
    # macOS framework definitions
    add_definitions(-DMAC_COCOA)
endif()

# VST2 SDK headers path
set(VST2_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vstsdk")

# Source files
set(VST_SDK_SOURCES
    ${VST2_SDK_PATH}/audioeffectx.cpp
)

set(PLUGIN_SOURCES
    src/HyeokStreamMaster.cpp
    src/HyeokStreamEditor.cpp
    src/vstplugmain.cpp
)

set(PLUGIN_HEADERS
    src/HyeokStreamMaster.h
    src/HyeokStreamEditor.h
)

# Create shared library (DLL) - Output name ELC4L
add_library(ELC4L SHARED
    ${VST_SDK_SOURCES}
    ${PLUGIN_SOURCES}
    ${PLUGIN_HEADERS}
)

# Include directories
target_include_directories(ELC4L PRIVATE
    ${VST2_SDK_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link system libraries (Win32 API)
if(WIN32)
    target_link_libraries(ELC4L PRIVATE
        user32.lib
        gdi32.lib
        kernel32.lib
        comctl32.lib
    )
    
    # Module definition file for symbol exports
    set_target_properties(ELC4L PROPERTIES
        LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/ELC4L.def\""
    )
    
    # Output settings
    set_target_properties(ELC4L PROPERTIES
        SUFFIX ".dll"
        PREFIX ""
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    
    # Disable debug info in release
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi")
endif()

# macOS-specific settings
if(APPLE)
    # Link Cocoa and CoreFoundation frameworks
    target_link_libraries(ELC4L PRIVATE
        "-framework Cocoa"
        "-framework CoreFoundation"
        "-framework Carbon"
    )
    
    # VST2 Bundle settings
    set_target_properties(ELC4L PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "vst"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/mac/Info.plist.in"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    
    # Compiler flags for optimization
    target_compile_options(ELC4L PRIVATE
        -O3
        -ffast-math
        -fvisibility=hidden
        -fvisibility-inlines-hidden
    )
endif()

# MSVC-specific settings
if(MSVC)
    target_compile_options(ELC4L PRIVATE
        /W3           # Warning level 3
        /WX-          # Don't treat warnings as errors
        /MP           # Multi-processor compilation
        /GR           # Enable RTTI
        /EHsc         # Enable C++ exceptions
    )
    
    # Use static runtime for portability
    set_property(TARGET ELC4L PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()

# Installation
install(TARGETS ELC4L
    RUNTIME DESTINATION "C:/Program Files/VSTPlugins"
    LIBRARY DESTINATION "C:/Program Files/VSTPlugins"
)

message(STATUS "ELC4L VST2 Plugin Configuration:")
message(STATUS "  VST2 SDK Path: ${VST2_SDK_PATH}")
message(STATUS "  Output Directory: ${CMAKE_BINARY_DIR}/bin")
