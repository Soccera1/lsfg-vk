include(ExternalProject)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(BUILD_TYPE "release")
    set(STRIP_FLAG "--strip")
else()
    set(BUILD_TYPE "debug")
    set(STRIP_FLAG "")
endif()

ExternalProject_Add(dxvk_git
    GIT_REPOSITORY "https://github.com/doitsujin/dxvk"
    GIT_TAG "v2.6.2"
    UPDATE_DISCONNECTED true
    USES_TERMINAL_CONFIGURE true
    USES_TERMINAL_BUILD true
    BUILD_IN_SOURCE true
    CONFIGURE_COMMAND
        sed -i s/private://g
            src/dxvk/dxvk_shader.h &&
        CC=clang CXX=clang++ CFLAGS=-w CXXFLAGS=-w
        meson setup
            --buildtype ${BUILD_TYPE}
            --prefix <SOURCE_DIR>/build-native
            ${STRIP_FLAG}
            -Dbuild_id=false
            --force-fallback-for=libdisplay-info
            --wipe
            build
    BUILD_COMMAND
        ninja -C build install &&
        mv build/src/dxvk/libdxvk.a build/src/dxvk/libldxvk.a &&
        cd build/src/dxgi/libdxvk_dxgi.so.0.20602.p &&
        ar rcs -o ../libdxgi.a
            dxgi_adapter.cpp.o dxgi_enums.cpp.o dxgi_factory.cpp.o
            dxgi_format.cpp.o dxgi_main.cpp.o dxgi_monitor.cpp.o
            dxgi_options.cpp.o dxgi_output.cpp.o dxgi_surface.cpp.o
            dxgi_swapchain.cpp.o &&
        cd ../../d3d11/libdxvk_d3d11.so.0.20602.p &&
        ar rcs -o ../libd3d11.a
            d3d11_annotation.cpp.o d3d11_blend.cpp.o d3d11_buffer.cpp.o
            d3d11_class_linkage.cpp.o d3d11_cmdlist.cpp.o d3d11_context.cpp.o
            d3d11_context_def.cpp.o d3d11_context_ext.cpp.o d3d11_context_imm.cpp.o
            d3d11_cuda.cpp.o d3d11_depth_stencil.cpp.o d3d11_device.cpp.o
            d3d11_enums.cpp.o d3d11_features.cpp.o d3d11_fence.cpp.o
            d3d11_gdi.cpp.o d3d11_initializer.cpp.o d3d11_input_layout.cpp.o
            d3d11_interop.cpp.o d3d11_main.cpp.o d3d11_on_12.cpp.o
            d3d11_options.cpp.o d3d11_query.cpp.o d3d11_rasterizer.cpp.o
            d3d11_resource.cpp.o d3d11_sampler.cpp.o d3d11_shader.cpp.o
            d3d11_state.cpp.o d3d11_state_object.cpp.o d3d11_swapchain.cpp.o
            d3d11_texture.cpp.o d3d11_util.cpp.o d3d11_video.cpp.o
            d3d11_view_dsv.cpp.o d3d11_view_rtv.cpp.o d3d11_view_srv.cpp.o
            d3d11_view_uav.cpp.o
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(dxvk_git SOURCE_DIR)

add_library(dxvk INTERFACE)
add_dependencies(dxvk dxvk_git)

target_link_directories(dxvk
    INTERFACE ${SOURCE_DIR}/build/src/dxvk
    INTERFACE ${SOURCE_DIR}/build/src/dxbc
    INTERFACE ${SOURCE_DIR}/build/src/dxgi
    INTERFACE ${SOURCE_DIR}/build/src/d3d11
    INTERFACE ${SOURCE_DIR}/build/src/spirv
    INTERFACE ${SOURCE_DIR}/build/src/util)
target_include_directories(dxvk SYSTEM
    INTERFACE ${SOURCE_DIR}/build-native/include/dxvk
    INTERFACE ${SOURCE_DIR}/src
    INTERFACE ${SOURCE_DIR}/include/spirv/include)
target_link_libraries(dxvk INTERFACE
    -Wl,--start-group dxgi d3d11 util ldxvk dxbc spirv -Wl,--end-group)
