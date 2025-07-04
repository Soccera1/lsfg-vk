#include "shaderchains/merge.hpp"
#include "utils.hpp"

using namespace LSFG::Shaderchains;

Merge::Merge(const Core::Device& device, Pool::ShaderPool& shaderpool,
        const Core::DescriptorPool& pool,
        Core::Image inImg1,
        Core::Image inImg2,
        Core::Image inImg3,
        Core::Image inImg4,
        Core::Image inImg5,
        const std::vector<int>& outFds,
        size_t genc)
        : inImg1(std::move(inImg1)),
          inImg2(std::move(inImg2)),
          inImg3(std::move(inImg3)),
          inImg4(std::move(inImg4)),
          inImg5(std::move(inImg5)) {
    this->shaderModule = shaderpool.getShader(device, "merge.spv",
        { { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
          { 2, VK_DESCRIPTOR_TYPE_SAMPLER },
          { 5, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
          { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } });
    this->pipeline = Core::Pipeline(device, this->shaderModule);
    for (size_t i = 0; i < genc; i++) {
        this->nDescriptorSets.emplace_back();
        for (size_t j = 0; j < 2; j++)
            this->nDescriptorSets.at(i).at(j) = Core::DescriptorSet(device, pool, this->shaderModule);
    }
    for (size_t i = 0; i < genc; i++) {
        auto data = Globals::fgBuffer;
        data.timestamp = static_cast<float>(i + 1) / static_cast<float>(genc + 1);
        this->buffers.emplace_back(device, data, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }

    auto extent = this->inImg1.getExtent();

    for (size_t i = 0; i < genc; i++)
        this->outImgs.emplace_back(device,
            extent,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            outFds.at(i));

    for (size_t fc = 0; fc < 2; fc++) {
        for (size_t i = 0; i < genc; i++) {
            this->nDescriptorSets.at(i).at(fc).update(device)
                .add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->buffers.at(i))
                .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampBorder)
                .add(VK_DESCRIPTOR_TYPE_SAMPLER, Globals::samplerClampEdge)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, (fc % 2 == 0) ? this->inImg1 : this->inImg2)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, (fc % 2 == 0) ? this->inImg2 : this->inImg1)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg3)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg4)
                .add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, this->inImg5)
                .add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, this->outImgs.at(i))
                .build();
        }
    }
}

void Merge::Dispatch(const Core::CommandBuffer& buf, uint64_t fc, uint64_t pass) {
    auto extent = this->inImg1.getExtent();

    // first pass
    const uint32_t threadsX = (extent.width + 15) >> 4;
    const uint32_t threadsY = (extent.height + 15) >> 4;

    Utils::BarrierBuilder(buf)
        .addW2R(this->inImg1)
        .addW2R(this->inImg2)
        .addW2R(this->inImg3)
        .addW2R(this->inImg4)
        .addW2R(this->inImg5)
        .addR2W(this->outImgs.at(pass))
        .build();

    this->pipeline.bind(buf);
    this->nDescriptorSets.at(pass).at(fc % 2).bind(buf, this->pipeline);
    buf.dispatch(threadsX, threadsY, 1);
}
