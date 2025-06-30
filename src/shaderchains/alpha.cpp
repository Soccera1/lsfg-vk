#include "shaderchains/alpha.hpp"
#include "utils.hpp"

using namespace Vulkan::Shaderchains;

Alpha::Alpha(const Device& device, const Core::DescriptorPool& pool,
        Core::Image inImg)
        : inImg(std::move(inImg)) {
    this->shaderModules = {{
        Core::ShaderModule(device, "rsc/shaders/alpha/0.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
            { 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
            { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        Core::ShaderModule(device, "rsc/shaders/alpha/1.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
            { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
            { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        Core::ShaderModule(device, "rsc/shaders/alpha/2.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
            { 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
            { 4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }),
        Core::ShaderModule(device, "rsc/shaders/alpha/3.spv",
            { { 1, VK_DESCRIPTOR_TYPE_SAMPLER },
            { 4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
            { 4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } })
    }};
    for (size_t i = 0; i < 4; i++) {
        this->pipelines.at(i) = Core::Pipeline(device,
            this->shaderModules.at(i));
        this->descriptorSets.at(i) = Core::DescriptorSet(device, pool,
            this->shaderModules.at(i));
    }

    const auto extent = this->inImg.getExtent();

    const VkExtent2D halfExtent = {
        .width = (extent.width + 1) >> 1,
        .height = (extent.height + 1) >> 1
    };
    for (size_t i = 0; i < 2; i++) {
        this->tempImgs1.at(i) = Core::Image(device,
            halfExtent,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
        this->tempImgs2.at(i) = Core::Image(device,
            halfExtent,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
    }

    const VkExtent2D quarterExtent = {
        .width = (extent.width + 3) >> 2,
        .height = (extent.height + 3) >> 2
    };
    for (size_t i = 0; i < 4; i++) {
        this->tempImgs3.at(i) = Core::Image(device,
            quarterExtent,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
        this->outImgs.at(i) = Core::Image(device,
            quarterExtent,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
    }

    this->descriptorSets.at(0).update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg)
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
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempImgs3)
        .build();
    this->descriptorSets.at(3).update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempImgs3)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImgs)
        .build();
}

void Alpha::Dispatch(const Core::CommandBuffer& buf) {
    const auto halfExtent = this->tempImgs1.at(0).getExtent();
    const auto quarterExtent = this->tempImgs3.at(0).getExtent();

    // first pass
    uint32_t threadsX = (halfExtent.width + 7) >> 3;
    uint32_t threadsY = (halfExtent.height + 7) >> 3;

    Utils::BarrierBuilder(buf)
        .addW2R(this->inImg)
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
    threadsX = (quarterExtent.width + 7) >> 3;
    threadsY = (quarterExtent.height + 7) >> 3;

    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs2)
        .addR2W(this->tempImgs3)
        .build();

    this->pipelines.at(2).bind(buf);
    this->descriptorSets.at(2).bind(buf, this->pipelines.at(2));
    buf.dispatch(threadsX, threadsY, 1);

    // fourth pass
    Utils::BarrierBuilder(buf)
        .addW2R(this->tempImgs3)
        .addR2W(this->outImgs)
        .build();

    this->pipelines.at(3).bind(buf);
    this->descriptorSets.at(3).bind(buf, this->pipelines.at(3));
    buf.dispatch(threadsX, threadsY, 1);
}
