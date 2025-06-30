#include "shaderchains/downsample.hpp"
#include "utils.hpp"

using namespace Vulkan::Shaderchains;

Downsample::Downsample(const Device& device, const Core::DescriptorPool& pool,
        const Core::Image& inImage) : inImage(inImage) {
    // create internal resources
    this->shaderModule = Core::ShaderModule(device, "rsc/shaders/downsample.spv",
        { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
          { 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
          { 7, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
          { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER } });
    this->pipeline = Core::Pipeline(device, this->shaderModule);
    this->descriptorSet = Core::DescriptorSet(device, pool, this->shaderModule);

    const Globals::FgBuffer data = Globals::fgBuffer;
    this->buffer = Core::Buffer(device, data, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    auto extent = inImage.getExtent();

    // create output images
    this->outImages.resize(7);
    for (size_t i = 0; i < 7; i++)
        this->outImages.at(i) = Core::Image(device,
            { extent.width >> i, extent.height >> i },
            VK_FORMAT_R8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);

    // update descriptor set
    this->descriptorSet.update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, inImage)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImages)
        .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffer)
        .build();
}

void Downsample::Dispatch(const Core::CommandBuffer& buf) {
    auto extent = inImage.getExtent();
    const uint32_t threadsX = (extent.width + 63) >> 6;
    const uint32_t threadsY = (extent.height + 63) >> 6;

    Utils::insertBarrier(
        buf,
        this->outImages,
        { this->inImage }
    );

    this->pipeline.bind(buf);
    this->descriptorSet.bind(buf, this->pipeline);
    buf.dispatch(threadsX, threadsY, 1);
}
