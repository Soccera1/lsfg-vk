#include "shaderchains/beta.hpp"
#include "utils.hpp"

using namespace LSFG::Shaderchains;

Beta::Beta(const Core::Device& device, const Core::DescriptorPool& pool,
        std::array<Core::Image, 4> inImgs_0,
        std::array<Core::Image, 4> inImgs_1,
        std::array<Core::Image, 4> inImgs_2,
        size_t genc)
        : inImgs_0(std::move(inImgs_0)),
          inImgs_1(std::move(inImgs_1)),
          inImgs_2(std::move(inImgs_2)) {
    this->shaderModules = {{
        Core::ShaderModule(device, "rsc/shaders/beta/0.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 8+4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        Core::ShaderModule(device, "rsc/shaders/beta/1.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        Core::ShaderModule(device, "rsc/shaders/beta/2.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        Core::ShaderModule(device, "rsc/shaders/beta/3.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        Core::ShaderModule(device, "rsc/shaders/beta/4.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 6, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
              { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER } })
    }};
    for (size_t i = 0; i < 5; i++) {
        this->pipelines.at(i) = Core::Pipeline(device,
            this->shaderModules.at(i));
        if (i == 0 || i == 4) continue; // first shader has special logic
        this->descriptorSets.at(i - 1) = Core::DescriptorSet(device, pool,
            this->shaderModules.at(i));
    }
    for (size_t i = 0; i < 3; i++)
        this->specialDescriptorSets.at(i) = Core::DescriptorSet(device, pool,
            this->shaderModules.at(0));
    for (size_t i = 0; i < genc; i++)
        this->nDescriptorSets.emplace_back(device, pool,
            this->shaderModules.at(4));
    for (size_t i = 0; i < genc; i++) {
        auto data = Globals::fgBuffer;
        data.timestamp = static_cast<float>(i + 1) / static_cast<float>(genc + 1);
        this->buffers.emplace_back(device, data, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }

    const auto extent = this->inImgs_0.at(0).getExtent();

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

    for (size_t i = 0; i < 6; i++) {
        this->outImgs.at(i) = Core::Image(device,
            { extent.width >> i, extent.height >> i },
            VK_FORMAT_R8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
    }

    for (size_t fc = 0; fc < 3; fc++) {
        auto* nextImgs = &this->inImgs_0;
        auto* prevImgs = &this->inImgs_2;
        auto* pprevImgs = &this->inImgs_1;
        if (fc == 1) {
            nextImgs = &this->inImgs_1;
            prevImgs = &this->inImgs_0;
            pprevImgs = &this->inImgs_2;
        } else if (fc == 2) {
            nextImgs = &this->inImgs_2;
            prevImgs = &this->inImgs_1;
            pprevImgs = &this->inImgs_0;
        }
        this->specialDescriptorSets.at(fc).update(device)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, *pprevImgs)
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, *prevImgs)
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, *nextImgs)
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1)
            .build();
    }
    this->descriptorSets.at(0).update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs2)
        .build();
    this->descriptorSets.at(1).update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs2)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1)
        .build();
    this->descriptorSets.at(2).update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs2)
        .build();
    for (size_t i = 0; i < genc; i++) {
        this->nDescriptorSets.at(i).update(device)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs2)
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImgs)
            .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffers.at(i))
            .build();
    }
}

void Beta::Dispatch(const Core::CommandBuffer& buf, uint64_t fc, uint64_t pass) {
    const auto extent = this->tempImgs1.at(0).getExtent();

    // first pass
    uint32_t threadsX = (extent.width + 7) >> 3;
    uint32_t threadsY = (extent.height + 7) >> 3;

    Utils::BarrierBuilder(buf)
        .addW2R(this->inImgs_0)
        .addW2R(this->inImgs_1)
        .addW2R(this->inImgs_2)
        .addR2W(this->tempImgs1)
        .build();

    this->pipelines.at(0).bind(buf);
    this->specialDescriptorSets.at(fc % 3).bind(buf, this->pipelines.at(0));
    buf.dispatch(threadsX, threadsY, 1);

    // second pass
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs1)
        .addR2W(this->tempImgs2)
        .build();

    this->pipelines.at(1).bind(buf);
    this->descriptorSets.at(0).bind(buf, this->pipelines.at(1));
    buf.dispatch(threadsX, threadsY, 1);

    // third pass
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs2)
        .addR2W(this->tempImgs1)
        .build();

    this->pipelines.at(2).bind(buf);
    this->descriptorSets.at(1).bind(buf, this->pipelines.at(2));
    buf.dispatch(threadsX, threadsY, 1);

    // fourth pass
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs1)
        .addR2W(this->tempImgs2)
        .build();

    this->pipelines.at(3).bind(buf);
    this->descriptorSets.at(2).bind(buf, this->pipelines.at(3));
    buf.dispatch(threadsX, threadsY, 1);

    // fifth pass
    threadsX = (extent.width + 31) >> 5;
    threadsY = (extent.height + 31) >> 5;

    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs2)
        .addR2W(this->outImgs)
        .build();

    this->pipelines.at(4).bind(buf);
    this->nDescriptorSets.at(pass).bind(buf, this->pipelines.at(4));
    buf.dispatch(threadsX, threadsY, 1);
}
