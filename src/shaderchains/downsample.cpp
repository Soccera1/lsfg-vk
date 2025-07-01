#include "shaderchains/downsample.hpp"
#include "utils.hpp"

using namespace LSFG::Shaderchains;

Downsample::Downsample(const Device& device, const Core::DescriptorPool& pool,
        Core::Image inImg)
        : inImg(std::move(inImg)) {
    this->shaderModule = Core::ShaderModule(device, "rsc/shaders/downsample.spv",
        { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
          { 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
          { 7, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
          { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER } });
    this->pipeline = Core::Pipeline(device, this->shaderModule);
    this->descriptorSet = Core::DescriptorSet(device, pool, this->shaderModule);
    this->buffer = Core::Buffer(device, Globals::fgBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    auto extent = this->inImg.getExtent();
    for (size_t i = 0; i < 7; i++)
        this->outImgs.at(i) = Core::Image(device,
            { extent.width >> i, extent.height >> i },
            VK_FORMAT_R8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);

    this->descriptorSet.update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImgs)
        .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffer)
        .build();
}

void Downsample::Dispatch(const Core::CommandBuffer& buf) {
    auto extent = this->inImg.getExtent();

    // first pass
    const uint32_t threadsX = (extent.width + 63) >> 6;
    const uint32_t threadsY = (extent.height + 63) >> 6;

    Utils::BarrierBuilder(buf)
        .addW2R(this->inImg)
        .addR2W(this->outImgs)
        .build();

    this->pipeline.bind(buf);
    this->descriptorSet.bind(buf, this->pipeline);
    buf.dispatch(threadsX, threadsY, 1);
}
