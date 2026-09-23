cmake_minimum_required(VERSION 3.22)
project(BlockBreakAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(BlockBreakAnalyzer SHARED
    src/Api.cpp
)

target_include_directories(BlockBreakAnalyzer PRIVATE
    include
    compat/preloader/include
)

target_compile_options(BlockBreakAnalyzer PRIVATE
    -fPIC
    -fvisibility=hidden
    -fno-omit-frame-pointer
)

target_compile_definitions(BlockBreakAnalyzer PRIVATE
    BBA_MINECRAFT_VERSION="1.26.51.01_RC0"
)

if(ANDROID)
    target_compile_options(BlockBreakAnalyzer PRIVATE
        -Oz
        -ffunction-sections
        -fdata-sections
        -flto
        -fexceptions
        -fvisibility-inlines-hidden
        -w
    )
    target_link_options(BlockBreakAnalyzer PRIVATE
        -Wl,--gc-sections
        -flto
        -Wl,-z,max-page-size=16384
        -Wl,--hash-style=gnu
        -Wl,--allow-shlib-undefined
    )
    target_link_libraries(BlockBreakAnalyzer PRIVATE log android EGL)
else()
    target_link_options(BlockBreakAnalyzer PRIVATE -Wl,--allow-shlib-undefined)
endif()

set_target_properties(BlockBreakAnalyzer PROPERTIES
    OUTPUT_NAME "BlockBreakAnalyzer"
)
