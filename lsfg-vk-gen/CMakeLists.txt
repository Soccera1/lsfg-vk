project(lsfg-vk-gen
    VERSION 0.0.1
    DESCRIPTION "lsfg-vk-gen: LSFG on Linux through Vulkan (backend)"
    LANGUAGES CXX)

file(GLOB BACKEND_SOURCES
    "src/core/*.cpp"
    "src/shaderchains/*.cpp"
    "src/*.cpp"
)

add_library(lsfg-vk-gen SHARED ${BACKEND_SOURCES})

target_include_directories(lsfg-vk-gen
    PRIVATE include
    PUBLIC public)
target_link_libraries(lsfg-vk-gen
    PUBLIC vulkan)
target_compile_options(lsfg-vk-gen PRIVATE
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
