#include "core/commandbuffer.hpp"
#include "core/commandpool.hpp"
#include "core/descriptorpool.hpp"
#include "core/fence.hpp"
#include "core/image.hpp"
#include "device.hpp"
#include "instance.hpp"
#include "shaderchains/downsample.hpp"
#include "utils/global.hpp"

#include <iostream>

using namespace Vulkan;

int main() {
    // initialize application
    const Instance instance;
    const Device device(instance);
    const Core::DescriptorPool descriptorPool(device);
    const Core::CommandPool commandPool(device);
    const Core::Fence fence(device);

    Globals::initializeGlobals(device);

    // create initialization resources
    const Core::Image inputImage(
        device, { 2560, 1411 }, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    // create the shaderchains
    Shaderchains::Downsample downsample(device, descriptorPool, inputImage);

    // start the rendering pipeline
    Core::CommandBuffer commandBuffer(device, commandPool);
    commandBuffer.begin();

    downsample.Dispatch(commandBuffer);

    // finish the rendering pipeline
    commandBuffer.end();

    commandBuffer.submit(device.getComputeQueue(), fence);
    if (!fence.wait(device)) {
        Globals::uninitializeGlobals();

        std::cerr << "Application failed due to timeout" << '\n';
        return 1;
    }

    Globals::uninitializeGlobals();

    std::cerr << "Application finished" << '\n';
    return 0;
}
