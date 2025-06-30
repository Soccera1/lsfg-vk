#include "shaderchains/extract.hpp"
#include "utils.hpp"

using namespace Vulkan::Shaderchains;

Extract::Extract(const Device& device, const Core::DescriptorPool& pool,
        Core::Image inImg1,
        Core::Image inImg2,
        VkExtent2D outExtent)
        : inImg1(std::move(inImg1)),
          inImg2(std::move(inImg2)) {
    this->shaderModule = Core::ShaderModule(device, "rsc/shaders/extract.spv",
        { { 2, VK_DESCRIPTOR_TYPE_SAMPLER },
          { 3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
          { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
          { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER } });
    this->pipeline = Core::Pipeline(device, this->shaderModule);
    this->descriptorSet = Core::DescriptorSet(device, pool, this->shaderModule);
    this->buffer = Core::Buffer(device, Globals::fgBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    this->whiteImg = Core::Image(device,
        outExtent,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);
    this->outImg = Core::Image(device,
        outExtent,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);

    this->descriptorSet.update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampEdge)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->whiteImg)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg1)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg2)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImg)
        .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffer)
        .build();

    // clear white image
    Utils::clearImage(device, this->whiteImg, true);
}

void Extract::Dispatch(const Core::CommandBuffer& buf) {
    auto extent = this->whiteImg.getExtent();

    // first pass
    const uint32_t threadsX = (extent.width + 7) >> 3;
    const uint32_t threadsY = (extent.height + 7) >> 3;

    Utils::BarrierBuilder(buf)
        .addW2R(this->whiteImg)
        .addW2R(this->inImg1)
        .addW2R(this->inImg2)
        .addR2W(this->outImg)
        .build();

    this->pipeline.bind(buf);
    this->descriptorSet.bind(buf, this->pipeline);
    buf.dispatch(threadsX, threadsY, 1);
}
