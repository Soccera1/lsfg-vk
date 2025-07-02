#include "shaderchains/magic.hpp"
#include "utils.hpp"

using namespace LSFG::Shaderchains;

Magic::Magic(const Core::Device& device, const Core::DescriptorPool& pool,
    std::array<Core::Image, 4> inImgs1_0,
    std::array<Core::Image, 4> inImgs1_1,
    std::array<Core::Image, 4> inImgs1_2,
        Core::Image inImg2,
        Core::Image inImg3,
        std::optional<Core::Image> optImg,
        size_t genc)
        : inImgs1_0(std::move(inImgs1_0)),
          inImgs1_1(std::move(inImgs1_1)),
          inImgs1_2(std::move(inImgs1_2)),
          inImg2(std::move(inImg2)), inImg3(std::move(inImg3)),
          optImg(std::move(optImg)) {
    this->shaderModule = Core::ShaderModule(device, "rsc/shaders/magic.spv",
        { { 2,       VK_DESCRIPTOR_TYPE_SAMPLER },
          { 4+4+2+1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
          { 3+3+2,   VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
          { 1,       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER } });
    this->pipeline = Core::Pipeline(device, this->shaderModule);
    for (size_t i = 0; i < genc; i++) {
        this->nDescriptorSets.emplace_back();
        for (size_t j = 0; j < 3; j++)
            this->nDescriptorSets.at(i).at(j) = Core::DescriptorSet(device, pool, this->shaderModule);
    }
    for (size_t i = 0; i < genc; i++) {
        auto data = Globals::fgBuffer;
        data.timestamp = static_cast<float>(i + 1) / static_cast<float>(genc + 1);
        data.firstIterS = !this->optImg.has_value();
        this->buffers.emplace_back(device, data, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }

    auto extent = this->inImgs1_0.at(0).getExtent();

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

    for (size_t fc = 0; fc < 3; fc++) {
        auto* nextImgs1 = &this->inImgs1_0;
        auto* prevImgs1 = &this->inImgs1_2;
        if (fc == 1) {
            nextImgs1 = &this->inImgs1_1;
            prevImgs1 = &this->inImgs1_0;
        } else if (fc == 2) {
            nextImgs1 = &this->inImgs1_2;
            prevImgs1 = &this->inImgs1_1;
        }
        for (size_t i = 0; i < genc; i++) {
            this->nDescriptorSets.at(i).at(fc).update(device)
                .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
                .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampEdge)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, *prevImgs1)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, *nextImgs1)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg2)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg3)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->optImg)
                .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImgs3)
                .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImgs2)
                .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImgs1)
                .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffers.at(i))
                .build();
        }
    }
}

void Magic::Dispatch(const Core::CommandBuffer& buf, uint64_t fc, uint64_t pass) {
    auto extent = this->inImgs1_0.at(0).getExtent();

    // first pass
    const uint32_t threadsX = (extent.width + 7) >> 3;
    const uint32_t threadsY = (extent.height + 7) >> 3;

    auto* nextImgs1 = &this->inImgs1_0;
    auto* prevImgs1 = &this->inImgs1_2;
    if ((fc % 3) == 1) {
        nextImgs1 = &this->inImgs1_1;
        prevImgs1 = &this->inImgs1_0;
    } else if ((fc % 3) == 2) {
        nextImgs1 = &this->inImgs1_2;
        prevImgs1 = &this->inImgs1_1;
    }
    Utils::BarrierBuilder(buf)
        .addW2R(*prevImgs1)
        .addW2R(*nextImgs1)
        .addW2R(this->inImg2)
        .addW2R(this->inImg3)
        .addW2R(this->optImg)
        .addR2W(this->outImgs3)
        .addR2W(this->outImgs2)
        .addR2W(this->outImgs1)
        .build();

    this->pipeline.bind(buf);
    this->nDescriptorSets.at(pass).at(fc % 3).bind(buf, this->pipeline);
    buf.dispatch(threadsX, threadsY, 1);
}
