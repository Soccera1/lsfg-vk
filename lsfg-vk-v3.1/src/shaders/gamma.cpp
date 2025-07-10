#include "shaders/gamma.hpp"
#include "utils/utils.hpp"
#include "core/commandbuffer.hpp"
#include "core/image.hpp"

#include <vulkan/vulkan_core.h>

#include <array>
#include <optional>
#include <utility>
#include <cstddef>
#include <cstdint>

using namespace LSFG::Shaders;

Gamma::Gamma(Vulkan& vk, std::array<std::array<Core::Image, 4>, 3> inImgs1,
        Core::Image inImg2,
        std::optional<Core::Image> optImg)
        : inImgs1(std::move(inImgs1)), inImg2(std::move(inImg2)),
          optImg(std::move(optImg)) {
    // create resources
    this->shaderModules = {{
        vk.shaders.getShader(vk.device, "gamma[0]",
            { { 1 , VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 9, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "gamma[1]",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "gamma[2]",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "gamma[3]",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        vk.shaders.getShader(vk.device, "gamma[4]",
            { { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
              { 2, VK_DESCRIPTOR_TYPE_SAMPLER },
              { 6, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
              { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } })
    }};
    this->pipelines = {{
        vk.shaders.getPipeline(vk.device, "gamma[0]"),
        vk.shaders.getPipeline(vk.device, "gamma[1]"),
        vk.shaders.getPipeline(vk.device, "gamma[2]"),
        vk.shaders.getPipeline(vk.device, "gamma[3]"),
        vk.shaders.getPipeline(vk.device, "gamma[4]")
    }};
    this->samplers.at(0) = vk.resources.getSampler(vk.device);
    this->samplers.at(1) = vk.resources.getSampler(vk.device,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_COMPARE_OP_NEVER, true);
    this->samplers.at(2) = vk.resources.getSampler(vk.device,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_COMPARE_OP_ALWAYS, false);

    // create internal images/outputs
    const VkExtent2D extent = this->inImgs1.at(0).at(0).getExtent();
    for (size_t i = 0; i < 4; i++) {
        this->tempImgs1.at(i) = Core::Image(vk.device, extent);
        this->tempImgs2.at(i) = Core::Image(vk.device, extent);
    }

    this->outImg = Core::Image(vk.device,
        { extent.width, extent.height },
        VK_FORMAT_R16G16B16A16_SFLOAT);

    // hook up shaders
    for (size_t pass_idx = 0; pass_idx < vk.generationCount; pass_idx++) {
        auto& pass = this->passes.emplace_back();
        pass.buffer = vk.resources.getBuffer(vk.device,
            static_cast<float>(pass_idx + 1) / static_cast<float>(vk.generationCount + 1),
            !this->optImg.has_value());
        for (size_t i = 0; i < 3; i++) {
            pass.firstDescriptorSet.at(i) = Core::DescriptorSet(vk.device, vk.descriptorPool,
                this->shaderModules.at(0));
            pass.firstDescriptorSet.at(i).update(vk.device)
                .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, pass.buffer)
                .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(1))
                .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(2))
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImgs1.at((i + 2) % 3))
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImgs1.at(i % 3))
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->optImg)
                .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1.at(0))
                .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1.at(1))
                .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1.at(2))
                .build();
        }
        pass.descriptorSets.at(0) = Core::DescriptorSet(vk.device, vk.descriptorPool,
            this->shaderModules.at(1));
        pass.descriptorSets.at(0).update(vk.device)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(0))
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1.at(0))
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1.at(1))
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1.at(2))
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs2)
            .build();
        pass.descriptorSets.at(1) = Core::DescriptorSet(vk.device, vk.descriptorPool,
            this->shaderModules.at(2));
        pass.descriptorSets.at(1).update(vk.device)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(0))
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs2)
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs1)
            .build();
        pass.descriptorSets.at(2) = Core::DescriptorSet(vk.device, vk.descriptorPool,
            this->shaderModules.at(3));
        pass.descriptorSets.at(2).update(vk.device)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(0))
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs1)
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs2)
            .build();
        pass.descriptorSets.at(3) = Core::DescriptorSet(vk.device, vk.descriptorPool,
            this->shaderModules.at(4));
        pass.descriptorSets.at(3).update(vk.device)
            .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, pass.buffer)
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(0))
            .add(VK_DESCRIPTOR_TYPE_SAMPLER, this->samplers.at(2))
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs2)
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->optImg)
            .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg2)
            .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImg)
            .build();
    }
}

void Gamma::Dispatch(const Core::CommandBuffer& buf, uint64_t frameCount, uint64_t pass_idx) {
    auto& pass = this->passes.at(pass_idx);

    // first shader
    const auto extent = this->tempImgs1.at(0).getExtent();
    const uint32_t threadsX = (extent.width + 7) >> 3;
    const uint32_t threadsY = (extent.height + 7) >> 3;

    Utils::BarrierBuilder(buf)
        .addW2R(this->inImgs1.at((frameCount + 2) % 3))
        .addW2R(this->inImgs1.at(frameCount % 3))
        .addW2R(this->optImg)
        .addR2W(this->tempImgs1.at(0))
        .addR2W(this->tempImgs1.at(1))
        .addR2W(this->tempImgs1.at(2))
        .build();

    this->pipelines.at(0).bind(buf);
    pass.firstDescriptorSet.at(frameCount % 3).bind(buf, this->pipelines.at(0));
    buf.dispatch(threadsX, threadsY, 1);

    // second shader
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs1.at(0))
        .addW2R(this->tempImgs1.at(1))
        .addW2R(this->tempImgs1.at(2))
        .addR2W(this->tempImgs2)
        .build();

    this->pipelines.at(1).bind(buf);
    pass.descriptorSets.at(0).bind(buf, this->pipelines.at(1));
    buf.dispatch(threadsX, threadsY, 1);

    // third shader
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs2)
        .addR2W(this->tempImgs1)
        .build();

    this->pipelines.at(2).bind(buf);
    pass.descriptorSets.at(1).bind(buf, this->pipelines.at(2));
    buf.dispatch(threadsX, threadsY, 1);

    // fourth shader
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs1)
        .addR2W(this->tempImgs2)
        .build();

    this->pipelines.at(3).bind(buf);
    pass.descriptorSets.at(2).bind(buf, this->pipelines.at(3));
    buf.dispatch(threadsX, threadsY, 1);

    // fifth shader
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs2)
        .addW2R(this->optImg)
        .addW2R(this->inImg2)
        .addR2W(this->outImg)
        .build();

    this->pipelines.at(4).bind(buf);
    pass.descriptorSets.at(3).bind(buf, this->pipelines.at(4));
    buf.dispatch(threadsX, threadsY, 1);
}
