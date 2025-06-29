#include "core/pipeline.hpp"
#include "core/shadermodule.hpp"
#include "device.hpp"
#include "instance.hpp"

#include <iostream>
#include <vulkan/vulkan_core.h>

int main() {
    const Vulkan::Instance instance;
    const Vulkan::Device device(instance);

    const Vulkan::Core::ShaderModule computeShader(device, "shaders/downsample.spv",
        { { 1, VK_DESCRIPTOR_TYPE_SAMPLER},
          { 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
          { 7, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE},
          { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER} });
    const Vulkan::Core::Pipeline computePipeline(device, computeShader);

    std::cerr << "Application finished" << '\n';
    return 0;
}
