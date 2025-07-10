cmake_minimum_required(VERSION 3.29)

# project
project(lsfg-vk-v3.1
    DESCRIPTION "Lossless Scaling Frame Generation v3.1"
    LANGUAGES CXX)

option(LDFGVK_EXCESS_DEBUG "Enable excessive debug output" OFF)

file(GLOB SOURCES
    "src/core/*.cpp"
    "src/pool/*.cpp"
    "src/shaders/*.cpp"
    "src/utils/*.cpp"
    "src/*.cpp"
)

add_library(lsfg-vk-v3.1 STATIC ${SOURCES})

# target
set_target_properties(lsfg-vk-v3.1 PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON)
target_include_directories(lsfg-vk-v3.1
    PRIVATE include
    PUBLIC public)
target_link_libraries(lsfg-vk-v3.1
    PRIVATE vulkan)
target_compile_options(lsfg-vk-v3.1 PRIVATE
    -fPIC)

# diagnostics
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(lsfg-vk-v3.1 PROPERTIES
        EXPORT_COMPILE_COMMANDS ON)
endif()

if(LSFGVK_EXCESS_DEBUG)
    target_compile_options(lsfg-vk-v3.1 PRIVATE
        -Weverything
        # disable compat c++ flags
        -Wno-pre-c++20-compat-pedantic
        -Wno-pre-c++17-compat
        -Wno-c++98-compat-pedantic
        -Wno-c++98-compat
        # disable other flags
        -Wno-missing-designated-field-initializers
        -Wno-shadow         # allow shadowing
        -Wno-switch-enum    # ignore missing cases
        -Wno-switch-default # ignore missing default
        -Wno-padded         # ignore automatic padding
        -Wno-exit-time-destructors # allow globals
        -Wno-global-constructors   # allow globals
        -Wno-cast-function-type-strict # for vulkan
    )

    set_target_properties(lsfg-vk-v3.1 PROPERTIES
        CXX_CLANG_TIDY clang-tidy)
endif()
