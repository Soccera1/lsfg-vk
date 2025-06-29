cmake_minimum_required(VERSION 3.29)

project(lsfg-vk-base-base
    VERSION 0.0.1
    DESCRIPTION "lsfg-vk-base: LSFG on Linux through Vulkan"
    LANGUAGES CXX)

# cmake options

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_CLANG_TIDY clang-tidy)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# main project

file(GLOB SOURCES
    "src/core/*.cpp"
    "src/shaderchains/*.cpp"
    "src/utils/*.cpp"
    "src/*.cpp"
)

add_executable(lsfg-vk-base ${SOURCES})

target_include_directories(lsfg-vk-base
    PUBLIC include)
target_link_libraries(lsfg-vk-base
    PUBLIC vulkan)
target_compile_options(lsfg-vk-base PRIVATE
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
    -Wno-global-constructors
    # required for vulkan
    -Wno-cast-function-type-strict
)
