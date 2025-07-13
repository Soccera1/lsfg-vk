include(ExternalProject)

ExternalProject_Add(dxbc_git
    GIT_REPOSITORY "https://github.com/PancakeTAS/dxbc"
    GIT_TAG "d77124f"
    UPDATE_DISCONNECTED true
    USES_TERMINAL_CONFIGURE true
    USES_TERMINAL_BUILD true
    BUILD_IN_SOURCE true
    CONFIGURE_COMMAND
        cmake -S . -B build -G Ninja
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_C_COMPILER=clang
            -DCMAKE_CXX_COMPILER=clang++
            -DCMAKE_C_FLAGS=-fPIC
            -DCMAKE_CXX_FLAGS=-fPIC
    BUILD_COMMAND
        ninja -C build
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(dxbc_git SOURCE_DIR)

add_library(dxvk_dxbc INTERFACE)
add_dependencies(dxvk_dxbc dxbc_git)

target_link_directories(dxvk_dxbc
    INTERFACE ${SOURCE_DIR}/build)
target_include_directories(dxvk_dxbc SYSTEM
    INTERFACE ${SOURCE_DIR}/include/dxbc
    INTERFACE ${SOURCE_DIR}/include/spirv ${SOURCE_DIR}/include/util ${SOURCE_DIR}/include/dxvk)
target_link_libraries(dxvk_dxbc
    INTERFACE dxbc)
