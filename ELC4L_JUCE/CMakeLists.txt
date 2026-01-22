# ==============================================================================
# ELC4L - Elite 4-Band Compressor + Limiter
# CMake Build Configuration
# Supports: VST2 (32/64bit), VST3, AU (macOS), Standalone
# ==============================================================================

cmake_minimum_required(VERSION 3.15)

# 프로젝트 설정
project(ELC4L VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE 경로 설정
if(NOT DEFINED JUCE_PATH)
    if(EXISTS "C:/JUCE")
        set(JUCE_PATH "C:/JUCE")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE")
        set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE")
    elseif(EXISTS "$ENV{HOME}/JUCE")
        set(JUCE_PATH "$ENV{HOME}/JUCE")
    else()
        message(FATAL_ERROR "JUCE not found. Set JUCE_PATH or install JUCE to a standard location.")
    endif()
endif()

message(STATUS "Using JUCE from: ${JUCE_PATH}")
add_subdirectory(${JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE)

# VST2 SDK 경로 (프로젝트 내 vstsdk 폴더 사용)
set(VST2_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../vstsdk")
if(EXISTS "${VST2_SDK_PATH}/aeffectx.h")
    message(STATUS "VST2 SDK found at: ${VST2_SDK_PATH}")
    juce_set_vst2_sdk_path(${VST2_SDK_PATH})
    set(VST2_ENABLED ON)
else()
    message(STATUS "VST2 SDK not found. VST2 build disabled.")
    set(VST2_ENABLED OFF)
endif()

# ==============================================================================
# 플러그인 타깃 설정
# ==============================================================================

# 플러그인 이름 설정 (빌드 타입에 따라)
if(BUILD_VST2_ONLY)
    set(PLUGIN_NAME "ELC4L_VST2")
    set(PLUGIN_FORMATS VST)
elseif(BUILD_VST3_ONLY)
    set(PLUGIN_NAME "ELC4L_VST3")
    set(PLUGIN_FORMATS VST3)
else()
    set(PLUGIN_NAME "ELC4L")
    set(PLUGIN_FORMATS VST3 Standalone)
    if(VST2_ENABLED)
        list(APPEND PLUGIN_FORMATS VST)
    endif()
    if(APPLE)
        list(APPEND PLUGIN_FORMATS AU)
    endif()
endif()

message(STATUS "Building plugin: ${PLUGIN_NAME}")
message(STATUS "Formats: ${PLUGIN_FORMATS}")

juce_add_plugin(${PLUGIN_NAME}
    COMPANY_NAME "Hyeok Audio"
    COMPANY_COPYRIGHT "2024 Hyeok Audio"
    COMPANY_WEBSITE "https://hyeokaudio.com"
    COMPANY_EMAIL "contact@hyeokaudio.com"
    
    PLUGIN_MANUFACTURER_CODE HyAu
    PLUGIN_CODE ELC4
    
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    
    COPY_PLUGIN_AFTER_BUILD FALSE
    
    FORMATS ${PLUGIN_FORMATS}
    
    PRODUCT_NAME "${PLUGIN_NAME}"
    
    # AU 전용 설정
    AU_MAIN_TYPE "kAudioUnitType_Effect"
    
    # VST3 전용 설정
    VST3_CATEGORIES "Fx" "Dynamics"
    
    # 번들 ID
    BUNDLE_ID "com.hyeokaudio.elc4l"
)

# ==============================================================================
# 소스 파일
# ==============================================================================
target_sources(${PLUGIN_NAME} PRIVATE
    # 메인 프로세서/에디터
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    
    # DSP 모듈
    Source/DSP/DSPModules.h
    
    # UI 컴포넌트
    Source/UI/CustomLookAndFeel.cpp
    Source/UI/CustomLookAndFeel.h
    Source/UI/SpectrumAnalyzer.cpp
    Source/UI/SpectrumAnalyzer.h
    Source/UI/Meters.cpp
    Source/UI/Meters.h
    Source/UI/BandSection.cpp
    Source/UI/BandSection.h
    Source/UI/LimiterSection.cpp
    Source/UI/LimiterSection.h
)

# ==============================================================================
# 컴파일 정의
# ==============================================================================
target_compile_definitions(${PLUGIN_NAME}
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

# JuceHeader.h 생성 활성화
juce_generate_juce_header(${PLUGIN_NAME})

# ==============================================================================
# 링크 라이브러리
# ==============================================================================
target_link_libraries(${PLUGIN_NAME}
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ==============================================================================
# 플랫폼별 설정
# ==============================================================================
if(WIN32)
    # Windows 전용 설정
    target_compile_definitions(${PLUGIN_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    
    # UTF-8 인코딩
    target_compile_options(${PLUGIN_NAME} PRIVATE /utf-8)
    
elseif(APPLE)
    # macOS 전용 설정
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
        XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
    )
    
    # Universal Binary (Intel + Apple Silicon)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architectures")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
endif()

# ==============================================================================
# 출력 경로 설정
# ==============================================================================
set_target_properties(${PLUGIN_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# ==============================================================================
# 빌드 정보 출력
# ==============================================================================
message(STATUS "")
message(STATUS "=== ELC4L Build Configuration ===")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Formats: ${PLUGIN_FORMATS}")
message(STATUS "  JUCE Path: ${JUCE_PATH}")
if(VST2_ENABLED)
    message(STATUS "  VST2 SDK: ${VST2_SDK_PATH}")
endif()
message(STATUS "=================================")
message(STATUS "")
