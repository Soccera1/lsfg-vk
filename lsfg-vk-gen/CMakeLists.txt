# dxvk subproject

include(ExternalProject)

ExternalProject_Add(dxvk_git
    PREFIX ${CMAKE_BINARY_DIR}/dxvk
    GIT_REPOSITORY "https://github.com/doitsujin/dxvk"
    GIT_TAG "v2.6.2"
    UPDATE_DISCONNECTED true
    USES_TERMINAL_CONFIGURE true
    USES_TERMINAL_BUILD true
    CONFIGURE_COMMAND
        cd ${CMAKE_BINARY_DIR}/dxvk/src/dxvk_git &&
        sed -i s/private://
            src/dxvk/dxvk_device.h &&
        CFLAGS=-w CXXFLAGS=-w CC=clang CXX=clang++ meson setup
            --buildtype "release"
            --prefix "${CMAKE_BINARY_DIR}/dxvk/native/usr"
            --strip
            -Dbuild_id=false
            --force-fallback-for=libdisplay-info
            --wipe
            ${CMAKE_BINARY_DIR}/dxvk/build
    BUILD_COMMAND
        cd ${CMAKE_BINARY_DIR}/dxvk &&
        ninja -C build install && # sorry cursed lol
        echo "clang -shared -o libdxvkinternals.so $(find build/src/{dxbc,wsi,vulkan,dxvk,dxgi,spirv,util}/*.p -type f -name \\*.o)" > archive.sh &&
        bash archive.sh
    INSTALL_COMMAND ""
)

add_library(dxvk INTERFACE)
add_dependencies(dxvk dxvk_git)

target_link_directories(dxvk
    INTERFACE ${CMAKE_BINARY_DIR}/dxvk/native/usr/lib
    INTERFACE ${CMAKE_BINARY_DIR}/dxvk)
target_include_directories(dxvk SYSTEM
    INTERFACE ${CMAKE_BINARY_DIR}/dxvk/native/usr/include/dxvk
    INTERFACE ${CMAKE_BINARY_DIR}/dxvk/src/dxvk_git/src
    INTERFACE ${CMAKE_BINARY_DIR}/dxvk/src/dxvk_git/include/spirv/include)
target_link_libraries(dxvk INTERFACE
    dxvk_d3d11 dxvk_dxgi display-info dxvkinternals)

# pe-parse subproject

include(FetchContent)
FetchContent_Declare(peparse_git
    GIT_REPOSITORY "https://github.com/trailofbits/pe-parse"
    GIT_TAG "v2.1.1"
)
FetchContent_MakeAvailable(peparse_git)

add_library(peparse INTERFACE)
add_dependencies(peparse peparse_git)

target_link_directories(peparse
    INTERFACE ${peparse_SOURCE_DIR}/pe-parser-library/include)
target_include_directories(peparse SYSTEM
    INTERFACE ${peparse_BUILD_DIR})
target_link_libraries(peparse
    INTERFACE pe-parse)

# main project

project(lsfg-vk-gen
    VERSION 0.0.1
    DESCRIPTION "lsfg-vk-gen: LSFG on Linux through Vulkan (backend)"
    LANGUAGES CXX)

file(GLOB BACKEND_SOURCES
    "src/core/*.cpp"
    "src/pool/*.cpp"
    "src/shaderchains/*.cpp"
    "src/*.cpp"
)

add_library(lsfg-vk-gen SHARED ${BACKEND_SOURCES})

target_include_directories(lsfg-vk-gen
    PRIVATE include
    PUBLIC public)
target_link_libraries(lsfg-vk-gen
    PUBLIC vulkan peparse crypto dxvk)
set_target_properties(lsfg-vk-gen
    PROPERTIES CXX_CLANG_TIDY clang-tidy)
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
    # required for peparse
    -Wno-unused-template
    # required for dxvk (yeah don't worry about it)
    -Qunused-arguments -Wl,--unresolved-symbols=ignore-all
)
