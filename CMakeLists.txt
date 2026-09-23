cmake_minimum_required(VERSION 3.22)

project(BlockBreakAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# This is an Android native mod.
if(NOT ANDROID)
    message(FATAL_ERROR
        "BlockBreakAnalyzer is an Android native mod and must be configured with the Android NDK."
    )
endif()

# Currently only ARM64 Bedrock is supported.
if(NOT CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    message(FATAL_ERROR
        "BlockBreakAnalyzer currently supports arm64-v8a only. Got: ${CMAKE_ANDROID_ARCH_ABI}"
    )
endif()

include(FetchContent)

# ============================================================
# Preloader Android SDK
# ============================================================
#
# Use the public SDK instead of copying BedrockTools files into
# this project.
#
FetchContent_Declare(
    preloader_android
    GIT_REPOSITORY https://github.com/LiteLDev/preloader-android.git
    GIT_TAG 0.2.2
    SOURCE_SUBDIR cmake/preloader-sdk
)

FetchContent_MakeAvailable(preloader_android)

# Some SDK layouts expose the CMake target through their source
# directory instead of FetchContent_MakeAvailable().
if(NOT TARGET preloader)
    add_subdirectory(
        "${preloader_android_SOURCE_DIR}"
        "${CMAKE_BINARY_DIR}/_preloader_android"
    )
endif()

# Do not allow the build to continue with unresolved SDK symbols.
if(NOT TARGET preloader)
    message(FATAL_ERROR
        "The preloader SDK target was not created. "
        "Refusing to link with unresolved SDK symbols."
    )
endif()

# ============================================================
# BlockBreakAnalyzer native library
# ============================================================

add_library(BlockBreakAnalyzer SHARED
    src/Api.cpp
)

set_target_properties(BlockBreakAnalyzer PROPERTIES
    OUTPUT_NAME "BlockBreakAnalyzer"
)

target_include_directories(BlockBreakAnalyzer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${preloader_android_SOURCE_DIR}/include"
)

target_compile_definitions(BlockBreakAnalyzer PRIVATE
    BBA_MINECRAFT_VERSION="1.26.51.01_RC0"
)

target_compile_options(BlockBreakAnalyzer PRIVATE
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    -fno-omit-frame-pointer
)

# Fail the build if any required native symbol is unresolved.
target_link_options(BlockBreakAnalyzer PRIVATE
    "-Wl,-z,max-page-size=16384"
    "-Wl,--no-undefined"
)

find_library(log-lib log REQUIRED)

target_link_libraries(BlockBreakAnalyzer PRIVATE
    preloader
    ${log-lib}
)

# ============================================================
# LeviPack packaging
# ============================================================

set(LEVI_PACKAGE_ROOT
    "${CMAKE_CURRENT_BINARY_DIR}/package"
)

set(LEVI_PACKAGE_MOD_DIR
    "${LEVI_PACKAGE_ROOT}/blockbreakanalyzer"
)

set(LEVI_PACKAGE_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/BlockBreakAnalyzer-arm64-v8a.levipack"
)

# Generate manifest.json from manifest.json.in
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/manifest.json.in"
    "${CMAKE_CURRENT_BINARY_DIR}/manifest.json"
    @ONLY
)

# Package layout:
#
# blockbreakanalyzer/
# ├── manifest.json
# ├── libBlockBreakAnalyzer.so
# ├── icon.png
# └── resources/
#     └── minecraft.ttf
#
add_custom_target(levi_package
    COMMAND "${CMAKE_COMMAND}" -E rm -rf
        "${LEVI_PACKAGE_ROOT}"

    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${LEVI_PACKAGE_MOD_DIR}/resources"

    COMMAND "${CMAKE_COMMAND}" -E copy
        "$<TARGET_FILE:BlockBreakAnalyzer>"
        "${LEVI_PACKAGE_MOD_DIR}/libBlockBreakAnalyzer.so"

    COMMAND "${CMAKE_COMMAND}" -E copy
        "${CMAKE_CURRENT_BINARY_DIR}/manifest.json"
        "${LEVI_PACKAGE_MOD_DIR}/manifest.json"

    COMMAND "${CMAKE_COMMAND}" -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.png"
        "${LEVI_PACKAGE_MOD_DIR}/icon.png"

    COMMAND "${CMAKE_COMMAND}" -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/minecraft.ttf"
        "${LEVI_PACKAGE_MOD_DIR}/resources/minecraft.ttf"

    COMMAND "${CMAKE_COMMAND}" -E tar
        "cf"
        "${LEVI_PACKAGE_FILE}"
        --format=zip
        "blockbreakanalyzer"

    WORKING_DIRECTORY
        "${LEVI_PACKAGE_ROOT}"

    DEPENDS
        BlockBreakAnalyzer

    COMMENT
        "Packaging BlockBreakAnalyzer LeviPack"

    VERBATIM
)
