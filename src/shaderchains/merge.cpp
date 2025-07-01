#include "shaderchains/merge.hpp"
#include "utils.hpp"

using namespace LSFG::Shaderchains;

Merge::Merge(const Device& device, const Core::DescriptorPool& pool,
        Core::Image inImg1,
        Core::Image inImg2,
        Core::Image inImg3,
        Core::Image inImg4,
        Core::Image inImg5)
        : inImg1(std::move(inImg1)),
          inImg2(std::move(inImg2)),
          inImg3(std::move(inImg3)),
          inImg4(std::move(inImg4)),
          inImg5(std::move(inImg5)) {
    this->shaderModule = Core::ShaderModule(device, "rsc/shaders/merge.spv",
        { { 2, VK_DESCRIPTOR_TYPE_SAMPLER },
          { 5, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
          { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
          { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER } });
    this->pipeline = Core::Pipeline(device, this->shaderModule);
    this->descriptorSet = Core::DescriptorSet(device, pool, this->shaderModule);
    this->buffer = Core::Buffer(device, Globals::fgBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    auto extent = this->inImg1.getExtent();

    this->outImg = Core::Image(device,
        extent,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);

    this->descriptorSet.update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampEdge)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg1)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg2)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg3)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg4)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg5)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImg)
        .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffer)
        .build();
}

void Merge::Dispatch(const Core::CommandBuffer& buf) {
    auto extent = this->inImg1.getExtent();

    // first pass
    const uint32_t threadsX = (extent.width + 15) >> 4;
    const uint32_t threadsY = (extent.height + 15) >> 4;

    Utils::BarrierBuilder(buf)
        .addW2R(this->inImg1)
        .addW2R(this->inImg2)
        .addW2R(this->inImg3)
        .addW2R(this->inImg4)
        .addW2R(this->inImg5)
        .addR2W(this->outImg)
        .build();

    this->pipeline.bind(buf);
    this->descriptorSet.bind(buf, this->pipeline);
    buf.dispatch(threadsX, threadsY, 1);
}
