#include "core/commandbuffer.hpp"
#include "core/commandpool.hpp"
#include "core/fence.hpp"
#include "core/pipeline.hpp"
#include "core/shadermodule.hpp"
#include "device.hpp"
#include "instance.hpp"

#include <cassert>
#include <iostream>

using namespace Vulkan;

int main() {
    // initialize Vulkan
    const Instance instance;
    const Device device(instance);

    // prepare render pass
    const Core::CommandPool commandPool(device);

    // prepare shader
    const Core::ShaderModule computeShader(device, "shaders/downsample.spv",
        { { 1, VK_DESCRIPTOR_TYPE_SAMPLER},
          { 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
          { 7, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE},
          { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER} });
    const Core::Pipeline computePipeline(device, computeShader);

    // start pass
    Core::Fence fence(device);

    Core::CommandBuffer commandBuffer(device, commandPool);
    commandBuffer.begin();

    // end pass
    commandBuffer.end();

    commandBuffer.submit(device.getComputeQueue(), fence);
    assert(fence.wait() && "Synchronization fence timed out");

    std::cerr << "Application finished" << '\n';
    return 0;
}
