cmake_minimum_required(VERSION 3.15)

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "")

project(ELC4L_VST3
    VERSION 2.0.0.1
    DESCRIPTION "ELC4L 4-Band Compressor + Limiter VST3"
)

set(SMTG_VSTGUI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../vst3sdk/vstgui4")
set(vst3sdk_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../vst3sdk")

# Ensure SMTG/VSTGUI-related options are set before importing the VST3 SDK
# so the SDK picks up these cache variables during configuration.
# Move any SMTG_* or VSTGUI-related `set()`/`option()` here.
set(SMTG_ADD_VSTGUI ON CACHE BOOL "" FORCE)
set(SMTG_CREATE_PLUGIN_LINK 0 CACHE BOOL "Disable creating symlinks to user VST3 folder" FORCE)

# VST3 SDK 경로 검사
if(NOT EXISTS "${vst3sdk_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "VST3 SDK not found at ${vst3sdk_SOURCE_DIR}. Run: git clone --recursive https://github.com/steinbergmedia/vst3sdk.git")
endif()

# VST3 SDK 추가 (after SMTG/VSTGUI options are configured)
add_subdirectory(${vst3sdk_SOURCE_DIR} ${CMAKE_BINARY_DIR}/vst3sdk)

# Plugin 소스 파일
set(ELC4L_SOURCES
    src/ELC4Lfactory.cpp
    src/ELC4Lprocessor.cpp
    src/ELC4Lprocessor.h
    src/ELC4Lcontroller.cpp
    src/ELC4Lcontroller.h
    src/ELC4Leditor.cpp
    src/ELC4Leditor.h
    src/ELC4Lids.h
    src/ELC4Ldsp.h
    src/version.h
)

# Windows 전용 DLL 진입점
if(WIN32)
    list(APPEND ELC4L_SOURCES
        "${vst3sdk_SOURCE_DIR}/public.sdk/source/main/dllmain.cpp"
    )
endif()

# VST3 플러그인 타깃 생성
smtg_add_vst3plugin(ELC4L ${ELC4L_SOURCES})

target_include_directories(ELC4L PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${SMTG_VSTGUI_ROOT}"
)

target_link_libraries(ELC4L PRIVATE
    sdk
    vstgui_support
)

# 출력 디렉터리 설정
smtg_target_set_bundle(ELC4L
    BUNDLE_IDENTIFIER "com.hyeokaudio.elc4l"
    COMPANY_NAME "Hyeok Audio"
)

# macOS: 코드 서명 비활성화 (개발용)
if(APPLE)
    smtg_target_codesign(ELC4L OFF)
endif()
