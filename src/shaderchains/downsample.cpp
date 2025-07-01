#include "shaderchains/downsample.hpp"
#include "utils.hpp"

using namespace LSFG::Shaderchains;

Downsample::Downsample(const Device& device, const Core::DescriptorPool& pool,
        Core::Image inImg_0, Core::Image inImg_1)
        : inImg_0(std::move(inImg_0)),
          inImg_1(std::move(inImg_1)) {
    this->shaderModule = Core::ShaderModule(device, "rsc/shaders/downsample.spv",
        { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
          { 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
          { 7, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
          { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER } });
    this->pipeline = Core::Pipeline(device, this->shaderModule);
    for (size_t i = 0; i < 2; i++)
        this->descriptorSets.at(i) = Core::DescriptorSet(device, pool, this->shaderModule);
    this->buffer = Core::Buffer(device, Globals::fgBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    auto extent = this->inImg_0.getExtent();
    for (size_t i = 0; i < 7; i++)
        this->outImgs.at(i) = Core::Image(device,
            { extent.width >> i, extent.height >> i },
            VK_FORMAT_R8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);

    for (size_t fc = 0; fc < 2; fc++) {
        auto& inImg = (fc % 2 == 0) ? this->inImg_0 : this->inImg_1;
        this->descriptorSets.at(fc).update(device)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, inImg)
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImgs)
            .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffer)
            .build();
    }
}

void Downsample::Dispatch(const Core::CommandBuffer& buf, uint64_t fc) {
    auto extent = this->inImg_0.getExtent();

    // first pass
    const uint32_t threadsX = (extent.width + 63) >> 6;
    const uint32_t threadsY = (extent.height + 63) >> 6;

    auto& inImg = (fc % 2 == 0) ? this->inImg_0 : this->inImg_1;
    Utils::BarrierBuilder(buf)
        .addW2R(inImg)
        .addR2W(this->outImgs)
        .build();

    this->pipeline.bind(buf);
    this->descriptorSets.at(fc % 2).bind(buf, this->pipeline);
    buf.dispatch(threadsX, threadsY, 1);
}
