#include "shaderchains/alpha.hpp"
#include "utils.hpp"

using namespace Vulkan::Shaderchains;

Alpha::Alpha(const Device& device, const Core::DescriptorPool& pool,
        const Core::Image& inImage) : inImage(inImage) {
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

    auto extent = inImage.getExtent();
    auto halfWidth = (extent.width + 1) >> 1;
    auto halfHeight = (extent.height + 1) >> 1;
    auto quarterWidth = (extent.width + 3) >> 2;
    auto quarterHeight = (extent.height + 3) >> 2;

    for (size_t i = 0; i < 2; i++) {
        this->tempTex1.at(i) = Core::Image(device,
            { halfWidth, halfHeight },
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
        this->tempTex2.at(i) = Core::Image(device,
            { halfWidth, halfHeight },
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
    }
    for (size_t i = 0; i < 4; i++) {
        this->tempTex3.at(i) = Core::Image(device,
            { quarterWidth, quarterHeight },
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
        this->outImages.at(i) = Core::Image(device,
            { quarterWidth, quarterHeight },
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
    }

    this->descriptorSets.at(0).update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, inImage)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempTex1)
        .build();
    this->descriptorSets.at(1).update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempTex1)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempTex2)
        .build();
    this->descriptorSets.at(2).update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempTex2)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->tempTex3)
        .build();
    this->descriptorSets.at(3).update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->tempTex3)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImages)
        .build();
}

void Alpha::Dispatch(const Core::CommandBuffer& buf) {
    const auto halfExtent = this->tempTex1.at(0).getExtent();
    const auto quarterExtent = this->tempTex3.at(0).getExtent();

    // first pass
    Utils::insertBarrier(
        buf,
        { this->inImage },
        this->tempTex1
    );

    this->pipelines.at(0).bind(buf);
    this->descriptorSets.at(0).bind(buf, this->pipelines.at(0));

    uint32_t threadsX = (halfExtent.width + 7) >> 3;
    uint32_t threadsY = (halfExtent.height + 7) >> 3;
    buf.dispatch(threadsX, threadsY, 1);

    // second pass
    Utils::insertBarrier(
        buf,
        this->tempTex1,
        this->tempTex2
    );

    this->pipelines.at(1).bind(buf);
    this->descriptorSets.at(1).bind(buf, this->pipelines.at(1));

    buf.dispatch(threadsX, threadsY, 1);

    // third pass
    Utils::insertBarrier(
        buf,
        this->tempTex2,
        this->tempTex3
    );

    this->pipelines.at(2).bind(buf);
    this->descriptorSets.at(2).bind(buf, this->pipelines.at(2));

    threadsX = (quarterExtent.width + 7) >> 3;
    threadsY = (quarterExtent.height + 7) >> 3;
    buf.dispatch(threadsX, threadsY, 1);

    // fourth pass
    Utils::insertBarrier(
        buf,
        this->tempTex3,
        this->outImages
    );

    this->pipelines.at(3).bind(buf);
    this->descriptorSets.at(3).bind(buf, this->pipelines.at(3));

    buf.dispatch(threadsX, threadsY, 1);
}
