#include "shaderchains/magic.hpp"
#include "utils.hpp"

using namespace LSFG::Shaderchains;

Magic::Magic(const Device& device, const Core::DescriptorPool& pool,
        std::array<Core::Image, 4> temporalImgs,
        std::array<Core::Image, 4> inImgs1,
        Core::Image inImg2,
        Core::Image inImg3,
        std::optional<Core::Image> optImg)
        : temporalImgs(std::move(temporalImgs)),
          inImgs1(std::move(inImgs1)),
          inImg2(std::move(inImg2)), inImg3(std::move(inImg3)),
          optImg(std::move(optImg)) {
    this->shaderModule = Core::ShaderModule(device, "rsc/shaders/magic.spv",
        { { 2,       VK_DESCRIPTOR_TYPE_SAMPLER },
          { 4+4+2+1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
          { 3+3+2,   VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
          { 1,       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER } });
    this->pipeline = Core::Pipeline(device, this->shaderModule);
    this->descriptorSet = Core::DescriptorSet(device, pool, this->shaderModule);

    Globals::FgBuffer data = Globals::fgBuffer;
    data.firstIterS = !this->optImg.has_value();
    this->buffer = Core::Buffer(device, data, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    auto extent = this->temporalImgs.at(0).getExtent();

    for (size_t i = 0; i < 2; i++)
        this->outImgs1.at(i) = Core::Image(device,
            extent,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
    for (size_t i = 0; i < 3; i++)
        this->outImgs2.at(i) = Core::Image(device,
            extent,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
    for (size_t i = 0; i < 3; i++)
        this->outImgs3.at(i) = Core::Image(device,
            extent,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);

    this->descriptorSet.update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampEdge)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->temporalImgs)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImgs1)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg2)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg3)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->optImg)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImgs3)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImgs2)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImgs1)
        .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffer)
        .build();
}

void Magic::Dispatch(const Core::CommandBuffer& buf) {
    auto extent = this->temporalImgs.at(0).getExtent();

    // first pass
    const uint32_t threadsX = (extent.width + 7) >> 3;
    const uint32_t threadsY = (extent.height + 7) >> 3;

    Utils::BarrierBuilder(buf)
        .addW2R(this->temporalImgs)
        .addW2R(this->inImgs1)
        .addW2R(this->inImg2)
        .addW2R(this->inImg3)
        .addW2R(this->optImg)
        .addR2W(this->outImgs3)
        .addR2W(this->outImgs2)
        .addR2W(this->outImgs1)
        .build();

    this->pipeline.bind(buf);
    this->descriptorSet.bind(buf, this->pipeline);
    buf.dispatch(threadsX, threadsY, 1);
}
