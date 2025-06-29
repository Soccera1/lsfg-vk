#include "core/buffer.hpp"
#include "core/commandbuffer.hpp"
#include "core/commandpool.hpp"
#include "core/descriptorpool.hpp"
#include "core/descriptorset.hpp"
#include "core/fence.hpp"
#include "core/image.hpp"
#include "core/pipeline.hpp"
#include "core/sampler.hpp"
#include "core/shadermodule.hpp"
#include "device.hpp"
#include "instance.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

using namespace Vulkan;

struct DataBuffer {
    std::array<uint32_t, 2> inputOffset;
    uint32_t firstIter;
    uint32_t firstIterS;
    uint32_t advancedColorKind;
    uint32_t hdrSupport;
    float resolutionInvScale;
    float timestamp;
    float uiThreshold;
    std::array<uint32_t, 3> pad;
};

const static DataBuffer data{
    .inputOffset = { 0, 29 },
    .resolutionInvScale = 1.0F,
    .timestamp = 0.5,
    .uiThreshold = 0.1F
};

int main() {
    // initialize Vulkan
    const Instance instance;
    const Device device(instance);

    // load shader
    const Core::ShaderModule computeShader(device, "shaders/downsample.spv",
        { { 1, VK_DESCRIPTOR_TYPE_SAMPLER},
          { 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
          { 7, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE},
          { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER} });

    // prepare render pass
    const Core::CommandPool commandPool(device);
    const Core::DescriptorPool descriptorPool(device);
    const Core::Pipeline computePipeline(device, computeShader);
    const Core::DescriptorSet descriptorSet(device, descriptorPool, computeShader);

    // create shader inputs
    const Core::Sampler sampler(device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

    const std::vector<Core::Image> images(1, Core::Image(
        device, { 2560, 1411 }, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    ));

    const auto* dataBuffer = reinterpret_cast<const uint8_t*>(&data);
    std::vector<uint8_t> dataVec(sizeof(DataBuffer));
    std::copy_n(dataBuffer, sizeof(DataBuffer), dataVec.data());
    const Core::Buffer buffer(device, dataVec.size(), dataVec,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    // create shader outputs
    const std::vector<Core::Image> outputImages = {
        Core::Image(
            device, { 1280, 705 }, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        ),
        Core::Image(
            device, { 640, 352 }, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        ),
        Core::Image(
            device, { 320, 176 }, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        ),
        Core::Image(
            device, { 160, 88 }, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        ),
        Core::Image(
            device, { 80, 44 }, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        ),
        Core::Image(
            device, { 40, 22 }, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        ),
        Core::Image(
            device, { 20, 11 }, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        )
    };

    // load descriptor set
    descriptorSet.update(
        device,
        {
            {{ VK_DESCRIPTOR_TYPE_SAMPLER, sampler }},
            {{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, images[0] }},
            {
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, outputImages[0] },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, outputImages[1] },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, outputImages[2] },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, outputImages[3] },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, outputImages[4] },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, outputImages[5] },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, outputImages[6] }
            },
            {{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buffer }}
        }
    );

    // start pass
    Core::Fence fence(device);

    Core::CommandBuffer commandBuffer(device, commandPool);
    commandBuffer.begin();

    // render
    computePipeline.bind(commandBuffer);
    descriptorSet.bind(commandBuffer, computePipeline);
    commandBuffer.dispatch(40, 23, 1);

    // end pass
    commandBuffer.end();

    commandBuffer.submit(device.getComputeQueue(), fence);
    assert(fence.wait(device) && "Synchronization fence timed out");

    std::cerr << "Application finished" << '\n';
    return 0;
}
