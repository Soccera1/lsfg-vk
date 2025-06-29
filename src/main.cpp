#include "core/shadermodule.hpp"
#include "device.hpp"
#include "instance.hpp"

#include <iostream>
#include <vulkan/vulkan_core.h>

int main() {
    const Vulkan::Instance instance;
    const Vulkan::Device device(instance);

    const Vulkan::Core::ShaderModule computeShader(device, "shaders/downsample.spv",
        {
            VK_DESCRIPTOR_TYPE_SAMPLER,
            VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
        }
    );

    std::cerr << "Application finished" << '\n';
    return 0;
}
