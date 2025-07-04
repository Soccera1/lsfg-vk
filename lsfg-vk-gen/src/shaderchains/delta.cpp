#include "shaderchains/delta.hpp"
#include "utils.hpp"

using namespace LSFG::Shaderchains;

Delta::Delta(const Core::Device& device, Pool::ShaderPool& shaderpool,
        const Core::DescriptorPool& pool,
        std::array<Core::Image, 2> inImgs,
        std::optional<Core::Image> optImg,
        size_t genc)
        : inImgs(std::move(inImgs)),
          optImg(std::move(optImg)) {
    this->shaderModules = {{
        shaderpool.getShader(device, "delta/0.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        shaderpool.getShader(device, "delta/1.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        shaderpool.getShader(device, "delta/2.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        shaderpool.getShader(device, "delta/3.spv",
            { { 2, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
              { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER } })
    }};
    for (size_t i = 0; i < 4; i++) {
        this->pipelines.at(i) = Core::Pipeline(device,
            this->shaderModules.at(i));
        if (i == 3) continue;
        this->descriptorSets.at(i) = Core::DescriptorSet(device, pool,
            this->shaderModules.at(i));
    }
    for (size_t i = 0; i < genc; i++)
        this->nDescriptorSets.emplace_back(device, pool,
            this->shaderModules.at(3));
    for (size_t i = 0; i < genc; i++) {
        auto data = Globals::fgBuffer;
        data.timestamp = static_cast<float>(i + 1) / static_cast<float>(genc + 1);
        data.firstIterS = !this->optImg.has_value();
        this->buffers.emplace_back(device, data, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }

    const auto extent = this->inImgs.at(0).getExtent();

    for (size_t i = 0; i < 2; i++) {
        this->tempImgs1.at(i) = Core::Image(device,
            extent,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
        this->tempImgs2.at(i) = Core::Image(device,
            extent,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
    }

    this->outImg = Core::Image(device,
        extent,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);

    this->descriptorSets.at(0).update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImgs)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1)
        .build();
    this->descriptorSets.at(1).update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs2)
        .build();
    this->descriptorSets.at(2).update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs2)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1)
        .build();
    for (size_t i = 0; i < genc; i++) {
        this->nDescriptorSets.at(i).update(device)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampEdge)
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1)
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->optImg)
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImg)
            .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffers.at(i))
            .build();
    }
}

void Delta::Dispatch(const Core::CommandBuffer& buf, uint64_t pass) {
    const auto extent = this->tempImgs1.at(0).getExtent();

    // first pass
    const uint32_t threadsX = (extent.width + 7) >> 3;
    const uint32_t threadsY = (extent.height + 7) >> 3;

    Utils::BarrierBuilder(buf)
        .addW2R(this->inImgs)
        .addR2W(this->tempImgs1)
        .build();

    this->pipelines.at(0).bind(buf);
    this->descriptorSets.at(0).bind(buf, this->pipelines.at(0));
    buf.dispatch(threadsX, threadsY, 1);

    // second pass
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs1)
        .addR2W(this->tempImgs2)
        .build();

    this->pipelines.at(1).bind(buf);
    this->descriptorSets.at(1).bind(buf, this->pipelines.at(1));
    buf.dispatch(threadsX, threadsY, 1);

    // third pass
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs2)
        .addR2W(this->tempImgs1)
        .build();

    this->pipelines.at(2).bind(buf);
    this->descriptorSets.at(2).bind(buf, this->pipelines.at(2));
    buf.dispatch(threadsX, threadsY, 1);

    // fourth pass
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs1)
        .addW2R(this->optImg)
        .addR2W(this->outImg)
        .build();

    this->pipelines.at(3).bind(buf);
    this->nDescriptorSets.at(pass).bind(buf, this->pipelines.at(3));
    buf.dispatch(threadsX, threadsY, 1);
}
