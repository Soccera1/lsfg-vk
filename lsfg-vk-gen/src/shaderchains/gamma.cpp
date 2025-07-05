#include "shaderchains/gamma.hpp"
#include "utils/utils.hpp"

using namespace LSFG::Shaderchains;

Gamma::Gamma(const Core::Device& device, Pool::ShaderPool& shaderpool,
        const Core::DescriptorPool& pool,
        std::array<Core::Image, 4> inImgs1_0,
        std::array<Core::Image, 4> inImgs1_1,
        std::array<Core::Image, 4> inImgs1_2,
        Core::Image inImg2,
        std::optional<Core::Image> optImg1, // NOLINT
        std::optional<Core::Image> optImg2,
        VkExtent2D outExtent,
        size_t genc)
        : inImgs1_0(std::move(inImgs1_0)),
          inImgs1_1(std::move(inImgs1_1)),
          inImgs1_2(std::move(inImgs1_2)),
          inImg2(std::move(inImg2)),
          optImg2(std::move(optImg2)) {
    this->shaderModules = {{
        shaderpool.getShader(device, "gamma/0.spv",
            { { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 10, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        shaderpool.getShader(device, "gamma/1.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        shaderpool.getShader(device, "gamma/2.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        shaderpool.getShader(device, "gamma/3.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        shaderpool.getShader(device, "gamma/4.spv",
            { { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 6, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        shaderpool.getShader(device, "gamma/5.spv",
            { { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } })
    }};
    for (size_t i = 0; i < 6; i++) {
        this->pipelines.at(i) = Core::Pipeline(device,
            this->shaderModules.at(i));
        if (i == 0 || i >= 4) continue; // first shader has special logic
        this->descriptorSets.at(i - 1) = Core::DescriptorSet(device, pool,
            this->shaderModules.at(i));
    }
    for (size_t i = 0; i < genc; i++)
        this->n1DescriptorSets.emplace_back(device, pool,
            this->shaderModules.at(4));
    for (size_t i = 0; i < genc; i++)
        this->n2DescriptorSets.emplace_back(device, pool,
            this->shaderModules.at(5));
    for (size_t i = 0; i < genc; i++) {
        this->nSpecialDescriptorSets.emplace_back();
        for (size_t j = 0; j < 3; j++)
            this->nSpecialDescriptorSets.at(i).at(j) = Core::DescriptorSet(device, pool,
                this->shaderModules.at(0));
    }
    for (size_t i = 0; i < genc; i++) {
        auto data = Globals::fgBuffer;
        data.timestamp = static_cast<float>(i + 1) / static_cast<float>(genc + 1);
        data.firstIter = !optImg1.has_value();
        this->buffers.emplace_back(device, data, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }

    const auto extent = this->inImgs1_0.at(0).getExtent();

    this->optImg1 = optImg1.value_or(Core::Image(device, extent,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
        | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT));

    for (size_t i = 0; i < 4; i++) {
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
    this->whiteImg = Core::Image(device, outExtent,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);

    this->outImg1 = Core::Image(device,
        extent,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);
    this->outImg2 = Core::Image(device,
        outExtent,
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
            this->nSpecialDescriptorSets.at(i).at(fc).update(device)
                .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffers.at(i))
                .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
                .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampEdge)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, *prevImgs1)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, *nextImgs1)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->optImg1)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->optImg2)
                .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1.at(0))
                .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1.at(1))
                .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1.at(2))
                .build();
        }
    }
    this->descriptorSets.at(0).update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1.at(0))
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1.at(1))
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1.at(2))
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
        this->n1DescriptorSets.at(i).update(device)
            .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffers.at(i))
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampEdge)
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs2)
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->optImg2)
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg2)
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImg1)
            .build();
        this->n2DescriptorSets.at(i).update(device)
            .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffers.at(i))
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampEdge)
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->whiteImg)
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->outImg1)
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImg2)
            .build();
    }

    // clear white image and optImg1 if needed
    Utils::clearImage(device, this->whiteImg, true);
    if (!optImg1.has_value())
        Utils::clearImage(device, this->optImg1);
}

void Gamma::Dispatch(const Core::CommandBuffer& buf, uint64_t fc, uint64_t pass) {
    const auto extent = this->tempImgs1.at(0).getExtent();

    // first pass
    uint32_t threadsX = (extent.width + 7) >> 3;
    uint32_t threadsY = (extent.height + 7) >> 3;

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
        .addW2R(this->optImg1)
        .addW2R(this->optImg2)
        .addR2W(this->tempImgs1.at(0))
        .addR2W(this->tempImgs1.at(1))
        .addR2W(this->tempImgs1.at(2))
        .build();

    this->pipelines.at(0).bind(buf);
    this->nSpecialDescriptorSets.at(pass).at(fc % 3).bind(buf, this->pipelines.at(0));
    buf.dispatch(threadsX, threadsY, 1);

    // second pass
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs1.at(0))
        .addW2R(this->tempImgs1.at(1))
        .addW2R(this->tempImgs1.at(2))
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
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs2)
        .addW2R(this->optImg2)
        .addW2R(this->inImg2)
        .addR2W(this->outImg1)
        .build();

    this->pipelines.at(4).bind(buf);
    this->n1DescriptorSets.at(pass).bind(buf, this->pipelines.at(4));
    buf.dispatch(threadsX, threadsY, 1);

    // sixth pass
    threadsX = (extent.width + 3) >> 2;
    threadsY = (extent.height + 3) >> 2;

    Utils::BarrierBuilder(buf)
        .addW2R(this->whiteImg)
        .addW2R(this->outImg1)
        .addR2W(this->outImg2)
        .build();

    this->pipelines.at(5).bind(buf);
    this->n2DescriptorSets.at(pass).bind(buf, this->pipelines.at(5));
    buf.dispatch(threadsX, threadsY, 1);
}
