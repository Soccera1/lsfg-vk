cmake_minimum_required(VERSION 3.29)

# project
project(lsfg-vk-common
    LANGUAGES CXX)

file(GLOB SOURCES
    "src/core/*.cpp"
    "src/pool/*.cpp"
    "src/common/*.cpp"
    "src/*.cpp"
)

add_library(lsfg-vk-common STATIC ${SOURCES})

# target
set_target_properties(lsfg-vk-common PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON)
target_include_directories(lsfg-vk-common
    PUBLIC include)
target_link_libraries(lsfg-vk-common
    PRIVATE vulkan)
target_compile_options(lsfg-vk-common PRIVATE
    -fPIC)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(lsfg-vk-common PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# diagnostics
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(lsfg-vk-common PROPERTIES
        EXPORT_COMPILE_COMMANDS ON)
endif()

if(LSFGVK_EXCESS_DEBUG)
    target_compile_options(lsfg-vk-common PRIVATE
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

    set_target_properties(lsfg-vk-common PROPERTIES
        CXX_CLANG_TIDY clang-tidy)
endif()
