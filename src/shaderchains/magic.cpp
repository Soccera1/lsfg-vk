#include "shaderchains/magic.hpp"
#include "utils.hpp"

using namespace Vulkan::Shaderchains;

Magic::Magic(const Device& device, const Core::DescriptorPool& pool,
        const std::vector<Core::Image>& temporalImgs,
        const std::vector<Core::Image>& inImgs1,
        Core::Image inImg2,
        Core::Image inImg3,
        const std::optional<Core::Image>& optImg)
        : temporalImgs(temporalImgs), inImgs1(inImgs1),
          inImg2(std::move(inImg2)), inImg3(std::move(inImg3)), optImg(optImg) {
    // create internal resources
    this->shaderModule = Core::ShaderModule(device, "rsc/shaders/magic.spv",
        { { 1,       VK_DESCRIPTOR_TYPE_SAMPLER },
          { 4+4+2+1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
          { 3+3+2,   VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
          { 1,       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER } });
    this->pipeline = Core::Pipeline(device, this->shaderModule);
    this->descriptorSet = Core::DescriptorSet(device, pool, this->shaderModule);

    Globals::FgBuffer data = Globals::fgBuffer;
    data.firstIterS = !optImg.has_value();
    this->buffer = Core::Buffer(device, data, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    auto extent = temporalImgs.at(0).getExtent();

    // create output images
    for (size_t i = 0; i < 2; i++)
        this->outImgs1.at(i) = Core::Image(device,
            { extent.width, extent.height },
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
    for (size_t i = 0; i < 3; i++)
        this->outImgs2.at(i) = Core::Image(device,
            { extent.width, extent.height },
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
    for (size_t i = 0; i < 3; i++)
        this->outImgs3.at(i) = Core::Image(device,
            { extent.width, extent.height },
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);

    // update descriptor set
    this->descriptorSet.update(device)
        .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->temporalImgs)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImgs1)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg2)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg3)
        .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, *this->optImg) // FIXME: invalid resource
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImgs3)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImgs2)
        .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImgs1)
        .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffer)
        .build();
}

void Magic::Dispatch(const Core::CommandBuffer& buf) {
    auto extent = this->temporalImgs.at(0).getExtent();
    const uint32_t threadsX = (extent.width + 7) >> 3;
    const uint32_t threadsY = (extent.height + 7) >> 3;

    Utils::insertBarrier(
        buf,
        { this->temporalImgs.at(0), this->temporalImgs.at(1),
          this->temporalImgs.at(2), this->temporalImgs.at(3),
          this->inImgs1.at(0), this->inImgs1.at(1),
          this->inImgs1.at(2), this->inImgs1.at(3),
          this->inImg2, this->inImg3, *this->optImg }, // FIXME: invalid resource
        { this->outImgs3.at(0), this->outImgs3.at(1), this->outImgs3.at(2),
          this->outImgs2.at(0), this->outImgs2.at(1), this->outImgs2.at(2),
          this->outImgs1.at(0), this->outImgs1.at(1) }
    );

    this->pipeline.bind(buf);
    this->descriptorSet.bind(buf, this->pipeline);
    buf.dispatch(threadsX, threadsY, 1);
}
