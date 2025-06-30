#include "shaderchains/beta.hpp"
#include "utils.hpp"

using namespace Vulkan::Shaderchains;

Beta::Beta(const Device& device, const Core::DescriptorPool& pool,
        std::array<Core::Image, 8> temporalImgs,
        std::array<Core::Image, 4> inImgs)
        : temporalImgs(std::move(temporalImgs)),
          inImgs(std::move(inImgs)) {
    this->shaderModules = {{
        Core::ShaderModule(device, "rsc/shaders/beta/0.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 8+4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
              { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER } }),
        Core::ShaderModule(device, "rsc/shaders/beta/1.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
              { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER } }),
        Core::ShaderModule(device, "rsc/shaders/beta/2.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
              { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER } }),
        Core::ShaderModule(device, "rsc/shaders/beta/3.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
              { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER } }),
        Core::ShaderModule(device, "rsc/shaders/beta/4.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 6, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
              { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER } })
    }};
    for (size_t i = 0; i < 5; i++) {
        this->pipelines.at(i) = Core::Pipeline(device,
            this->shaderModules.at(i));
        this->descriptorSets.at(i) = Core::DescriptorSet(device, pool,
            this->shaderModules.at(i));
    }
    this->buffer = Core::Buffer(device, Globals::fgBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    const auto extent = this->temporalImgs.at(0).getExtent();

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

    this->descriptorSets.at(0).update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->temporalImgs)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImgs)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1)
        .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffer)
        .build();
    this->descriptorSets.at(1).update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs2)
        .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffer)
        .build();
    this->descriptorSets.at(2).update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs2)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1)
        .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffer)
        .build();
    this->descriptorSets.at(3).update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs2)
        .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffer)
        .build();
    this->descriptorSets.at(4).update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs2)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImgs)
        .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffer)
        .build();
}

void Beta::Dispatch(const Core::CommandBuffer& buf) {
    const auto extent = this->tempImgs1.at(0).getExtent();

    // first pass
    uint32_t threadsX = (extent.width + 7) >> 3;
    uint32_t threadsY = (extent.height + 7) >> 3;

    Utils::BarrierBuilder(buf)
        .addW2R(this->temporalImgs)
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
        .addR2W(this->tempImgs2)
        .build();

    this->pipelines.at(3).bind(buf);
    this->descriptorSets.at(3).bind(buf, this->pipelines.at(3));
    buf.dispatch(threadsX, threadsY, 1);

    // fifth pass
    threadsX = (extent.width + 31) >> 5;
    threadsY = (extent.height + 31) >> 5;

    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs2)
        .addR2W(this->outImgs)
        .build();

    this->pipelines.at(4).bind(buf);
    this->descriptorSets.at(4).bind(buf, this->pipelines.at(4));
    buf.dispatch(threadsX, threadsY, 1);
}
